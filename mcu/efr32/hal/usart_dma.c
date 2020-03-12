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

/* Only one USART, this is easy */
static volatile serial_rx_callback_f    m_rx_callback;

/* Map some registers constant to the USART selected */
#if BOARD_USART_ID == 0
#define BOARD_USART         USART0
#define LDMA_REQ_SOURCESEL  LDMA_CH_REQSEL_SOURCESEL_USART0
#define LDMA_REQ_SIGSEL_TX  LDMA_CH_REQSEL_SIGSEL_USART0TXEMPTY
#define LDMA_REQ_SIGSEL_RX  LDMA_CH_REQSEL_SIGSEL_USART0RXDATAV
#define BOARD_UART_RX_IRQn  USART0_RX_IRQn
#define BOARD_UART_TX_IRQn  USART0_TX_IRQn
#elif BOARD_USART_ID == 1
#define BOARD_USART         USART1
#define LDMA_REQ_SOURCESEL  LDMA_CH_REQSEL_SOURCESEL_USART1
#define LDMA_REQ_SIGSEL_TX  LDMA_CH_REQSEL_SIGSEL_USART1TXEMPTY
#define LDMA_REQ_SIGSEL_RX  LDMA_CH_REQSEL_SIGSEL_USART1RXDATAV
#define BOARD_UART_RX_IRQn  USART1_RX_IRQn
#define BOARD_UART_TX_IRQn  USART1_TX_IRQn
#elif BOARD_USART_ID == 2
#define BOARD_USART         USART2
#define LDMA_REQ_SOURCESEL  LDMA_CH_REQSEL_SOURCESEL_USART2
#define LDMA_REQ_SIGSEL_TX  LDMA_CH_REQSEL_SIGSEL_USART2TXEMPTY
#define LDMA_REQ_SIGSEL_RX  LDMA_CH_REQSEL_SIGSEL_USART2RXDATAV
#define BOARD_UART_RX_IRQn  USART2_RX_IRQn
#define BOARD_UART_TX_IRQn  USART2_TX_IRQn
#elif BOARD_USART_ID == 3
#define BOARD_USART         USART3
#define LDMA_REQ_SOURCESEL  LDMA_CH_REQSEL_SOURCESEL_USART3
#define LDMA_REQ_SIGSEL_TX  LDMA_CH_REQSEL_SIGSEL_USART3TXEMPTY
#define LDMA_REQ_SIGSEL_RX  LDMA_CH_REQSEL_SIGSEL_USART3RXDATAV
#define BOARD_UART_RX_IRQn  USART3_RX_IRQn
#define BOARD_UART_TX_IRQn  USART3_TX_IRQn
#else
#error USART ID must be 0, 1, 2 or 3
#endif

/** DMA channel config. RX must have a higher priority (lower value) */
#define DMA_RX_CH   1
#define DMA_TX_CH   2

/** Declare buffer size (defined by max DMA transfert size) */
#define BUFFER_SIZE                     256u
#include "doublebuffer.h"

/** Timeout without activity on the line to define the end of frame */
#define TIMEOUT_INACTIVITY_RX_BAUD_TIME  20 //< Unit is in baud time

/** Indicate if USART is enabled */
static volatile uint32_t                m_enabled;

/** Indicate if RX is enabled */
static bool                             m_rx_enabled;

/** Indicate if at least one Tw was started */
static bool                             m_tx_started;

/** Enable or disable HW flow control */
static void set_baud(uint32_t baud);

/** Declare the interrupt handlers */
void __attribute__((__interrupt__))     USART_RX_IRQHandler(void);
void __attribute__((__interrupt__))     LDMA_IRQHandler(void);

/** Internal function to start a RX session on dma */
static void start_dma_rx_lock(void);

/** Internal function to wait for end of latest Tx transfer */
static void wait_end_of_tx(void);

/* Is tx_ongoing ? */
static volatile bool m_tx_ongoing;

/* Buffers used for transmission */
static double_buffer_t m_tx_buffers;

/* Buffer used for reception */
static double_buffer_t m_rx_buffers;

/* DMA descriptor used for reception */
static DMA_DESCRIPTOR_TypeDef m_rx_dma_descriptor[2];

