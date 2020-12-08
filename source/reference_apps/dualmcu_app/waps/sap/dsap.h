/* Copyright 2017 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

#ifndef WAPS_DSAP_H_
#define WAPS_DSAP_H_

#include "waps_private.h"
#include "waps_item.h"
#include "waddr.h"

/**
 *  \brief  Process received request
 *  \param  item
 *          Structure containing the received request frame
 *  \return True, if a response was generated
 */
bool Dsap_handleFrame(waps_item_t * item);

/**
 * \brief   Create a packet sent indication for sending to WAPS protocol
 * \param   id
 *          Tracking id for PDU
 * \param   src_ep
 *          Source end point
 * \param   dst_ep
 *          Destination end point
 * \param   queue_time
 *          Packet queuing time
 * \param   dst
 *          Packet destination address
 * \param   success
 *          True, if the packet was successfully sent
 * \param   output_ptr
 *          Memory area to construct the reply to
 */
void Dsap_packetSent(pduid_t id,
                     uint8_t src_ep,
                     uint8_t dst_ep,
                     uint32_t queue_time,
                     w_addr_t dst,
                     bool success,
                     waps_item_t * output_ptr);

/**
 * \brief   Create a packet received indication for sending to WAPS protocol
 * \param   data
 *          Information on incoming data
 * \param   dst_addr
 *          Destination address of packet, either unicast address or broadcast
 * \param   output_ptr
 *          Memory area to construct the reply to
 */
void Dsap_packetReceived(const app_lib_data_received_t * data,
                         w_addr_t dst_addr,
                         waps_item_t * output_ptr);

#endif /* WAPS_DSAP_H_ */
