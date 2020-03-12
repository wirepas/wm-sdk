// TEST_REG_SDK_ONLY_BEGIN
/* Copyright 2017 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

#ifndef WAPS_TSAP_H_
#define WAPS_TSAP_H_

#include <stdbool.h>

#include "waps_item.h"

/**
 *  \brief  Process received request
 *  \param  item
 *          Structure containing the received request frame
 *  \return True, if a response was generated
 */
bool Tsap_handleFrame(waps_item_t * item);

/**
 *  \brief  Creates indication containing test data
 *  \param  bytes
 *          Test data in playload
 * \param   size
 *          Size of the test data in payload
 * \param   seq
 *          Sequence/packet number of the received data
 * \param   duplicates
 *          Number of received duplicates
 *          (consecutive packages have the same sequence number)
 * \param   rxCntr
 *          Number of all received packages (including duplicates)
 * \param   rssi
 *          Absolute signal strength in dBm of the last received message
 * \param   item
 *          Structure containing the received request frame
 */
void Tsap_testPacketReceived(const uint8_t * bytes,
                                   size_t size,
                                   uint32_t seq,
                                   uint32_t duplicates,
                                   uint32_t rxCntr,
                                   int8_t rssi,
                                   waps_item_t * item);

#endif /* WAPS_TSAP_H_ */
// TEST_REG_SDK_ONLY_END
