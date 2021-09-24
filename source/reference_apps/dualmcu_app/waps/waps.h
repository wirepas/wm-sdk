/* Copyright 2017 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

#ifndef WAPS_H__
#define WAPS_H__

/**
 * \file    waps.h
 *          WAPS layer main module.
 *
 *          WAPS processes primitives from lower level, serves queries, forwards
 *          upper layer primitives.
 *          WAPS takes care of application registrations.
 */

#include <stdint.h>
#include <stdbool.h>

#include "api.h"

/**
 * \brief   Different Dual MCU API interface options
 */
typedef enum
{
    APP_UART_INT,  //!< The Dual MCU API is through UART
} interface_type_e;

/**
 * \brief   Structure to store the interface configuration
 */
typedef struct
{
    uint32_t baudrate;  //!< The baud rate (only for uart)
    uint8_t  interface; //!< The interface to use
    bool     flow_ctrl; //!< Enable hardware flow control
    uint8_t  padding[2];//!< Padding to be memory alligned
} app_interface_config_s;

extern const app_interface_config_s m_interface_config;

/**
 * \brief   Initializes waps and modules it requires.
 *          Currently waps relies on waps_uart as sole communication module.
 *          It is initialized first. Finally, attribute manager is initialized.
 * \return  True if successful, false otherwise
 */
bool Waps_init(void);


/**
 * \brief   Packet sent callback
 * \param   tracking_id
 *          Tracking ID of PDU
 * \param   src_ep
 *          Source end point
 * \param   dst_ep
 *          Destination end point
 * \param   queue_time
 *          Queuing time of packet
 * \param   dst_addr
 * \param   success
 *          True: Message was sent OK to next hop. False: Message was discarded (timed out)
 */
void Waps_packetSent(app_lib_data_tracking_id_t tracking_id,
                     uint8_t src_ep,
                     uint8_t dst_ep,
                     uint32_t queue_time,
                     app_addr_t dst_addr,
                     bool success);

#endif // WAPS_H_
