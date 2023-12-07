/* Copyright 2021 Wirepas Ltd. All Rights Reserved.
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

/* Only one USART, this is easy */
static volatile serial_rx_callback_f    m_rx_callback;

/** Declare buffer size (defined by max DMA transfert size) */
#define BUFFER_SIZE                     256u
#include "doublebuffer.h"

/* Buffers used for transmission */
static double_buffer_t m_tx_buffers;

/* Is tx_ongoing ? */
static volatile bool m_tx_ongoing;

/* Buffer used for reception
 * Rx happens in a circular buffer and is limited to 255 bytes as it is
 * the maximum DMA RX transfer available
 */
#define RX_BUFFER_SIZE                 255u
static uint8_t m_rx_circular_buffer[RX_BUFFER_SIZE];
static uint8_t m_last_rx_index = 0;

/* Timeout in number of bytes at configured baudrate to declare that
 * a message is received */
#define TIMEOUT_CHAR_N                  5

/** Indicate if USART is enabled */
static volatile uint32_t                m_enabled;

/** Indicate if RX is enabled */
static bool                             m_rx_enabled;

#if defined(BOARD_USART_CTS_PIN) && defined (BOARD_USART_RTS_PIN)
/** Enable or disable HW flow control */
static void                             set_flow_control(bool hw);
#endif

/** Set uarte baudrate */
static bool                             set_baud(uint32_t baudrate);

/** Initialize DMA part */
static void                             init_dma(uint32_t baudrate);

/** Called each time a transfer is ready or when a previous transfer
 *  was finished. This function must be called with interrupts
 *  disabled or from interrupt context
 */
static void                             start_tx_lock(void);

/** Declare the interrupt handler */
void __attribute__((__interrupt__))     UARTE0_IRQHandler(void);
void __attribute__((__interrupt__))     TIMER1_IRQHandler(void);


bool Usart_init(uint32_t baudrate, uart_flow_control_e flow_control)
{
    bool ret;

    /* Module variables */
    m_enabled = 0;
    m_rx_enabled = false;
    m_rx_callback = NULL;

    /* GPIO init */
    nrf_gpio_cfg_default(BOARD_USART_TX_PIN);
    nrf_gpio_pin_set(BOARD_USART_TX_PIN);
    nrf_gpio_cfg_default(BOARD_USART_RX_PIN);
    nrf_gpio_pin_set(BOARD_USART_RX_PIN);

    NRF_UARTE0->PSEL.TXD = BOARD_USART_TX_PIN;
    NRF_UARTE0->PSEL.RXD = BOARD_USART_RX_PIN;
    NRF_UARTE0->TASKS_STOPTX = 1;
    NRF_UARTE0->TASKS_STOPRX = 1;
    NRF_UARTE0->ENABLE = UARTE_ENABLE_ENABLE_Disabled;

#if defined(BOARD_USART_CTS_PIN) && defined (BOARD_USART_RTS_PIN)
    nrf_gpio_cfg_default(BOARD_USART_CTS_PIN);
    nrf_gpio_pin_set(BOARD_USART_CTS_PIN);
    nrf_gpio_cfg_default(BOARD_USART_RTS_PIN);
    nrf_gpio_pin_set(BOARD_USART_RTS_PIN);
    /* Set flow control */
    set_flow_control(flow_control == UART_FLOW_CONTROL_HW);
#endif

    /* Uart speed */
    ret = set_baud(baudrate);
    // Even if ret is False, do the end of init to have a uart at default baudrate

    /* Initialize DMA part*/
    init_dma(baudrate);

    /* APP IRQ */
    Sys_clearFastAppIrq(UART0_IRQn);
    Sys_enableFastAppIrq(UART0_IRQn,
                         APP_LIB_SYSTEM_IRQ_PRIO_HI,
                         UARTE0_IRQHandler);

    Sys_clearFastAppIrq(TIMER1_IRQn);
    Sys_enableFastAppIrq(TIMER1_IRQn,
                         APP_LIB_SYSTEM_IRQ_PRIO_HI,
                         TIMER1_IRQHandler);

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
            NRF_UARTE0->ENABLE = UARTE_ENABLE_ENABLE_Enabled;
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
            NRF_UARTE0->ENABLE = UARTE_ENABLE_ENABLE_Disabled;
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
    Sys_enterCriticalSection();
    if (m_rx_enabled)
    {
        // Already enabled
        Sys_exitCriticalSection();
        return;
    }

    m_rx_enabled = true;

    // Clear events
    NRF_UARTE0->EVENTS_RXDRDY = 0;
    NRF_UARTE0->EVENTS_ENDRX = 0;
    NRF_UARTE0->EVENTS_RXSTARTED = 0;

    // Reset Timer 1
    NRF_TIMER1->EVENTS_COMPARE[0] = 0;

    // Prepare RX buffer
    NRF_UARTE0->RXD.PTR = (uint32_t) m_rx_circular_buffer;
    NRF_UARTE0->RXD.MAXCNT = RX_BUFFER_SIZE;
    m_last_rx_index = 0;

    // Reset Timer2 that is our index in circular buffer
    NRF_TIMER2->TASKS_CLEAR = 1;
    NRF_TIMER2->TASKS_START = 1;

    // Enable our ppi group
    NRF_DPPIC->TASKS_CHG[0].EN = 1;

    // Start uart reception
    NRF_UARTE0->TASKS_STARTRX = 1;
    Sys_exitCriticalSection();
}

