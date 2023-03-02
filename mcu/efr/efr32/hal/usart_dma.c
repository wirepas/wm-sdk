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
#include "em_ldma.h"

static volatile serial_rx_callback_f    m_rx_callback;

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

/** Declare the interrupt handlers */
void __attribute__((__interrupt__))     USART_RX_IRQHandler(void);
void __attribute__((__interrupt__))     LDMA_IRQHandler(void);

/** Internal function to start a RX session on dma */
static void start_dma_rx_lock(void);

/** Internal function to wait for end of latest Tx transfer */
static void wait_end_of_tx(void);

// Is tx_ongoing ?
static volatile bool m_tx_ongoing;

// Buffers used for transmission
static double_buffer_t m_tx_buffers;

// Buffer used for reception
static double_buffer_t m_rx_buffers;

// DMA descriptor used for reception
static LDMA_Descriptor_t m_rx_dma_descriptor[2];

// DMA descriptor used for transmission
static LDMA_Descriptor_t m_tx_dma_descriptor[3];

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
    m_rx_callback = NULL;
    m_enabled = 0;
    m_rx_enabled = false;

    // Disable RX interrupt
    Sys_disableAppIrq(BOARD_UART_RX_IRQn);
    Sys_clearFastAppIrq(BOARD_UART_RX_IRQn);

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

    /* Setup the UART Rx timeout
     * Timeout is in baud time */
    BOARD_USART->TIMECMP1 =
        USART_TIMECMP1_TSTOP_RXACT |
        USART_TIMECMP1_TSTART_RXEOF |
        ((TIMEOUT_INACTIVITY_RX_BAUD_TIME & _USART_TIMECMP1_TCMPVAL_MASK)
            << _USART_TIMECMP1_TCMPVAL_SHIFT);

    // Disable all interrupt sources
    BOARD_USART->IEN = 0;
    // Clear all irq flags
    BOARD_USART_CLR_IRQ_ALL;
    // Enable transmitter
    BOARD_USART->CMD = USART_CMD_TXEN;

    // Configuration done: disable clock
    BOARD_USART_DISABLE_USART_CLK;

    // APP IRQ
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
            // Enable clocks
            BOARD_USART_ENABLE_USART_CLK;
            BOARD_USART_ENABLE_LDMA_CLK;
            // Enable LDMA block
            BOARD_USART_LDMA_ENABLE;

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
        // Usart was enabled at least one time
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
            // Disable LDMA block
            BOARD_USART_LDMA_DISABLE;
            // Disable clocks
            BOARD_USART_DISABLE_LDMA_CLK;
            BOARD_USART_DISABLE_USART_CLK;
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
    BOARD_USART_CLR_IRQ_TCMP1;

    // Prepare RX buffer
    DoubleBuffer_init(m_rx_buffers);

    start_dma_rx_lock();

    BUS_RegBitWrite(&(BOARD_USART->CMD), _USART_CMD_RXEN_SHIFT, 1);

    Sys_exitCriticalSection();
}

