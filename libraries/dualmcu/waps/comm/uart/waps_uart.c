/* Copyright 2017 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

#include "waps/comm/uart/waps_uart.h"

#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include "indication_signal.h"
#include "usart.h"
#include "crc.h"
#include "waps/waps_buffer_sizes.h"
#include "waps/waps_frames.h" // For frame min/max length constants
#include "mcu.h"
#include "wms_settings.h"


/* SLIP special characters */
#define SLIP_END                (uint8_t)0xC0
#define SLIP_ESC                (uint8_t)0xDB
#define SLIP_ESC_END            (uint8_t)0xDC
#define SLIP_ESC_ESC            (uint8_t)0xDD

/** Verifies frame and puts it to received frames queue */
static void frame_completed(void);

/** RX callback for serial port */
static void waps_uart_receive(uint8_t * chars, size_t n);

/** Return true if autopower must be used */
static bool is_autopower_in_use(void);

__STATIC_INLINE void write_tx_buffer(uint8_t ch);
__STATIC_INLINE void write_rx_buffer(uint8_t ch);
__STATIC_INLINE void reset_rx_buffer(void);
__STATIC_INLINE void slip_put(uint8_t ch);

/** Buffers for TX/RX */
static uint32_t         m_tx_buffer_idx;
static uint8_t *        m_tx_buffer;
static uint32_t         m_rx_buffer_idx;
static uint8_t *        m_rx_buffer;
static crc_t            m_rx_crc;

/* Status of receiver */
static volatile bool    m_escaped;

#if defined WAPS_DIAGNOSTICS
/* Waps diagnostics */
typedef struct {
    uint32_t    successful_frame;
    uint32_t    escape_error;
    uint32_t    way_too_short_frame_error;
    uint32_t    frame_size_out_of_bounds_error;
    uint32_t    crc_error;
} waps_diagnostics_t;
static volatile waps_diagnostics_t m_waps_diagnostics;
#endif /* WAPS_DIAGNOSTICS */

/** Valid frame received callback */
static new_frame_cb_f   m_frame_cb;

/** Baudrate configured for the uart */
static uint32_t         m_baudrate;

/** Constant for maximum possible baudrate for autopower mechanism */
#define MAX_BAUDRATE_FOR_AUTOPOWER  125000

/** Waps uart init */
bool Waps_uart_init(new_frame_cb_f frame_cb,
                    uint32_t baud,
                    bool flow_ctrl,
                    void * tx_buffer,
                    void * rx_buffer)
{
    bool res = false;
    uart_flow_control_e flow;
    m_escaped = false;
    m_frame_cb = frame_cb;
    m_tx_buffer = tx_buffer;
    m_rx_buffer = rx_buffer;
    m_tx_buffer_idx = 0;
    m_baudrate = baud;
    reset_rx_buffer();
    flow_ctrl ? (flow = UART_FLOW_CONTROL_HW) :
                (flow = UART_FLOW_CONTROL_NONE);
#if defined WAPS_DIAGNOSTICS
    memset((void *)&m_waps_diagnostics, 0x00, sizeof(m_waps_diagnostics));
#endif /* WAPS_DIAGNOSTICS */
    res = Usart_init(baud, flow);
    Usart_enableReceiver(waps_uart_receive);

    Waps_uart_powerReset();

    return res;
}

void Waps_uart_powerReset(void)
{
    if (is_autopower_in_use())
    {
        // Disable the continuous UART powering
        // and enable the auto-powering.
        Usart_receiverOff();
        Usart_setEnabled(false);
        Waps_uart_AutoPowerOn();
    }
    else
    {
        // Disable the UART auto-powering and
        // enable the continuous one.
        Waps_uart_AutoPowerOff();
        Usart_setEnabled(true);
        Usart_receiverOn();
    }

    // Initialize UART IRQ pin
    IndicationSignal_enable();
    IndicationSignal_clear();
}

