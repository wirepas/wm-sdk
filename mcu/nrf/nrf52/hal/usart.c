/* Copyright 2017 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "board.h"
#include "hal_api.h"
#include "api.h"


#if defined(BOARD_USART_TX_PIN) && defined (BOARD_USART_RX_PIN)

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

#if defined(BOARD_USART_CTS_PIN) && defined (BOARD_USART_RTS_PIN)
/** Enable or disable HW flow control */
static void                             set_flow_control(bool hw);
#endif

/** Set uarte baudrate */
static bool                             set_baud(uint32_t baudrate);

/** Declare the interrupt handler */
void __attribute__((__interrupt__))     UART0_IRQHandler(void);

bool Usart_init(uint32_t baudrate, uart_flow_control_e flow_control)
{
    bool ret;
    //uart_tx_pin
    nrf_gpio_cfg_default(BOARD_USART_TX_PIN);
    nrf_gpio_pin_set(BOARD_USART_TX_PIN);

    //uart_rx_pin
    nrf_gpio_cfg_default(BOARD_USART_RX_PIN);
    nrf_gpio_pin_set(BOARD_USART_RX_PIN);

    /* Module variables */
    m_enabled = false;
    Ringbuffer_reset(m_usart_tx_buffer);
    m_rx_callback = NULL;
    m_tx_active = false;

    /* GPIO init */
    NRF_UART0->PSELTXD = BOARD_USART_TX_PIN;
    NRF_UART0->PSELRXD = BOARD_USART_RX_PIN;
    NRF_UART0->TASKS_STOPTX = 1;
    NRF_UART0->TASKS_STOPRX = 1;
    NRF_UART0->ENABLE = UART_ENABLE_ENABLE_Disabled;

#if defined(BOARD_USART_CTS_PIN) && defined (BOARD_USART_RTS_PIN)
    //uart_cts_pin
    nrf_gpio_cfg_default(BOARD_USART_CTS_PIN);
    nrf_gpio_pin_set(BOARD_USART_CTS_PIN);

    //uart_rts_pin
    nrf_gpio_cfg_default(BOARD_USART_RTS_PIN);
    nrf_gpio_pin_set(BOARD_USART_RTS_PIN);

    // Set flow control
    set_flow_control(flow_control == UART_FLOW_CONTROL_HW);
#endif

    /* Uart speed */
    ret = set_baud(baudrate);
    // Even if ret is False, do the end of init to have a uart at default baudrate

    NRF_UART0->EVENTS_RXDRDY = 0;
    NRF_UART0->EVENTS_TXDRDY = 0;

    /* Interrupt init */
    /* This bitmask is from nrf examples, because the reference manual
     * simply does not contain the register information... */
    NRF_UART0->INTENCLR = 0xffffffffUL;
    NRF_UART0->INTENSET =
    (UART_INTENSET_TXDRDY_Set << UART_INTENSET_TXDRDY_Pos) |
    (UART_INTENSET_ERROR_Set << UART_INTENSET_ERROR_Pos) |
    (UART_INTENSET_RXTO_Set << UART_INTENSET_RXTO_Pos);

    /* APP IRQ */
    Sys_clearFastAppIrq(UART0_IRQn);
    Sys_enableFastAppIrq(UART0_IRQn, APP_LIB_SYSTEM_IRQ_PRIO_HI, UART0_IRQHandler);

    return ret;
}

void Usart_setEnabled(bool enabled)
{
    Sys_enterCriticalSection();
    if (enabled)
    {
        if(m_enabled == 0)
        {
            // Disable deep sleep
            DS_Disable(DS_SOURCE_USART);
            // Set output
            nrf_gpio_cfg(BOARD_USART_TX_PIN,
                         NRF_GPIO_PIN_DIR_OUTPUT,
                         NRF_GPIO_PIN_INPUT_CONNECT,
                         NRF_GPIO_PIN_NOPULL,
                         NRF_GPIO_PIN_S0S1,
                         NRF_GPIO_PIN_NOSENSE);
            NRF_UART0->ENABLE = UART_ENABLE_ENABLE_Enabled;
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
            NRF_UART0->ENABLE = UART_ENABLE_ENABLE_Disabled;
            // Set input
            nrf_gpio_cfg_input(BOARD_USART_TX_PIN, NRF_GPIO_PIN_NOPULL);
            // Enable deep sleep
            DS_Enable(DS_SOURCE_USART);
        }
    }
    Sys_exitCriticalSection();
}

void Usart_receiverOn(void)
{
    NRF_UART0->TASKS_STARTRX = 1;
}

void Usart_receiverOff(void)
{
    NRF_UART0->TASKS_STOPRX = 1;
}

bool Usart_setFlowControl(uart_flow_control_e flow)
{
    bool ret = false;

#if defined(BOARD_USART_CTS_PIN) && defined (BOARD_USART_RTS_PIN)
    Sys_enterCriticalSection();
    if (m_enabled == 0)
    {
        switch (flow)
        {
            case UART_FLOW_CONTROL_NONE:
                set_flow_control(false);
                ret = true;
                break;
            case UART_FLOW_CONTROL_HW:
                set_flow_control(true);
                ret = true;
                break;
            default:
                break;
        }
    }
    Sys_exitCriticalSection();
#endif

    return ret;
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
        NRF_UART0->TASKS_STARTTX = 1;
        NRF_UART0->TXD = Ringbuffer_getTailByte(m_usart_tx_buffer);
        m_tx_active = true;
    }
buffer_too_large:
    Sys_exitCriticalSection();
    return size_in;
}