void Usart_receiverOff(void)
{
    Sys_enterCriticalSection();

#if defined(EFR32MG22) || defined(EFR32FG23)
    // Stop Current DMA transfer
    if (CMU->CLKEN0 & CMU_CLKEN0_LDMA)
    {
        BUS_RegBitWrite(&(LDMA->CHEN), DMA_RX_CH, 0);
    }
    if (CMU->CLKEN0 & BOARD_USART_CMU_BIT)
    {
        BUS_RegBitWrite(&(BOARD_USART->CMD), _USART_CMD_RXDIS_SHIFT, 1);
    }
#else
    // Stop Current DMA transfer
    BUS_RegBitWrite(&(LDMA->CHEN), DMA_RX_CH, 0);
    BUS_RegBitWrite(&(BOARD_USART->CMD), _USART_CMD_RXDIS_SHIFT, 1);
#endif
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

    m_tx_dma_descriptor[0] = (LDMA_Descriptor_t){
        .xfer = {
            .structType   = ldmaCtrlStructTypeXfer,
            .structReq    = 0,
            .xferCnt      = 0, // Number of bytes to transfer - 1:
                               // transfer one byte.
            .byteSwap     = 0,
            .blockSize    = ldmaCtrlBlockSizeAll,
            .doneIfs      = 0,
            .reqMode      = ldmaCtrlReqModeAll,
            .decLoopCnt   = 1,
            .ignoreSrec   = 1,
            .srcInc       = ldmaCtrlSrcIncOne,
            .size         = ldmaCtrlSizeByte,
            .dstInc       = ldmaCtrlDstIncNone,
            .srcAddrMode  = ldmaCtrlSrcAddrModeAbs,
            .dstAddrMode  = ldmaCtrlDstAddrModeAbs,
            .srcAddr      = (uint32_t)src,
            .dstAddr      = (uint32_t)&(BOARD_USART->TXDATA),
            .linkMode     = ldmaLinkModeAbs,
            .link         = 0,
            .linkAddr     = ((uint32_t)&m_tx_dma_descriptor[1])>>2
        }
    };

    m_tx_dma_descriptor[1] = (LDMA_Descriptor_t){
        .xfer = {
            .structType   = ldmaCtrlStructTypeXfer,
            .structReq    = 0,
            .xferCnt      = 0, // Number of bytes to transfer - 1:
                               // transfer one byte.
            .byteSwap     = 0,
            .blockSize    = ldmaCtrlBlockSizeAll,
            .doneIfs      = 0,
            .reqMode      = ldmaCtrlReqModeAll,
            .decLoopCnt   = 1,
            .ignoreSrec   = 1,
            .srcInc       = ldmaCtrlSrcIncOne,
            .size         = ldmaCtrlSizeByte,
            .dstInc       = ldmaCtrlDstIncNone,
            .srcAddrMode  = ldmaCtrlSrcAddrModeRel,
            .dstAddrMode  = ldmaCtrlDstAddrModeAbs,
            .srcAddr      = 0, // Relative to previous transfer.
            .dstAddr      = (uint32_t)&(BOARD_USART->TXDATA),
            .linkMode     = ldmaLinkModeRel,
            .link         = 0,
            .linkAddr     = 0  // Point to ourself.
        }
    };

    m_tx_dma_descriptor[2] = (LDMA_Descriptor_t){
        .xfer = {
            .structType   = ldmaCtrlStructTypeXfer,
            .structReq    = 0,
            .xferCnt      = 0, // Number of bytes to transfer - 1:
                               // transfer one byte.
            .byteSwap     = 0,
            .blockSize    = ldmaCtrlBlockSizeAll,
            .doneIfs      = 1, // Interrupt when done
            .reqMode      = ldmaCtrlReqModeAll,
            .decLoopCnt   = 0,
            .ignoreSrec   = 1,
            .srcInc       = ldmaCtrlSrcIncOne,
            .size         = ldmaCtrlSizeByte,
            .dstInc       = ldmaCtrlDstIncNone,
            .srcAddrMode  = ldmaCtrlSrcAddrModeRel,
            .dstAddrMode  = ldmaCtrlDstAddrModeAbs,
            .srcAddr      = 0, // Relative to previous transfer.
            .dstAddr      = (uint32_t)&(BOARD_USART->TXDATA),
            .linkMode     = ldmaLinkModeRel,
            .link         = 0,
            .linkAddr     = 0  // Point to ourself
        }
    };

    LDMA_TransferCfg_t ldmaTXConfig =
        (LDMA_TransferCfg_t)LDMA_TRANSFER_CFG_PERIPHERAL_LOOP(
                                 BOARD_UART_LDMA_TX,
                                 size - 1);

    LDMA_StartTransfer(DMA_TX_CH,
                       &ldmaTXConfig,
                       &m_tx_dma_descriptor[0]);
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
    m_rx_dma_descriptor[0] = (LDMA_Descriptor_t){
        .xfer = {
            .structType   = ldmaCtrlStructTypeXfer,
            .structReq    = 0,
            .xferCnt      = 0, // Number of bytes to transfer - 1:
                               // transfer one byte.
            .byteSwap     = 0,
            .blockSize    = ldmaCtrlBlockSizeAll,
            .doneIfs      = 0,
            .reqMode      = ldmaCtrlReqModeAll,
            .decLoopCnt   = 1,
            .ignoreSrec   = 1,
            .srcInc       = ldmaCtrlSrcIncNone,
            .size         = ldmaCtrlSizeByte,
            .dstInc       = ldmaCtrlDstIncOne,
            .srcAddrMode  = ldmaCtrlSrcAddrModeAbs,
            .dstAddrMode  = ldmaCtrlDstAddrModeAbs,
            .srcAddr      = (uint32_t)&(BOARD_USART->RXDATA),
            .dstAddr      = (uint32_t)DoubleBuffer_getActive(m_rx_buffers),
            .linkMode     = ldmaLinkModeAbs,
            .link         = 0,
            .linkAddr     = ((uint32_t)&m_rx_dma_descriptor[1])>>2
        }
    };

    m_rx_dma_descriptor[1] = (LDMA_Descriptor_t){
        .xfer = {
            .structType   = ldmaCtrlStructTypeXfer,
            .structReq    = 0,
            .xferCnt      = 0, // Number of bytes to transfer - 1:
                               // transfer one byte.
            .byteSwap     = 0,
            .blockSize    = ldmaCtrlBlockSizeAll,
            .doneIfs      = 0,
            .reqMode      = ldmaCtrlReqModeAll,
            .decLoopCnt   = 1,
            .ignoreSrec   = 1,
            .srcInc       = ldmaCtrlSrcIncNone,
            .size         = ldmaCtrlSizeByte,
            .dstInc       = ldmaCtrlDstIncOne,
            .srcAddrMode  = ldmaCtrlSrcAddrModeAbs,
            .dstAddrMode  = ldmaCtrlDstAddrModeRel,
            .srcAddr      = (uint32_t)&(BOARD_USART->RXDATA),
            .dstAddr      = 0,
            .linkMode     = ldmaLinkModeRel,
            .link         = 0,
            .linkAddr     = 0 // Point to ourself
        }
    };

    // Loop as much as we can, it will be stopped by uart timeout
    // (255 max at the moment).
    LDMA_TransferCfg_t ldmaRXConfig =
        (LDMA_TransferCfg_t)LDMA_TRANSFER_CFG_PERIPHERAL_LOOP(
                                 BOARD_UART_LDMA_RX,
                                 0xff);

    LDMA_StartTransfer(DMA_RX_CH,
                       &ldmaRXConfig,
                       &m_rx_dma_descriptor[0]);
}

