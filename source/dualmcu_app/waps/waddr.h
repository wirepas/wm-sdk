/* Copyright 2017 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

#ifndef WADDR_H_
#define WADDR_H_

#include "waps_frames.h"
#include "api.h"

/**
 * \file    waddr.h
 *          Address mapping between internal and external addresses
 *
 * \note    Size of w_addr_t is defined in waps_frames.
 * \note    Waddr has valid ranges of:
 *          0x0000 0000 (legacy any sink address, avoid using this)
 *          0x0000 0001 - 0x7FFF FFFF (unicast address)
 *          0x8000 0000 - 0x80FF FFFD (multicast address)
 *          0x8100 0000 - 0xFFFF FFFD (unicast address)
 *          0xFFFF FFFE (any sink address)
 *          0xFFFF FFFF (broadcast address)
 */

/** Broadcast address symbol */
#define WADDR_BCAST     (w_addr_t)(-1)

/** Any sink symbol */
#define WADDR_ANYSINK   (w_addr_t)(-2)

/** Highest valid unicast node address for a device */
#define WADDR_UCAST_MAX (w_addr_t)(-3)

/** Multicast bit for address */
#define WADDR_MULTICAST (w_addr_t)(0x80000000)

/**
 * \brief   Convert address from WAPS to APP domain
 * \param   waddr
 *          Waps address to convert
 * \return  Converted address value
 */
app_addr_t Waddr_to_Addr(w_addr_t waddr);

/**
 * \brief   Convert address from APP to WAPS domain
 * \param   app_addr
 *          Application address to convert
 * \return  Converted address value
 */
w_addr_t Addr_to_Waddr(app_addr_t app_addr);

#endif /* WADDR_H_ */
