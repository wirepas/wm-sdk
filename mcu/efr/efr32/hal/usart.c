/* Copyright 2019 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "hal_api.h"
#include "board.h"
#include "board_usart.h"
#include "api.h"

#include "em_cmu.h"
#include "em_gpio.h"
#include "em_usart.h"

#if defined(BOARD_USART_TX_PIN) && defined (BOARD_USART_RX_PIN)

static volatile serial_rx_callback_f    m_rx_callback;

// Declare unique ring-buffer size
#define BUFFER_SIZE                     512u
#include "ringbuffer.h"

// Buffer for transmissions
static volatile ringbuffer_t            m_usart_tx_buffer;

/** Indicate if USART is enabled */
static volatile uint32_t                m_enabled;
static volatile bool                    m_tx_active;

/** Declare the interrupt handlers */
void __attribute__((__interrupt__))     USART_RX_IRQHandler(void);
void __attribute__((__interrupt__))     USART_TX_IRQHandler(void);

bool Usart_init(uint32_t baudrate, uart_flow_control_e flow_control)
{
#ifdef BOARD_USART_FORCE_BAUDRATE
    // Some hardware only support a given speed, so override the chosen baudrate
    baudrate = BOARD_USART_FORCE_BAUDRATE;
#endif
    (void)flow_control;

    // Enable GPIO clock
    BOARD_USART_ENABLE_GPIO_CLK;

#ifdef BOARD_USART_VCOM_PORT
    // Enable vcom
    hal_gpio_set_mode(BOARD_USART_VCOM_PORT,
                      BOARD_USART_VCOM_PIN,
                      GPIO_MODE_DISABLED);
    hal_gpio_clear(BOARD_USART_VCOM_PORT,
                   BOARD_USART_VCOM_PIN);
    hal_gpio_set_mode(BOARD_USART_VCOM_PORT,
                      BOARD_USART_VCOM_PIN,
                      GPIO_MODE_OUT_PP);
    hal_gpio_set(BOARD_USART_VCOM_PORT,
                 BOARD_USART_VCOM_PIN);
#endif

    // uart_tx_pin
    hal_gpio_set_mode(BOARD_USART_GPIO_PORT,
                      BOARD_USART_TX_PIN,
                      GPIO_MODE_DISABLED);
    hal_gpio_clear(BOARD_USART_GPIO_PORT,
                   BOARD_USART_TX_PIN);
    // uart_rx_pin
    hal_gpio_set_mode(BOARD_USART_GPIO_PORT,
                      BOARD_USART_RX_PIN,
                      GPIO_MODE_DISABLED);
    hal_gpio_clear(BOARD_USART_GPIO_PORT,
                   BOARD_USART_RX_PIN);

    // Module variables
    Ringbuffer_reset(m_usart_tx_buffer);
    m_rx_callback = NULL;
    m_tx_active = false;
    m_enabled = 0;

    // Disable for RX
    Sys_disableAppIrq(BOARD_UART_RX_IRQn);
    Sys_clearFastAppIrq(BOARD_UART_RX_IRQn);
    // Disable for TX
    Sys_disableAppIrq(BOARD_UART_TX_IRQn);
    Sys_clearFastAppIrq(BOARD_UART_TX_IRQn);

    // Must enable clock for configuration period
    BOARD_USART_ENABLE_USART_CLK;

    // Set UART baudrate
    USART_InitAsync_TypeDef USART_init = USART_INITASYNC_DEFAULT;
    USART_init.enable = usartDisable;
    USART_init.baudrate = baudrate;
    USART_init.oversampling = usartOVS4;
    USART_init.hwFlowControl = usartHwFlowControlNone;
    USART_InitAsync(BOARD_USART, &USART_init);

    // Set UART route
    BOARD_USART_ROUTE;

    // Set UART output pins
    BOARD_USART_PINS;

    // Disable all interrupt sources
    BOARD_USART->IEN = 0;
    // Clear all irq flags
    BOARD_USART_CLR_IRQ_ALL;
    // Enable transmitter
    BOARD_USART->CMD = USART_CMD_TXEN;

    // APP IRQ
    Sys_clearFastAppIrq(BOARD_UART_TX_IRQn);
    Sys_enableFastAppIrq(BOARD_UART_TX_IRQn,
                         APP_LIB_SYSTEM_IRQ_PRIO_HI,
                         USART_TX_IRQHandler);

    // Configuration done: disable clock
    BOARD_USART_DISABLE_USART_CLK;

    return true;
}

void Usart_setEnabled(bool enabled)
{
    Sys_enterCriticalSection();
    if(enabled)
    {
        // Detect if someone is enabling UART but not disabling it ever
        if(m_enabled == 0)
        {
            // Enable clock
            BOARD_USART_ENABLE_USART_CLK;
            // Disable deep sleep
            DS_Disable(DS_SOURCE_USART);
            // Set output
            hal_gpio_set_mode(BOARD_USART_GPIO_PORT,
                              BOARD_USART_TX_PIN,
                              GPIO_MODE_OUT_PP);
        }
        m_enabled++;
    }
    else
    {
        if (m_enabled > 0)
        {
            m_enabled--;
        }
        if(m_enabled == 0)
        {
            // Set light pullup
            hal_gpio_set_mode(BOARD_USART_GPIO_PORT,
                              BOARD_USART_TX_PIN,
                              GPIO_MODE_IN_PULL);
            hal_gpio_set(BOARD_USART_GPIO_PORT,
                         BOARD_USART_TX_PIN);
            // Enable deep sleep
            DS_Enable(DS_SOURCE_USART);
            // Disable clock
            BOARD_USART_DISABLE_USART_CLK;
        }
    }
    Sys_exitCriticalSection();
}

