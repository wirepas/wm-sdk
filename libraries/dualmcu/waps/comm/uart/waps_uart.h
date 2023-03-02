/* Copyright 2017 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

#ifndef APP_WAPSUART_H__
#define APP_WAPSUART_H__

#include <stdbool.h>
#include <stdint.h>
#include "waps/comm/waps_comm.h"

/**
 * \file    waps_uart.h
 *          Low level interface for WAPS. Used for transmitting and
 *          receiving data via UART.
 */

/**
 * \brief   WAPS UART initialize, after this, WAPS UART is ready to transmit
 *          and receive serial data
 * \param   frame_cb
 *          Mandatory callback for upper layer notification about a valid
 *          looking frame
 * \param   baud
 *          Baudrate for communication
 * \param   flow_ctrl
 *          Is flow control to be used or not
 * \param   tx_buffer
 *          Memory block for transmissions
 * \param   rx_buffer
 *          Memory block for receptions
 * \return  True if successful, false otherwise
 */
bool Waps_uart_init(new_frame_cb_f frame_cb,
                    uint32_t baud,
                    bool flow_ctrl,
                    void * tx_buffer,
                    void * rx_buffer);

/**
 * \brief   Power config. Disable UART auto-powering on Sinks and LL Nodes.
 *
 */
void Waps_uart_powerReset(void);

/**
 * \brief   WAPS UART send, send a block of data via serial port
 *          Automatically applies SLIP encoding to data and appends CRC
 * \param   buffer
 *          Pointer to memory block for transmit
 * \param   size
 *          Amount of data in bytes to send
 */
bool Waps_uart_send(const void * buffer, uint32_t size);

/**
 * \brief   Flush UART module TX buffer.
 *          Waits for operation (pend) to complete before returning.
 */
void Waps_uart_flush(void);

/**
 * \brief   Set indication that something is pending
 * \param   state
 *          True means that something is pending, false that queues are empty
 * \note    This reflects only the WAPS frame buffers state, IRQ pin state
 *          is finally determined by the SPI interface itself
 */
void Waps_uart_setIrq(bool state);

/**
 * \brief   Enable automatic RX power
 */
void Waps_uart_AutoPowerOn(void);

/**
 * \brief   Disable automatic RX power
 */
void Waps_uart_AutoPowerOff(void);

/**
 * \brief   Keep power on (receiver is receiving)
 */
void Waps_uart_keepPowerOn(void);

/**
 * \brief   Grafecully power down UART, if autopower-feature is used
 */
void Waps_uart_powerOff(void);

/**
 * \brief   Callback task for power manager
 */
uint32_t Waps_uart_powerExec(void);

/**
 * \brief   Clean waps from old data.
 */
void Waps_uart_clean(void);
#endif
