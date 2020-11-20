/* Copyright 2019 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "hal_api.h"
#include "hfperclk.h"
#include "board.h"
#include "api.h"

/* Map some registers constant to the USART selected */
#if BOARD_USART_ID == 0
#define BOARD_USART         USART0
#define BOARD_USART_CMU_BIT CMU_HFPERCLKEN0_USART0
#define BOARD_UART_RX_IRQn  USART0_RX_IRQn
#define BOARD_UART_TX_IRQn  USART0_TX_IRQn
#elif BOARD_USART_ID == 1
#define BOARD_USART         USART1
#define BOARD_USART_CMU_BIT CMU_HFPERCLKEN0_USART1
#define BOARD_UART_RX_IRQn  USART1_RX_IRQn
#define BOARD_UART_TX_IRQn  USART1_TX_IRQn
#elif BOARD_USART_ID == 2
#define BOARD_USART         USART2
#define BOARD_USART_CMU_BIT CMU_HFPERCLKEN0_USART2
#define BOARD_UART_RX_IRQn  USART2_RX_IRQn
#define BOARD_UART_TX_IRQn  USART2_TX_IRQn
#elif BOARD_USART_ID == 3
#define BOARD_USART         USART3
#define BOARD_USART_CMU_BIT CMU_HFPERCLKEN0_USART3
#define BOARD_UART_RX_IRQn  USART3_RX_IRQn
#define BOARD_UART_TX_IRQn  USART3_TX_IRQn
#else
#error USART ID must be 0, 1, 2 or 3
#endif

/* Only one USART, this is easy */
static volatile serial_rx_callback_f    m_rx_callback;

// Declare unique ring-buffer size
#define BUFFER_SIZE                     512u
#include "ringbuffer.h"

// Buffer for transmissions
static volatile ringbuffer_t            m_usart_tx_buffer;

/** Indicate if USART is enabled */
static volatile uint32_t                m_enabled;
static volatile bool                    m_tx_active;

/** Enable or disable HW flow control */
static void set_baud(uint32_t baud);

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

    //    /* Enable clocks */
    CMU->HFBUSCLKEN0 |= CMU_HFBUSCLKEN0_GPIO;

    //uart_tx_pin
    hal_gpio_set_mode(BOARD_USART_GPIO_PORT,
                      BOARD_USART_TX_PIN,
                      GPIO_MODE_DISABLED);
    hal_gpio_clear(BOARD_USART_GPIO_PORT, BOARD_USART_TX_PIN);
    //uart_rx_pin
    hal_gpio_set_mode(BOARD_USART_GPIO_PORT,
                      BOARD_USART_RX_PIN,
                      GPIO_MODE_DISABLED);
    hal_gpio_clear(BOARD_USART_GPIO_PORT, BOARD_USART_RX_PIN);

    /* Module variables */
    Ringbuffer_reset(m_usart_tx_buffer);
    m_rx_callback = NULL;
    m_tx_active = false;
    m_enabled = 0;

    /* Disable for RX */
    Sys_disableAppIrq(BOARD_UART_RX_IRQn);
    Sys_clearFastAppIrq(BOARD_UART_RX_IRQn);
    /* Disable for TX */
    Sys_disableAppIrq(BOARD_UART_TX_IRQn);
    Sys_clearFastAppIrq(BOARD_UART_TX_IRQn);

    // Must enable clock for configuration period
    enableHFPERCLK(true);
    /* Set UART output pins */
    BOARD_USART->ROUTEPEN = USART_ROUTEPEN_RXPEN |
                            USART_ROUTEPEN_TXPEN;

    /* Set UART route */
    BOARD_USART->ROUTELOC0 = BOARD_USART_ROUTELOC_RXLOC |
                             BOARD_USART_ROUTELOC_TXLOC;

    /* Initialize UART for asynch mode with baudrate baud */
    /* Disable transceiver */
    BOARD_USART->CMD = 0;
    BOARD_USART->CTRL = 0;
    BOARD_USART->I2SCTRL = 0;
    /* Disables PRS */
    BOARD_USART->INPUT = 0;
    /* Set frame params: 8bit, nopar, 1stop */
    BOARD_USART->FRAME = USART_FRAME_DATABITS_EIGHT |
                         USART_FRAME_PARITY_NONE |
                         USART_FRAME_STOPBITS_ONE;
    set_baud(baudrate);

    /* Disable all interrupt sources */
    BOARD_USART->IEN = 0;
    /* Clear all irq flags */
    BOARD_USART->IFC = _USART_IFC_MASK;
    /* Enable transmitter */
    BOARD_USART->CMD = USART_CMD_TXEN;

    /* APP IRQ */
    Sys_clearFastAppIrq(BOARD_UART_TX_IRQn);
    Sys_enableFastAppIrq(BOARD_UART_TX_IRQn,
                         APP_LIB_SYSTEM_IRQ_PRIO_HI,
                         USART_TX_IRQHandler);

    // Configuration done: disable clock
    enableHFPERCLK(false);

    return true;
}

