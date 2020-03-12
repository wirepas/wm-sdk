/* Copyright 2017 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

#ifndef WAPS_MSAP_H_
#define WAPS_MSAP_H_

#include "waps_item.h"
#include "api.h"

/**
 *  \brief  Process received request
 *  \param  item
 *          Structure containing the received request frame
 *  \return True, if a response was generated
 */
bool Msap_handleFrame(waps_item_t * item);

/**
 * \brief   Used to generate an indication when stack reboots
 * \return  waps_item containing stack status
 */
waps_item_t * Msap_getStackStatusIndication(void);

/**
 * \brief   Create new app config indication
 * \note    This should only be called when new DREQ are received
 * \param   seq
 *          Application config sequence
 * \param   config
 *          New application config data
 * \param   interval
 *          Diagnostic interval in seconds
 * \param   item
 *          Memory area where indication is constructed to
 */
 void Msap_handleAppConfig(uint8_t seq,
                           const uint8_t * config,
                           uint16_t interval,
                           waps_item_t * item);

/**
 * \brief   Generate an indication after to scan neighbors is done
 * \param   item
 *          Where the indication is generated
 */
void Msap_onScannedNbors(waps_item_t * item);

#endif /* WAPS_MSAP_H_ */
