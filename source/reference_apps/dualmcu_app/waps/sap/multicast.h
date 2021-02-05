/* Copyright 2018 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */
#ifndef _MULTICAST_H_
#define _MULTICAST_H_

#include "api.h"

// Amount of multicast addresses supported. Must fit to storage.
#define MULTICAST_ADDRESS_AMOUNT    10

// Definition for multicast address
typedef struct __attribute__ ((__packed__))
{
    uint8_t addr[3];    // LSB first
} multicast_group_addr_t;

/**
 * \brief   Callback for querying group callback
 * \param   group_addr
 *          Address of the group
 * \return  true: Is part of this group, false: Is not part of this group
 */
bool Multicast_isGroupCb(app_addr_t group_addr);

/**
 * \brief   Set multicast groups
 * \param   groups
 *          Pointer to the groups (non-aligned pointer)
 * \return  Result of the operation
 */
app_res_e Multicast_setGroups(const uint8_t * groups);

/**
 * \brief   Get multicast groups
 * \param   groups
 *          Out: Pointer to the groups (non-aligned pointer). Note, value 0 is
 *          used for replacement value of 0xffffffff (factory reset)
 * \return  Result of the operation
 */
app_res_e Multicast_getGroups(uint8_t * groups);

#endif /* SOURCE_DUALMCU_APP_WAPS_SAP_MULTICAST_H_ */