bool Waps_uart_send(const void * buffer, uint32_t size)
{
    uint8_t * p = (uint8_t *)buffer;
    uint32_t ret = 0;
    uint32_t size_in;
    crc_t crc;
    size_in = size;
    crc.crc = Crc_fromBuffer(p, size);
    m_tx_buffer_idx = 0;
    write_tx_buffer(SLIP_END);
    while(size--)
    {
        slip_put(*p++);
    }
    slip_put(crc.lsb);
    slip_put(crc.msb);
    write_tx_buffer(SLIP_END);
    if(m_tx_buffer_idx < size_in)
    {
        /* If out bytes is less than size, then output buffer does not contain
         * the entire message -> do not write UART, return error instead */
        return false;
    }
    ret = Usart_sendBuffer((void *)m_tx_buffer, m_tx_buffer_idx);
    return (bool)(ret == m_tx_buffer_idx);
}

void Waps_uart_flush(void)
{
    Usart_flush();
}

void Waps_uart_setIrq(bool state)
{
   if(state)
   {
       // Assert IRQ pin
       IndicationSignal_set();
   }
   else
   {
       // De-assert IRQ pin
       IndicationSignal_clear();
   }
}

void Waps_uart_clean(void)
{
    reset_rx_buffer();
}

static void frame_completed(void)
{
    uint32_t pld_size;
    crc_t crc_1, crc_2;
    uint8_t error = 0;
    /* Step 1: see if frame makes any sense */
    if(m_rx_buffer_idx >= sizeof(crc_t))
    {
        pld_size = m_rx_buffer_idx - sizeof(crc_t);
        /* Step 2: see if upper layer payload length is ok */
        if ((pld_size >= WAPS_MIN_FRAME_LENGTH) &&
            (pld_size <= WAPS_MAX_FRAME_LENGTH))
        {
            /* Step 3: see if CRC makes any sense */
            crc_1.crc = m_rx_crc.crc; // Calculated on the fly, from DATA + CRC
            crc_2.lsb = m_rx_buffer[pld_size];
            crc_2.msb = m_rx_buffer[pld_size+1];
            /* As the receiver calculates CRC over data + CRC, add it here */
            crc_2.crc = Crc_addByte(crc_2.crc, m_rx_buffer[pld_size]);
            crc_2.crc = Crc_addByte(crc_2.crc, m_rx_buffer[pld_size+1]);
            if(crc_1.crc == crc_2.crc)
            {
                /* CRC valid, message OK by serial: Serial off */
                Waps_uart_powerOff();
                /* Send message to upper layer */
                if(m_frame_cb != NULL)
                {
#if defined WAPS_DIAGNOSTICS
                    m_waps_diagnostics.successful_frame++;
#endif /* WAPS_DIAGNOSTICS */
                    (void)m_frame_cb((void *)m_rx_buffer, pld_size);
                }
            }
            else
            {
                error = 1;
#if defined WAPS_DIAGNOSTICS
                m_waps_diagnostics.crc_error++;
#endif /* WAPS_DIAGNOSTICS */
            }
        }
        else
        {
            error = 2;
#if defined WAPS_DIAGNOSTICS
            m_waps_diagnostics.frame_size_out_of_bounds_error++;
#endif /* WAPS_DIAGNOSTICS */
        }
    }
    else
    {
        error = 3;
#if defined WAPS_DIAGNOSTICS
        m_waps_diagnostics.way_too_short_frame_error++;
#endif /* WAPS_DIAGNOSTICS */
    }

    if ( error != 0)
    {
        // Error in reception from host, so no answer will be provided.
        // In order to avoid host waiting for timeout
        // return back the error by generating ourself a dummy response
        // containing a CRC error (CRC being 0xFFFF, it cannot be valid)
        // (payload size must be at least 4)
        /* Dummy message containing a CRC error (So host can interpret it as a propagated CRC error) */
        uint8_t dummy_invalid_crc_message[] = {SLIP_END, error, m_rx_buffer_idx & 0xFF, (m_rx_buffer_idx >> 8) & 0xFF, 0xFF, 0xFF, SLIP_END};
        Usart_sendBuffer(&dummy_invalid_crc_message, sizeof(dummy_invalid_crc_message));
    }
    reset_rx_buffer();
}