/* DMA descriptor used for transmission */
static DMA_DESCRIPTOR_TypeDef m_tx_dma_descriptor[3];

/**
 * \brief   Enable the DMA clk
 * \param   enable
 *          True to enable, false otherwise
 * TODO: Should be move to hfperclk with a different file name
 */
static void enableDMACLK(bool enable)
{
    if(enable)
    {
        CMU->HFBUSCLKEN0 |= CMU_HFBUSCLKEN0_LDMA;
    }
    else
    {
        CMU->HFBUSCLKEN0 &= ~(CMU_HFBUSCLKEN0_LDMA);
    }
}

/**
 * \brief   Enable the GPIO clk
 * \param   enable
 *          True to enable, false otherwise
 * TODO: Should be move to hfperclk with a different file name
 */
static void enableGPIOCLK(bool enable)
{
    if(enable)
    {
        CMU->HFBUSCLKEN0 |= CMU_HFBUSCLKEN0_GPIO;
    }
    else
    {
        CMU->HFBUSCLKEN0 &= ~(CMU_HFBUSCLKEN0_GPIO);
    }
}

void Usart_init(uint32_t baudrate, uart_flow_control_e flow_control)
{
#ifdef BOARD_USART_FORCE_BAUDRATE
    // Some hardware only support a given speed, so override the chosen baudrate
    baudrate = BOARD_USART_FORCE_BAUDRATE;
#endif
    (void)flow_control;

    // Enable GPIO clock
    enableGPIOCLK(true);

    //uart_tx_pin
    hal_gpio_set_mode(BOARD_USART_GPIO_PORT,
                      BOARD_USART_TX_PIN,
                      GPIO_MODE_DISABLED);
    hal_gpio_clear(BOARD_USART_GPIO_PORT,
                   BOARD_USART_TX_PIN);
    //uart_rx_pin
    hal_gpio_set_mode(BOARD_USART_GPIO_PORT,
                      BOARD_USART_RX_PIN,
                      GPIO_MODE_DISABLED);
    hal_gpio_clear(BOARD_USART_GPIO_PORT,
                   BOARD_USART_RX_PIN);

    /* Module variables */
    m_rx_callback = NULL;
    m_enabled = 0;
    m_rx_enabled = false;

    /* Disable RX interrupt */
    Sys_disableAppIrq(BOARD_UART_RX_IRQn);
    Sys_clearFastAppIrq(BOARD_UART_RX_IRQn);

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

    BOARD_USART->I2SCTRL = 0;
    /* Disables PRS */
    BOARD_USART->INPUT = 0;
    /* Set frame params: 8bit, nopar, 1stop */
    BOARD_USART->FRAME = USART_FRAME_DATABITS_EIGHT |
                         USART_FRAME_PARITY_NONE |
                         USART_FRAME_STOPBITS_ONE;
    set_baud(baudrate);

    /* Setup the UART Rx timeout
     * Timeout is in baud time */
    BOARD_USART->TIMECMP1 =
        USART_TIMECMP1_TSTOP_RXACT |
        USART_TIMECMP1_TSTART_RXEOF |
        ((TIMEOUT_INACTIVITY_RX_BAUD_TIME & _USART_TIMECMP1_TCMPVAL_MASK)
            << _USART_TIMECMP1_TCMPVAL_SHIFT);

    /* Disable all interrupt sources */
    BOARD_USART->IEN = 0;
    /* Clear all irq flags */
    BOARD_USART->IFC = _USART_IFC_MASK;
    /* Enable transmitter */
    BOARD_USART->CMD = USART_CMD_TXEN;

    // Configuration done: disable clock
    enableHFPERCLK(false);

    /* APP IRQ */
    Sys_clearFastAppIrq(LDMA_IRQn);
    Sys_enableFastAppIrq(LDMA_IRQn,
                         APP_LIB_SYSTEM_IRQ_PRIO_HI,
                         LDMA_IRQHandler);

    // Prepare TX buffer
    m_tx_ongoing = false;
    m_tx_started = false;
    DoubleBuffer_init(m_tx_buffers);

    // Prepare RX buffer
    DoubleBuffer_init(m_rx_buffers);
}