void Usart_setEnabled(bool enabled)
{
    Sys_enterCriticalSection();
    if(enabled)
    {
        /* Detect if someone is enabling UART but not disabling it ever */
        if(m_enabled == 0)
        {
            // Enable clock
            enableHFPERCLK(true);
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
            enableHFPERCLK(false);
        }
    }
    Sys_exitCriticalSection();
}

void Usart_receiverOn(void)
{
    BITBAND_Peripheral(&(BOARD_USART->CMD), _USART_CMD_RXEN_SHIFT, 1);
}

void Usart_receiverOff(void)
{
    BITBAND_Peripheral(&(BOARD_USART->CMD), _USART_CMD_RXDIS_SHIFT, 1);
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
        BITBAND_Peripheral(&(BOARD_USART->IEN), _USART_IEN_TXC_SHIFT, 1);
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
    /* Set callback */
    m_rx_callback = rx_callback;
    // Enable clock
    enableHFPERCLK(true);
    if(rx_callback)
    {
        Sys_enableFastAppIrq(BOARD_UART_RX_IRQn,
                             APP_LIB_SYSTEM_IRQ_PRIO_HI,
                             USART_RX_IRQHandler);
        BITBAND_Peripheral(&(BOARD_USART->IEN), _USART_IEN_RXDATAV_SHIFT, 1);
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
        BITBAND_Peripheral(&(BOARD_USART->IEN), _USART_IEN_RXDATAV_SHIFT, 0);
        hal_gpio_set_mode(BOARD_USART_GPIO_PORT,
                          BOARD_USART_RX_PIN,
                          GPIO_MODE_DISABLED);
        // Disable pull-up for disabled GPIO:s
        hal_gpio_clear(BOARD_USART_GPIO_PORT,
                       BOARD_USART_RX_PIN);
    }
    /* Clear all interrupts */
    dummy = BOARD_USART->RXDATA;
    BOARD_USART->IFC = USART_IFC_RXFULL;
    // Disable clock
    enableHFPERCLK(false);
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
    /* Data received */
    uint16_t ch = BOARD_USART->RXDATA;
    /* RXFULL must be explicitly cleared */
    BOARD_USART->IFC = USART_IFC_RXFULL;
    if (m_rx_callback != NULL)
    {
        m_rx_callback((uint8_t *) &ch, 1);
    }
//    DBG_EXIT_IRQ_USART();
}

void __attribute__((__interrupt__)) USART_TX_IRQHandler(void)
{
//    DBG_ENTER_IRQ_USART();
    BOARD_USART->IFC = _USART_IFC_TXC_MASK;
    /* byte has been sent -> move tail */
    Ringbuffer_incrTail(m_usart_tx_buffer, 1);
    if (Ringbuffer_usage(m_usart_tx_buffer) != 0)
    {
        BOARD_USART->TXDATA = Ringbuffer_getTailByte(m_usart_tx_buffer);
    }
    else
    {
        /* when buffer becomes empty, reset indexes */
        BITBAND_Peripheral(&(BOARD_USART->IEN), _USART_IEN_TXC_SHIFT, 0);
        Usart_setEnabled(false);
        m_tx_active = false;
    }
//    DBG_EXIT_IRQ_USART();
}

static void set_baud(uint32_t baud)
{
    volatile uint32_t baud_gen;
    /* Calculate baudrate: see em_usart.c in emlib for reference */
    baud_gen = 32 * HFPERCLK_FREQ + (4 * baud) / 2;
    baud_gen /= (4 * baud);
    baud_gen -= 32;
    baud_gen *= 8;
    baud_gen &= _USART_CLKDIV_DIV_MASK;

    /* Set oversampling bit (8) */
    BOARD_USART->CTRL  &= ~_USART_CTRL_OVS_MASK;
    BOARD_USART->CTRL  |= USART_CTRL_OVS_X4;
    BOARD_USART->CLKDIV = baud_gen;
}