void Usart_receiverOff(void)
{
    Sys_enterCriticalSection();
    // Disable our ppi group
    NRF_DPPIC->TASKS_CHG[0].DIS = 1;

    // Stop uart reception
    NRF_UARTE0->TASKS_STOPRX = 1;

    // Stop Timer2 that count the RX bytes
    NRF_TIMER2->TASKS_STOP = 1;

    // Stop Timer1 that is used for timeout and started
    // once RX is started to reduce power consumption.
    // It may be improved even further by stopping it when
    // there is no byte received yet, but not important as
    // we already have auto-power mechanism when needed.
    NRF_TIMER1->TASKS_STOP = 1;

    m_rx_enabled = false;
    Sys_exitCriticalSection();
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
    Sys_enterCriticalSection();

    // Check if there is enough room
    if (BUFFER_SIZE - DoubleBuffer_getIndex(m_tx_buffers) < length)
    {
        Sys_exitCriticalSection();
        return 0;
    }

    // Copy data to current buffer
    memcpy(DoubleBuffer_getActive(m_tx_buffers) +
           DoubleBuffer_getIndex(m_tx_buffers),
           buffer,
           length);

    DoubleBuffer_incrIndex(m_tx_buffers, length);

    start_tx_lock();

    Sys_exitCriticalSection();

    return length;
}