void Usart_setEnabled(bool enabled)
{
    Sys_enterCriticalSection();
    if(enabled)
    {
        /* Detect if someone is enabling UART but not disabling it ever */
        if(m_enabled == 0)
        {
            // Enable clocks
            enableHFPERCLK(true);
            enableDMACLK(true);

            // Disable deep sleep
            DS_Disable(DS_SOURCE_USART);
            // Set output
            hal_gpio_set_mode(BOARD_USART_GPIO_PORT,
                              BOARD_USART_TX_PIN,
                              GPIO_MODE_OUT_PP);
        }
        m_enabled++;
    }
    else if (m_enabled > 0)
    {
        /* Usart was enabled at least one time */
        m_enabled--;

        if(m_enabled == 0)
        {
            // In case a Tx was started, wait for end of transfer
            wait_end_of_tx();

            // Set light pullup
            hal_gpio_set_mode(BOARD_USART_GPIO_PORT,
                              BOARD_USART_TX_PIN,
                              GPIO_MODE_IN_PULL);
            hal_gpio_set(BOARD_USART_GPIO_PORT,
                         BOARD_USART_TX_PIN);
            // Enable deep sleep
            DS_Enable(DS_SOURCE_USART);
            // Disable clocks
            enableHFPERCLK(false);
            enableDMACLK(false);
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

    // Clear RX timeout
    BOARD_USART->IFC = USART_IFC_TCMP1;

    // Prepare RX buffer
    DoubleBuffer_init(m_rx_buffers);

    start_dma_rx_lock();

    BITBAND_Peripheral(&(BOARD_USART->CMD), _USART_CMD_RXEN_SHIFT, 1);

    Sys_exitCriticalSection();
}

void Usart_receiverOff(void)
{
    Sys_enterCriticalSection();

    // Stop Current DMA transfer
    BITBAND_Peripheral(&(LDMA->CHEN), DMA_RX_CH, 0);
    BITBAND_Peripheral(&(BOARD_USART->CMD), _USART_CMD_RXDIS_SHIFT, 1);

    m_rx_enabled = false;
    Sys_exitCriticalSection();
}

bool Usart_setFlowControl(uart_flow_control_e flow)
{
    (void)flow;
    return false;
}

static void start_dma_tx_lock(uint8_t size, void * src)
{
    /* DMA transmission happens with 3 transfers
     *  - First one is used for initial transfer with absolute source address
     *  - Second one is a loop which increment by one the source for each transfer
     *  - Third one is last byte that will generate an event when transfer is done
     */
    m_tx_dma_descriptor[0].CTRL = LDMA_CH_CTRL_DSTMODE_ABSOLUTE |
                               LDMA_CH_CTRL_SRCMODE_ABSOLUTE |
                               LDMA_CH_CTRL_DSTINC_NONE | // Destination is fifo so no increment
                               LDMA_CH_CTRL_SIZE_BYTE | // Fifo is fill with one byte
                               LDMA_CH_CTRL_SRCINC_ONE | // Source is RAM so increment by one
                               LDMA_CH_CTRL_IGNORESREQ | // As TX and RX setup, must be one according to RM in UART chapter
                               LDMA_CH_CTRL_DECLOOPCNT | // Dec for loop count
                               LDMA_CH_CTRL_REQMODE_ALL | // Transfer byte by byte as specified in XFERCNT
                               LDMA_CH_CTRL_BLOCKSIZE_ALL | // Transfer all in arbitration
                               (_LDMA_CH_CTRL_XFERCNT_MASK & (0 << _LDMA_CH_CTRL_XFERCNT_SHIFT)) |
                               LDMA_CH_CTRL_STRUCTTYPE_TRANSFER; // Transfer type

    m_tx_dma_descriptor[0].SRC = src;
    m_tx_dma_descriptor[0].DST = &(BOARD_USART->TXDATA);
    m_tx_dma_descriptor[0].LINK = (void *) &m_tx_dma_descriptor[1]; // Link to next transfer

    // Setup the second loop transfer
    m_tx_dma_descriptor[1].CTRL = LDMA_CH_CTRL_DSTMODE_ABSOLUTE |
                               LDMA_CH_CTRL_SRCMODE_RELATIVE |
                               LDMA_CH_CTRL_DSTINC_NONE | // Destination is fifo so no increment
                               LDMA_CH_CTRL_SIZE_BYTE | // Fifo is fill with one byte
                               LDMA_CH_CTRL_SRCINC_ONE | // Source is RAM so increment by one
                               LDMA_CH_CTRL_IGNORESREQ | // As TX and RX setup, must be one according to RM in UART chapter
                               LDMA_CH_CTRL_DECLOOPCNT | // Dec for loop count
                               LDMA_CH_CTRL_REQMODE_ALL | // Transfer byte by byte as specified in XFERCNT
                               LDMA_CH_CTRL_BLOCKSIZE_ALL | // Transfer all in arbitration
                               (_LDMA_CH_CTRL_XFERCNT_MASK & (0 << _LDMA_CH_CTRL_XFERCNT_SHIFT)) |
                               LDMA_CH_CTRL_STRUCTTYPE_TRANSFER; // Transfer type

    m_tx_dma_descriptor[1].SRC = 0; // Relative to previous transfer
    m_tx_dma_descriptor[1].DST = &(BOARD_USART->TXDATA);
    m_tx_dma_descriptor[1].LINK = (void *) LDMA_CH_LINK_LINKMODE_RELATIVE; // Point to ourself

    // Setup last transfer that will automatically be executed at end of previous loop as continuous in memory
    m_tx_dma_descriptor[2].CTRL = LDMA_CH_CTRL_DSTMODE_ABSOLUTE |
                                   LDMA_CH_CTRL_SRCMODE_RELATIVE |
                                   LDMA_CH_CTRL_DSTINC_NONE | // Destination is fifo so no increment
                                   LDMA_CH_CTRL_SIZE_BYTE | // Fifo is fill with one byte
                                   LDMA_CH_CTRL_SRCINC_ONE | // Source is RAM so increment by one
                                   LDMA_CH_CTRL_IGNORESREQ | // As TX and RX setup, must be one according to RM in UART chapter
                                   LDMA_CH_CTRL_REQMODE_ALL | // Transfer byte by byte as specified in XFERCNT
                                   LDMA_CH_CTRL_DONEIFSEN | // Interrupt at end of transfer
                                   LDMA_CH_CTRL_BLOCKSIZE_ALL | // Transfer all in arbitration
                                   (_LDMA_CH_CTRL_XFERCNT_MASK & (0 << _LDMA_CH_CTRL_XFERCNT_SHIFT)) |
                                   LDMA_CH_CTRL_STRUCTTYPE_TRANSFER; // Transfer type

    m_tx_dma_descriptor[2].SRC = 0;  // Relative to previous transfer
    m_tx_dma_descriptor[2].DST = &(BOARD_USART->TXDATA);
    m_tx_dma_descriptor[2].LINK = 0;

    // Loop until last byte, and then execute last transfer that will generate IRQ
    LDMA->CH[DMA_TX_CH].LOOP = size - 1;

    // Trigger when USART TX is empty
    LDMA->CH[DMA_TX_CH].REQSEL = LDMA_REQ_SOURCESEL |
                                 LDMA_REQ_SIGSEL_TX;
    // Positive sign, no round robin
    LDMA->CH[DMA_TX_CH].CFG = _LDMA_CH_CFG_RESETVALUE;

    // Enable DONE interrupt for TX channel
    LDMA->IEN = LDMA->IEN | (1 << DMA_TX_CH);

    LDMA->CH[DMA_TX_CH].LINK = (uint32_t) &m_tx_dma_descriptor;

    // Start transfer by loading initial transfer
    LDMA->LINKLOAD = 1 << DMA_TX_CH;
}

/**
 * \brief   Called each time a transfer is ready or when a previous transfer was finished
 * \note    This function must be called with interrupts disable or from interrupt context
 */
static void start_tx_lock()
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
    // Enable again the UART to avoid being disabled while sending
    Usart_setEnabled(true);

    start_dma_tx_lock(DoubleBuffer_getIndex(m_tx_buffers),
                      DoubleBuffer_getActive(m_tx_buffers));

    // Swipe buffers (it automatically reset writing index)
    DoubleBuffer_swipe(m_tx_buffers);
}

uint32_t Usart_sendBuffer(const void * buffer, uint32_t length)
{
    Sys_enterCriticalSection();

    // At least one tx transfer started
    m_tx_started = true;

    // Check if there is enough room
    if (BUFFER_SIZE - DoubleBuffer_getIndex(m_tx_buffers) < length)
    {
        Sys_exitCriticalSection();
        return 0;
    }

    // Copy data to current buffer
    memcpy(DoubleBuffer_getActive(m_tx_buffers) + DoubleBuffer_getIndex(m_tx_buffers),
           buffer,
           length);

    DoubleBuffer_incrIndex(m_tx_buffers, length);

    // Start the transfer
    start_tx_lock();

    Sys_exitCriticalSection();

    return length;
}

static void start_dma_rx_lock(void)
{
    // Setup the LDMA descriptor
    m_rx_dma_descriptor[0].CTRL =  LDMA_CH_CTRL_DSTMODE_ABSOLUTE |
                                   LDMA_CH_CTRL_SRCMODE_ABSOLUTE |
                                   LDMA_CH_CTRL_DSTINC_ONE | // Destination is RAM so increment by one
                                   LDMA_CH_CTRL_SIZE_BYTE | // Fifo is fill with one byte
                                   LDMA_CH_CTRL_SRCINC_NONE | // Source is FIFO so no increment
                                   LDMA_CH_CTRL_IGNORESREQ | // As TX and RX setup, must be one according to RM in UART chapter
                                   LDMA_CH_CTRL_DECLOOPCNT | // Dec for loop count
                                   LDMA_CH_CTRL_REQMODE_ALL | // Transfer byte by byte as specified in XFERCNT
                                   LDMA_CH_CTRL_BLOCKSIZE_ALL | // Transfer all in arbitration
                                   (_LDMA_CH_CTRL_XFERCNT_MASK & (0 << _LDMA_CH_CTRL_XFERCNT_SHIFT)) |
                                   LDMA_CH_CTRL_STRUCTTYPE_TRANSFER; // Transfer type

    m_rx_dma_descriptor[0].SRC = (void *) &(BOARD_USART->RXDATA);
    m_rx_dma_descriptor[0].DST = DoubleBuffer_getActive(m_rx_buffers);
    m_rx_dma_descriptor[0].LINK = (void *) &m_rx_dma_descriptor[1];

    m_rx_dma_descriptor[1].CTRL = LDMA_CH_CTRL_DSTMODE_RELATIVE |
                                  LDMA_CH_CTRL_SRCMODE_ABSOLUTE |
                                  LDMA_CH_CTRL_DSTINC_ONE | // Destination is RAM so increment by one
                                  LDMA_CH_CTRL_SIZE_BYTE | // Fifo is fill with one byte
                                  LDMA_CH_CTRL_SRCINC_NONE | // Source is FIFO so no increment
                                  LDMA_CH_CTRL_IGNORESREQ | // As TX and RX setup, must be one according to RM in UART chapter
                                  LDMA_CH_CTRL_DECLOOPCNT | // Dec for loop count
                                  LDMA_CH_CTRL_REQMODE_ALL | // Transfer byte by byte as specified in XFERCNT
                                  LDMA_CH_CTRL_BLOCKSIZE_ALL | // Transfer all in arbitration
                                  (_LDMA_CH_CTRL_XFERCNT_MASK & (0 << _LDMA_CH_CTRL_XFERCNT_SHIFT)) |
                                  LDMA_CH_CTRL_STRUCTTYPE_TRANSFER; // Transfer type

    m_rx_dma_descriptor[1].SRC = (void *) &(BOARD_USART->RXDATA);
    m_rx_dma_descriptor[1].DST = 0;
    m_rx_dma_descriptor[1].LINK = (void *) LDMA_CH_LINK_LINKMODE_RELATIVE; // Point to ourself

    // Loop as much as we can, it will be stopped by uart timeout (255 max at the moment)
    LDMA->CH[DMA_RX_CH].LOOP = 0xff;

    // Trigger when USART0 RX is not empty
    LDMA->CH[DMA_RX_CH].REQSEL = LDMA_REQ_SOURCESEL |
                                 LDMA_REQ_SIGSEL_RX;
    // Positive sign, no round robin
    LDMA->CH[DMA_RX_CH].CFG = _LDMA_CH_CFG_RESETVALUE;

    // Enable DONE interrupt for RX channel
    // (Not needed if maximum RX size is lower than 255)
    LDMA->IEN = LDMA->IEN | (1 << DMA_RX_CH);

    LDMA->CH[DMA_RX_CH].LINK = (uint32_t) &m_rx_dma_descriptor;

    // Start transfer by loading initial transfer
    LDMA->LINKLOAD = 1 << DMA_RX_CH;
}

void Usart_enableReceiver(serial_rx_callback_f rx_callback)
{
    Sys_enterCriticalSection();
    /* Set callback */
    m_rx_callback = rx_callback;
    // Enable clock
    enableHFPERCLK(true);
    if(rx_callback)
    {
        // Enable the RX interrupt
        Sys_enableFastAppIrq(BOARD_UART_RX_IRQn,
                             APP_LIB_SYSTEM_IRQ_PRIO_HI,
                             USART_RX_IRQHandler);
        // Enable the correct interrupt
        BITBAND_Peripheral(&(BOARD_USART->IEN), _USART_IEN_TCMP1_SHIFT, 1);

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
        BITBAND_Peripheral(&(BOARD_USART->IEN), _USART_IEN_TCMP1_SHIFT, 0);
        hal_gpio_set_mode(BOARD_USART_GPIO_PORT,
                          BOARD_USART_RX_PIN,
                          GPIO_MODE_DISABLED);
        // Disable pull-up for disabled GPIO:s
        hal_gpio_clear(BOARD_USART_GPIO_PORT,
                       BOARD_USART_RX_PIN);
    }

    /* Clear RX timeout */
    BOARD_USART->IFC = USART_IFC_TCMP1;
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
    // Wait for end of dma transfer
    while(m_tx_ongoing)
    {
    }

    // Dma transfer is finished but UART Tx fifo
    // may still contain bytes
    wait_end_of_tx();
}

void __attribute__((__interrupt__)) USART_RX_IRQHandler(void)
{
    /* RX Timeout */
    if (BOARD_USART->IF && USART_IF_TCMP1)
    {
        BOARD_USART->IFC = USART_IFC_TCMP1;
        if(m_rx_callback != NULL)
        {
            uint8_t * end_buffer = (uint8_t *) LDMA->CH[DMA_RX_CH].DST;
            uint8_t * old_buffer = DoubleBuffer_getActive(m_rx_buffers);
            // Stop RX DMA
            BITBAND_Peripheral(&(LDMA->CHEN), DMA_RX_CH, 0);

            // Swipe RX buffers
            DoubleBuffer_swipe(m_rx_buffers);

            // Restart DMA
            start_dma_rx_lock();

            // Send bytes to upper level
            m_rx_callback(old_buffer, end_buffer-old_buffer);
        }
    }
}

void __attribute__((__interrupt__)) LDMA_IRQHandler(void)
{
    // Check if DMA_TX_CHANNEL is done
    if (LDMA->IF & (1 << DMA_TX_CH))
    {
        LDMA->IFC = 1 << DMA_TX_CH;
        BITBAND_Peripheral(&(LDMA->CHEN), 1 << DMA_TX_CH, 0);
        BITBAND_Peripheral(&(LDMA->CHDONE), 1 << DMA_TX_CH, 0);
        // TX channel done
        m_tx_ongoing = false;
        Usart_setEnabled(false);

        // Retry to send in case something was queued while transmitting
        start_tx_lock();
    }
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

static void wait_end_of_tx(void)
{
    if (!m_tx_started)
    {
        // No tx transfer started
        return;
    }

    // The following test can only be done if at least one transfer
    // was started as TXC field initial value is 0 and set to
    // 1 for any end of TX transfer. So it will be an infinite
    // loop if no transfer started as it will never be set to 1.
    while((BOARD_USART->STATUS & USART_STATUS_TXC)
          != USART_STATUS_TXC)
    {
    }

}

