/**
* @file         poslib_event.h
* @brief        Header file for the poslib_event.c
* @copyright    Wirepas Ltd. 2020
*/

#ifndef _POSLIB_EVENT_H_
#define _POSLIB_EVENT_H_

/**
 * @brief   Processing of a network scan edge.
 *
 *          When a network scan is performed, the application bundles all the
 *          available beacons in positioning measurement payloads and
 *          dispatches them towards a sink.
 *
 *          A message is only sent if there is at least one neighbor.
 *
 *          After the message is *requested* to be sent, the node either goes
 *          to sleep or stays idle - waiting for a network scan trigger.*
 *
 *          Possible state changes:
 *              - Wait for packet acknowledgement
 *              - Wait for connection
 * @return  See \ref positioning_state_e
 */
void PosLibEvent_ScanEnd(void);

/**
 * @brief   Processing of an appconfig message.
 *
 *          When a node receives an appconfig, it attempts to decode it
 *          against the known overhaul types.
 *
 *          By default nothing else is done.
 *
 * @param   bytes         The bytes
 * @param   num_bytes     The number bytes
 */
void PosLibEvent_AppConfig(const uint8_t * bytes, uint32_t num_bytes);

/**
 * @brief   Gives info if ack from stack is received for sent message.
 *          Every reads clears the received ack status to false;
 * @return  true if ack is received
 */
bool PosLibEvent_AckReceived(void);
#endif