void Usart_enableReceiver(serial_rx_callback_f rx_callback)
{
    Sys_enterCriticalSection();
    /* Set callback */
    m_rx_callback = rx_callback;
    if (m_rx_callback)
    {
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
    while(m_tx_ongoing && timeout > 0)
    {
        timeout--;
    }
}

/**
 * \brief Function for handling the USART Interrupt for TX End
 */
void __attribute__((__interrupt__)) UARTE0_IRQHandler(void)
{
    /* TX transfer complete, start next */
    if (NRF_UARTE0->EVENTS_ENDTX != 0)
    {
        m_tx_ongoing = false;
        NRF_UARTE0->EVENTS_ENDTX = 0;
        NRF_UARTE0->TASKS_STOPTX = 1;

        // Retry to send in case something was queued while transmitting
        start_tx_lock();
    }

    /* Handle errors: Nothing to do specific at the moment */
    if (NRF_UARTE0->EVENTS_ERROR != 0)
    {
        NRF_UARTE0->EVENTS_ERROR = 0;
    }

    // read any event from peripheral to flush the write buffer:
    EVENT_READBACK = NRF_UARTE0->EVENTS_RXDRDY;
}

void __attribute__((__interrupt__)) TIMER1_IRQHandler(void)
{
    uint8_t index;
    // Handle reception
    if (NRF_TIMER1->EVENTS_COMPARE[0] != 0)
    {
        NRF_TIMER1->EVENTS_COMPARE[0] = 0;

        // Read current index in RX buffer reflected in Timer2 counter
        NRF_TIMER2->TASKS_CAPTURE[0] = 1;
        index = NRF_TIMER2->CC[0] & 0xff;

        if (m_last_rx_index != index)
        {
            // Time to send received byte to upper level
            if(m_rx_callback != NULL)
            {
                // Check for a wrap of counter
                if (index < m_last_rx_index)
                {
                    // Send end of the buffer
                    m_rx_callback(m_rx_circular_buffer + m_last_rx_index,
                                  RX_BUFFER_SIZE - m_last_rx_index);
                    m_last_rx_index = 0;
                }

                m_rx_callback(m_rx_circular_buffer + m_last_rx_index,
                              index - m_last_rx_index);
                m_last_rx_index = index;
            }
        }
        // Do not clear the TIMER1, it will be cleared on next reception by PPI
    }

    // read any event from peripheral to flush the write buffer:
    EVENT_READBACK = NRF_TIMER1->EVENTS_COMPARE[0];
}

static void start_tx_lock(void)
{
    if (m_tx_ongoing)
    {
        // Already tx ongoing, this buffer will be chained later
        // as this function will be called again after current transfert
        return;
    }

    if (DoubleBuffer_getIndex(m_tx_buffers) == 0)
    {
        // Nothing to send
        return;
    }

    // No tx ongoing and something to send
    m_tx_ongoing = true;

    Usart_setEnabled(true);

    // Start DMA
    NRF_UARTE0->TXD.PTR = (uint32_t) DoubleBuffer_getActive(m_tx_buffers);
    NRF_UARTE0->TXD.MAXCNT = DoubleBuffer_getIndex(m_tx_buffers);

    NRF_UARTE0->EVENTS_ENDTX = 0;
    NRF_UARTE0->TASKS_STARTTX = 1;

    // Swipe buffers (it automatically reset writing index)
    DoubleBuffer_swipe(m_tx_buffers);
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

        NRF_UARTE0->PSEL.RTS = 0xFFFFFFFF;
        NRF_UARTE0->PSEL.CTS = BOARD_USART_CTS_PIN;
        /* No parity, HW flow control */
        NRF_UARTE0->CONFIG = UARTE_CONFIG_HWFC_Enabled << UARTE_CONFIG_HWFC_Pos;
    }
    else
    {
        NRF_UARTE0->PSEL.RTS = 0xFFFFFFFF;
        NRF_UARTE0->PSEL.CTS = 0xFFFFFFFF;
        /* No parity, no HW flow control */
        NRF_UARTE0->CONFIG = 0;

        // Deactivate CTS & RTS pins
        nrf_gpio_cfg_default(BOARD_USART_CTS_PIN);
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
        NRF_UARTE0->BAUDRATE = (uint32_t)UARTE_BAUDRATE_BAUDRATE_Baud115200;
        break;
    case 125000:
        // This value is not from official nrf9160_bitfields.h
        NRF_UARTE0->BAUDRATE = (uint32_t)(0x02000000UL);
        break;
    case 250000:
        NRF_UARTE0->BAUDRATE = (uint32_t)UARTE_BAUDRATE_BAUDRATE_Baud250000;
        break;
    case 460800:
        NRF_UARTE0->BAUDRATE = (uint32_t)UARTE_BAUDRATE_BAUDRATE_Baud460800;
        break;
    case 1000000:
        NRF_UARTE0->BAUDRATE = (uint32_t)UARTE_BAUDRATE_BAUDRATE_Baud1M;
        break;
    default:
        // Intended baudrate is not in the list, default baudrate from chip will be used
        ret = false;
        break;
    }

    return ret;
}

