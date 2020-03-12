/* Copyright 2017 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

#ifndef WAPS_CSAP_H_
#define WAPS_CSAP_H_

#include <stdbool.h>
#include "waps_item.h"

/**
 *  \brief  Process received request
 *  \param  item
 *          Structure containing the received request frame
 *  \return True, if a response was generated
 */
bool Csap_handleFrame(waps_item_t * item);

#endif /* WAPS_CSAP_H_ */