void Usart_enableReceiver(serial_rx_callback_f rx_callback)
{
    Sys_enterCriticalSection();
    // Set callback
    m_rx_callback = rx_callback;
    // Enable clock
    BOARD_USART_ENABLE_USART_CLK;
    if(rx_callback)
    {
        // Enable the RX interrupt
        Sys_enableFastAppIrq(BOARD_UART_RX_IRQn,
                             APP_LIB_SYSTEM_IRQ_PRIO_HI,
                             USART_RX_IRQHandler);
        // Enable the correct interrupt
        BUS_RegBitWrite(&(BOARD_USART->IEN), _USART_IEN_TCMP1_SHIFT, 1);

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
        BUS_RegBitWrite(&(BOARD_USART->IEN), _USART_IEN_TCMP1_SHIFT, 0);
        hal_gpio_set_mode(BOARD_USART_GPIO_PORT,
                          BOARD_USART_RX_PIN,
                          GPIO_MODE_DISABLED);
        // Disable pull-up for disabled GPIO:s
        hal_gpio_clear(BOARD_USART_GPIO_PORT,
                       BOARD_USART_RX_PIN);
    }

    // Clear RX timeout
    BOARD_USART_CLR_IRQ_TCMP1;
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
    // RX Timeout
    if (BOARD_USART->IF && USART_IF_TCMP1)
    {
        BOARD_USART_CLR_IRQ_TCMP1;
        if(m_rx_callback != NULL)
        {
            uint8_t * end_buffer = (uint8_t *) LDMA->CH[DMA_RX_CH].DST;
            uint8_t * old_buffer = DoubleBuffer_getActive(m_rx_buffers);
            // Stop RX DMA
            BUS_RegBitWrite(&(LDMA->CHEN), DMA_RX_CH, 0);

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
        BOARD_USART_LDMA_CLR_IRQ = 1 << DMA_TX_CH;
        BUS_RegBitWrite(&(LDMA->CHEN), DMA_TX_CH, 0);
        BUS_RegBitWrite(&(LDMA->CHDONE), DMA_TX_CH, 0);
        // TX channel done
        m_tx_ongoing = false;
        Usart_setEnabled(false);

        // Retry to send in case something was queued while transmitting
        start_tx_lock();
    }
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
