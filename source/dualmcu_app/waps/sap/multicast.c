/* Copyright 2018 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

#include "multicast.h"
#include "waddr.h"
#include <string.h>
#include "persistent.h"

/**
 * \brief   Convert packed multicast address to app addr structure
 * \param   addr
 *          Packed address
 * \return  App address
 */
static app_addr_t mcast_group_addr_to_app_addr(multicast_group_addr_t * addr)
{
    app_addr_t retval = APP_ADDR_MULTICAST;
    memcpy(&retval, addr, sizeof(multicast_group_addr_t));
    return retval;
}

/**
 * \brief   Convert waddr to multicast group address
 * \param   from
 *          Waps address (in unaligned pointer format)
 * \param   to
 *          Where conversion is done
 */
static void waddr_to_mcast_group_addr(const uint8_t * from,
                                      multicast_group_addr_t * to)
{
    memcpy(to, from, sizeof(multicast_group_addr_t));
}

/**
 * \brief   Convert multicast groud address to waddr
 * \param   from
 *          Multicast address used
 * \param   to
 *          Where conversion is done
 */
static void mcast_group_addr_to_waddr(const multicast_group_addr_t * from,
                                      uint8_t * to)
{
    // Make sure MSByte is cleared, copy to aligned 32-bit variable first
    w_addr_t addr = 0;
    memcpy(&addr, from, sizeof(multicast_group_addr_t));
    memcpy(to, &addr, sizeof(w_addr_t));
}

bool Multicast_isGroupCb(app_addr_t group_addr)
{
    multicast_group_addr_t addresses[MULTICAST_ADDRESS_AMOUNT];

    if (Persistent_getGroups(&addresses[0]) != APP_RES_OK)
    {
        // Failure, not a member of the group
        return false;
    }

    for (uint_fast8_t i=0; i < MULTICAST_ADDRESS_AMOUNT; i++)
    {
        // Check if group address matches
        app_addr_t groupaddr = mcast_group_addr_to_app_addr(&addresses[i]);
        if (groupaddr == group_addr)
        {
            return true;
        }
    }

    return false;
}

app_res_e Multicast_setGroups(const uint8_t * groups)
{
    // Storage groups used
    multicast_group_addr_t stgroups[MULTICAST_ADDRESS_AMOUNT];

    // Copy addresses one-by-one (convert from 32-bits to 24-bits)
    for (uint_fast8_t i=0; i < MULTICAST_ADDRESS_AMOUNT; i++)
    {
        waddr_to_mcast_group_addr(&groups[i * sizeof(w_addr_t)], &stgroups[i]);
    }

    // Set to storage
    return Persistent_setGroups(&stgroups[0]);
}

app_res_e Multicast_getGroups(uint8_t * groups)
{
    multicast_group_addr_t persistent_groups[MULTICAST_ADDRESS_AMOUNT];

    app_res_e retval = Persistent_getGroups(&persistent_groups[0]);

    if (retval == APP_RES_OK)
    {
        for (uint_fast8_t i=0; i < MULTICAST_ADDRESS_AMOUNT; i++)
        {
            mcast_group_addr_to_waddr(&persistent_groups[i], groups);
            groups += sizeof(w_addr_t);
        }
    }

    return retval;
}