void Usart_enableReceiver(serial_rx_callback_f rx_callback)
{
    Sys_enterCriticalSection();
    /* Set callback */
    m_rx_callback = rx_callback;
    if (m_rx_callback)
    {
        /* Enable interrupt */
        NRF_UART0->INTENSET =
        (UART_INTENSET_RXDRDY_Set << UART_INTENSET_RXDRDY_Pos);
        // Enable RX input
        nrf_gpio_cfg(BOARD_USART_RX_PIN,
                     NRF_GPIO_PIN_DIR_INPUT,
                     NRF_GPIO_PIN_INPUT_CONNECT,
                     NRF_GPIO_PIN_NOPULL,
                     NRF_GPIO_PIN_S0S1,
                     NRF_GPIO_PIN_SENSE_LOW);
    }
    else
    {
        /* Disable interrupt */
        NRF_UART0->INTENCLR =
        (UART_INTENSET_RXDRDY_Set << UART_INTENSET_RXDRDY_Pos);
        // Disable RX input: note autopowering uart will not work either
        nrf_gpio_cfg_default(BOARD_USART_RX_PIN);
    }
    Sys_exitCriticalSection();
}

uint32_t Usart_getMTUSize(void)
{
    return BUFFER_SIZE;
}

void Usart_flush(void)
{
    volatile uint32_t timeout = 20000;
    while(m_tx_active && timeout > 0)
    {
        timeout--;
    }
}

/**@brief Function for handling the USART Interrupt.
 *
 * @details USART interrupt handler to process TX Ready when TXD is available,
 *          RX Ready when a byte is received,
 *          or in case of error when receiving a byte.
 */
void __attribute__((__interrupt__)) UART0_IRQHandler(void)
{
    // Handle reception
    if (NRF_UART0->EVENTS_RXDRDY != 0)
    {
        uint8_t rx_byte;
        // Clear event first
        NRF_UART0->EVENTS_RXDRDY  = 0;
        rx_byte = NRF_UART0->RXD;
        if(m_rx_callback != NULL)
        {
            m_rx_callback(&rx_byte, 1);
        }
    }
    /* TX byte complete, start next or disable transmitter */
    if (NRF_UART0->EVENTS_TXDRDY != 0)
    {
        NRF_UART0->EVENTS_TXDRDY = 0;
        /* byte has been sent -> move tail */
        Ringbuffer_incrTail(m_usart_tx_buffer, 1);
        if (Ringbuffer_usage(m_usart_tx_buffer) != 0)
        {
            NRF_UART0->TXD = Ringbuffer_getTailByte(m_usart_tx_buffer);
        }
        else
        {
            /* when buffer becomes empty, reset indexes */
            NRF_UART0->TASKS_STOPTX = 1;
            Usart_setEnabled(false);
            m_tx_active = false;
        }
    }
    // Handle errors.
    if (NRF_UART0->EVENTS_ERROR != 0)
    {
        NRF_UART0->EVENTS_ERROR = 0;
    }
    if (NRF_UART0->EVENTS_RXTO != 0)
    {
        NRF_UART0->EVENTS_RXTO = 0;
    }
    // read any event from peripheral to flush the write buffer:
    EVENT_READBACK = NRF_UART0->EVENTS_RXDRDY;
}

#if defined(BOARD_USART_CTS_PIN) && defined (BOARD_USART_RTS_PIN)
static void set_flow_control(bool hw)
{
    if (hw)
    {
        // Set input & pull down
        nrf_gpio_cfg_sense_input(BOARD_USART_CTS_PIN,
                                 NRF_GPIO_PIN_PULLDOWN,
                                 NRF_GPIO_PIN_NOSENSE);
        nrf_gpio_cfg_sense_input(BOARD_USART_RTS_PIN,
                                 NRF_GPIO_PIN_PULLDOWN,
                                 NRF_GPIO_PIN_NOSENSE);

        NRF_UART0->PSELRTS = 0xFFFFFFFF;
        NRF_UART0->PSELCTS = BOARD_USART_CTS_PIN;
        /* No parity, HW flow control */
        NRF_UART0->CONFIG = (UART_CONFIG_HWFC_Enabled << UART_CONFIG_HWFC_Pos);
    }
    else
    {
        NRF_UART0->PSELRTS = 0xFFFFFFFF;
        NRF_UART0->PSELCTS = 0xFFFFFFFF;
        /* No parity, no HW flow control */
        NRF_UART0->CONFIG = 0;

        // Deactivate HAL_USART_CTS_PIN
        nrf_gpio_cfg_default(BOARD_USART_CTS_PIN);
        // Deactivate HAL_USART_RTS_PIN
        nrf_gpio_cfg_default(BOARD_USART_RTS_PIN);
    }
}
#endif

static bool set_baud(uint32_t baudrate)
{
    bool ret = true;
    switch (baudrate)
    {
    case 115200:
        NRF_UART0->BAUDRATE = (uint32_t)UARTE_BAUDRATE_BAUDRATE_Baud115200;
        break;
    case 125000:
        // This value is not from official nrf52_bitfields.h
        NRF_UART0->BAUDRATE = (uint32_t)(0x02000000UL);
        break;
    case 250000:
        NRF_UART0->BAUDRATE = (uint32_t)UART_BAUDRATE_BAUDRATE_Baud250000;
        break;
    case 460800:
        NRF_UART0->BAUDRATE = (uint32_t)UART_BAUDRATE_BAUDRATE_Baud460800;
        break;
    case 1000000:
        NRF_UART0->BAUDRATE = (uint32_t)UARTE_BAUDRATE_BAUDRATE_Baud1M;
        break;
    default:
        // Intended baudrate is not in the list, default baudrate from chip will be used
        ret = false;
        break;
    }

    return ret;
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
