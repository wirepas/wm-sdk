/* Copyright 2017 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

#ifndef UNIQUEID_H
#define UNIQUEID_H

#include "mcu.h"

static inline uint32_t getUniqueId()
{
    return NRF_FICR->DEVICEID[0];
}
#endif