static void waps_uart_receive(uint8_t * chars, size_t n)
{
    uint8_t ch;
    /* Something received, keep UART power on */
    Waps_uart_keepPowerOn();

    /* Check number of bytes. Upper limit is here to avoid infinite loop and
     * has lot of margin for slip encoding. Lower level sent a slip encoded message
     * that is stored in an internal buffer without slip encoding.
     * Internal overflow buffer is checked in write_rx_buffer
     */
    if ((n == 0) || (n >= (WAPS_RX_BUFFER_SIZE * 2)))
    {
        /* Wrong value received */
        return;
    }

    while (n--)
    {
        ch = *(chars++);
        /* Check state machine */
        if(m_escaped)
        {
            switch(ch)
            {
                case SLIP_ESC_END:
                    write_rx_buffer(SLIP_END);
                    break;
                case SLIP_ESC_ESC:
                    write_rx_buffer(SLIP_ESC);
                    break;
                default:
                    /* Unexpected ESC character -> reset */
    #if defined WAPS_DIAGNOSTICS
                    m_waps_diagnostics.escape_error++;
    #endif /* WAPS_DIAGNOSTICS */
                    reset_rx_buffer();
            }
            m_escaped = false;
        }
        else if (ch == SLIP_END)
        {
            /* No point in doing anything if sequential END characters received */
            if(m_rx_buffer_idx != 0)
            {
                frame_completed();
            }
        }
        else if(ch == SLIP_ESC)
        {
            m_escaped = true;
        }
        else
        {
            write_rx_buffer(ch);
        }
    }
}

static bool is_autopower_in_use(void)
{
#if BOARD_HW_CRYSTAL_32K == 0
    // Auto power mechanism cannot be used without 32kHz crystal.
    return false;
#else
    app_lib_settings_role_t node_role;
    if (m_baudrate > MAX_BAUDRATE_FOR_AUTOPOWER)
    {
        // Auto power mechanism rely on UART 0xC0 preamble sent by host library.
        // Dual mcu API only refers to 3 symbols that represent the minimum time
        // for the uart module to wakeup at 125000 baudrate. At a higher baudrate
        // it will probably not wakeup fat enough.
        // Only solution for now is to disable it. But it is probably fine
        // because the need for speed higher than 125000 is often for sinks that
        // are mainly main powered.
        // Final solution would be to extend dual mcu api to explicitly
        // enable/disable it from host side and a setting to inform the lib
        // about how many C0 bytes are needed to wake up uart module.
        return false;
    }

    if (lib_settings->getNodeRole(&node_role) != APP_RES_OK)
    {
        // If the node role is not set then the UART auto-powering is used.
        return true;
    }

    if (node_role == APP_LIB_SETTINGS_ROLE_SINK_LE ||
        node_role == APP_LIB_SETTINGS_ROLE_SINK_LL ||
        node_role == APP_LIB_SETTINGS_ROLE_HEADNODE_LL ||
        node_role == APP_LIB_SETTINGS_ROLE_SUBNODE_LL ||
        node_role == APP_LIB_SETTINGS_ROLE_AUTOROLE_LL)
    {
        // No need to enable autopower on sink or LL nodes
        // That are main powered
        return false;
    }

    // Enable it in all other cases
    return true;
#endif
}

__STATIC_INLINE void write_tx_buffer(uint8_t ch)
{
    if(m_tx_buffer_idx < WAPS_TX_BUFFER_SIZE)
    {
        m_tx_buffer[m_tx_buffer_idx++] = ch;
    }
}

__STATIC_INLINE void write_rx_buffer(uint8_t ch)
{
    if(m_rx_buffer_idx < WAPS_RX_BUFFER_SIZE)
    {
        m_rx_buffer[m_rx_buffer_idx++] = ch;
        // Calculate new CRC value
        m_rx_crc.crc = Crc_addByte(m_rx_crc.crc, ch);
    }
}

__STATIC_INLINE void reset_rx_buffer(void)
{
    m_rx_buffer_idx = 0;
    m_rx_crc.crc = Crc_initValue();
}

__STATIC_INLINE void slip_put(uint8_t ch)
{
    switch(ch)
    {
        case SLIP_ESC:
            write_tx_buffer(SLIP_ESC);
            write_tx_buffer(SLIP_ESC_ESC);
            break;
        case SLIP_END:
            write_tx_buffer(SLIP_ESC);
            write_tx_buffer(SLIP_ESC_END);
            break;
        default:
            write_tx_buffer(ch);
            break;
    }
}