void Usart_receiverOn(void)
{
    BUS_RegBitWrite(&(BOARD_USART->CMD), _USART_CMD_RXEN_SHIFT, 1);
}

void Usart_receiverOff(void)
{
    BUS_RegBitWrite(&(BOARD_USART->CMD), _USART_CMD_RXDIS_SHIFT, 1);
}

bool Usart_setFlowControl(uart_flow_control_e flow)
{
    (void)flow;
    return false;
}

uint32_t Usart_sendBuffer(const void * buffer, uint32_t length)
{
    bool empty = false;
    uint32_t size_in = length;
    uint8_t * data_out = (uint8_t *)buffer;
    Sys_enterCriticalSection();
    if (Ringbuffer_free(m_usart_tx_buffer) < length)
    {
        size_in = 0;
        goto buffer_too_large;
    }
    empty = (Ringbuffer_usage(m_usart_tx_buffer) == 0);
    while(length--)
    {
        Ringbuffer_getHeadByte(m_usart_tx_buffer) = *data_out++;
        Ringbuffer_incrHead(m_usart_tx_buffer, 1);
    }
    if (empty)
    {
        Usart_setEnabled(true);
        BUS_RegBitWrite(&(BOARD_USART->IEN), _USART_IEN_TXC_SHIFT, 1);
        BOARD_USART->TXDATA = Ringbuffer_getTailByte(m_usart_tx_buffer);
        m_tx_active = true;
    }
buffer_too_large:
    Sys_exitCriticalSection();
    return size_in;
}

void Usart_enableReceiver(serial_rx_callback_f rx_callback)
{
    uint32_t __attribute__((unused))dummy;
    Sys_enterCriticalSection();
    // Set callback
    m_rx_callback = rx_callback;
    // Enable clock
    BOARD_USART_ENABLE_USART_CLK;
    if(rx_callback)
    {
        Sys_enableFastAppIrq(BOARD_UART_RX_IRQn,
                             APP_LIB_SYSTEM_IRQ_PRIO_HI,
                             USART_RX_IRQHandler);
        BUS_RegBitWrite(&(BOARD_USART->IEN), _USART_IEN_RXDATAV_SHIFT, 1);
        // Set light pull-up resistor
        hal_gpio_set_mode(BOARD_USART_GPIO_PORT,
                          BOARD_USART_RX_PIN,
                          GPIO_MODE_IN_PULL);
        hal_gpio_set(BOARD_USART_GPIO_PORT,
                     BOARD_USART_RX_PIN);

    }
    else
    {
        Sys_disableAppIrq(BOARD_UART_RX_IRQn);
        BUS_RegBitWrite(&(BOARD_USART->IEN), _USART_IEN_RXDATAV_SHIFT, 0);
        hal_gpio_set_mode(BOARD_USART_GPIO_PORT,
                          BOARD_USART_RX_PIN,
                          GPIO_MODE_DISABLED);
        // Disable pull-up for disabled GPIO:s
        hal_gpio_clear(BOARD_USART_GPIO_PORT,
                       BOARD_USART_RX_PIN);
    }
    // Clear all interrupts
    dummy = BOARD_USART->RXDATA;
    BOARD_USART_CLR_IRQ_RXFULL;
    // Disable clock
    BOARD_USART_DISABLE_USART_CLK;
    Sys_clearFastAppIrq(BOARD_UART_RX_IRQn);
    Sys_exitCriticalSection();
}

uint32_t Usart_getMTUSize(void)
{
    return BUFFER_SIZE;
}

void Usart_flush(void)
{
    while(m_tx_active)
    {
    }
}

void __attribute__((__interrupt__)) USART_RX_IRQHandler(void)
{
//    DBG_ENTER_IRQ_USART();
    // Data received
    uint16_t ch = BOARD_USART->RXDATA;
    // RXFULL must be explicitly cleared
    BOARD_USART_CLR_IRQ_RXFULL;
    if (m_rx_callback != NULL)
    {
        m_rx_callback((uint8_t *) &ch, 1);
    }
//    DBG_EXIT_IRQ_USART();
}

void __attribute__((__interrupt__)) USART_TX_IRQHandler(void)
{
//    DBG_ENTER_IRQ_USART();
    BOARD_USART_CLR_IRQ_TXC;
    // byte has been sent -> move tail
    Ringbuffer_incrTail(m_usart_tx_buffer, 1);
    if (Ringbuffer_usage(m_usart_tx_buffer) != 0)
    {
        BOARD_USART->TXDATA = Ringbuffer_getTailByte(m_usart_tx_buffer);
    }
    else
    {
        // when buffer becomes empty, reset indexes
        BUS_RegBitWrite(&(BOARD_USART->IEN), _USART_IEN_TXC_SHIFT, 0);
        Usart_setEnabled(false);
        m_tx_active = false;
    }
//    DBG_EXIT_IRQ_USART();
}

#else

bool Usart_init(uint32_t baudrate, uart_flow_control_e flow_control)
{
    (void) baudrate;
    (void) flow_control;

    return false;
}

void Usart_setEnabled(bool enabled)
{
    (void) enabled;
}

void Usart_receiverOn(void)
{
}

void Usart_receiverOff(void)
{
}

bool Usart_setFlowControl(uart_flow_control_e flow)
{
    (void) flow;

    return false;
}

uint32_t Usart_sendBuffer(const void * buf, uint32_t len)
{
    (void) buf;
    (void) len;

    return 0;
}

void Usart_enableReceiver(serial_rx_callback_f callback)
{
    (void) callback;
}

uint32_t Usart_getMTUSize(void)
{
    return 0;
}

void Usart_flush(void)
{
}

#endif