static void init_dma(uint32_t baudrate)
{
    /* Interrupt init */
    NRF_UARTE0->INTENCLR = 0xffffffffUL;
    NRF_UARTE0->INTENSET =
        (UARTE_INTEN_ENDTX_Enabled << UARTE_INTEN_ENDTX_Pos) |
        (UARTE_INTEN_ERROR_Enabled << UARTE_INTEN_ERROR_Pos);

    /* Configure RX part */
    /* RX works as followed:
     * - When RX transfer is started, the DMA transfer is ongoing all
     *   the time and fill the 255 circular bytes buffers. It is in fact,
     *   a 255 bytes transfers that is automatically restarted every time
     *   the transfer is ended
     *     => ENDRX / STARTRX shorts is enabled
     * - A timer in counter mode is incremented on each received bytes and
     *   reseted each time the new transfer is started. It allows the
     *   software to know the last written index inside buffer
     * - A second timer is configured to wake-up software each time there
     *   is no activity on the RX line for 5 bytes length (at configured baud)
     *   It is then time to send received bytes to upper level.
     *
     * - It works as long as maximum packet size sent from other side
     *   is smaller than 255 (it is the case for us). But it could be improved
     *   also to allow infinite size packets by just waking up the software
     *   when enough bytes are received in circular buffer before being erased.
     *   But this improvement is not need at the moment.
     */

    /* Add shortcut ENDRX->STARTRX */
    NRF_UARTE0->SHORTS =
        UARTE_SHORTS_ENDRX_STARTRX_Enabled << UARTE_SHORTS_ENDRX_STARTRX_Pos;

    /* Configure TIMER 2 to count the number of byte received by dma
     * Timer 2 is automatically incremented by PPI on byte reception
     * and is reseted each time the RX buffer wraps (every 255 bytes)
     */
    NRF_TIMER2->MODE =
        TIMER_MODE_MODE_LowPowerCounter << TIMER_MODE_MODE_Pos;
    NRF_TIMER2->BITMODE =
        TIMER_BITMODE_BITMODE_16Bit << TIMER_BITMODE_BITMODE_Pos;
    NRF_TIMER2->TASKS_CLEAR = 1;
    NRF_TIMER2->TASKS_STOP = 1;


    /* Configure TIMER 1 to monitor timeout on RX line and be notified
     * for end of message
     */

    /* Counter mode at 1MHz 1 tick every 1us */
    NRF_TIMER1->MODE = TIMER_MODE_MODE_Timer;
    NRF_TIMER1->BITMODE = TIMER_BITMODE_BITMODE_32Bit;
    NRF_TIMER1->PRESCALER = 4; // So 2^4 = 16 => 1Mhz
    NRF_TIMER1->INTENSET =
        TIMER_INTENSET_COMPARE0_Enabled << TIMER_INTENSET_COMPARE0_Pos;

    /* Compute timeout in us for length of N consecutive bytes on RX line */
    uint32_t timeout_us = TIMEOUT_CHAR_N * (10 * (1000 * 1000) / (baudrate));

    NRF_TIMER1->CC[0] = timeout_us;
    NRF_TIMER1->TASKS_STOP = 1;
    NRF_TIMER1->TASKS_CLEAR = 1;

    /* Configure PPI: 3 channels used, configured in a group */

    /* Create group */
    NRF_DPPIC->CHG[0] = (DPPIC_CHG_CH3_Included << DPPIC_CHG_CH3_Pos) |
                  (DPPIC_CHG_CH4_Included << DPPIC_CHG_CH4_Pos) |
                  (DPPIC_CHG_CH5_Included << DPPIC_CHG_CH5_Pos);

    /* Start Timer 1 when RX is started. Only used one time when starting RX */
    //NRF_PPI->CH[3].EEP = (uint32_t)&NRF_UARTE0->EVENTS_RXSTARTED;
    //NRF_PPI->CH[3].TEP = (uint32_t)&NRF_TIMER1->TASKS_START;
    NRF_UARTE0->PUBLISH_RXSTARTED = 3 << UARTE_PUBLISH_RXSTARTED_CHIDX_Pos |
                                    UARTE_PUBLISH_RXSTARTED_EN_Msk;

    NRF_TIMER1->SUBSCRIBE_START =   3 << TIMER_SUBSCRIBE_START_CHIDX_Pos |
                                    TIMER_SUBSCRIBE_START_EN_Msk;

    /* Reset timer 1, each time a byte is received to avoid Timeout */
    /* Count the number of bytes received with Timer2 in count mode */
    NRF_UARTE0->PUBLISH_RXDRDY  = 4 << UARTE_PUBLISH_RXDRDY_CHIDX_Pos   |
                                    UARTE_PUBLISH_RXDRDY_EN_Msk;

    NRF_TIMER1->SUBSCRIBE_CLEAR = 4 << TIMER_SUBSCRIBE_CLEAR_CHIDX_Pos  |
                                    TIMER_SUBSCRIBE_CLEAR_EN_Msk;

    NRF_TIMER2->SUBSCRIBE_COUNT = 4 << TIMER_SUBSCRIBE_COUNT_CHIDX_Pos  |
                                    TIMER_SUBSCRIBE_COUNT_EN_Msk;


    /* Clear the Timer2 when ENDRX happens, ie buffer wrap*/
    NRF_UARTE0->PUBLISH_ENDRX = 5 << UARTE_PUBLISH_ENDRX_CHIDX_Pos |
                                UARTE_PUBLISH_ENDRX_EN_Msk;

    NRF_TIMER2->SUBSCRIBE_CLEAR = 5 << TIMER_SUBSCRIBE_CLEAR_CHIDX_Pos |
                                  TIMER_SUBSCRIBE_CLEAR_EN_Msk;

    /* Configure TX part */
    DoubleBuffer_init(m_tx_buffers);
}
