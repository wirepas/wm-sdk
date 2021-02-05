/* Copyright 2017 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

#include "waddr.h"

app_addr_t Waddr_to_Addr(w_addr_t waddr)
{
    if(waddr == WADDR_BCAST)
    {
        return APP_ADDR_BROADCAST;
    }
    else if((waddr == 0) || (waddr == WADDR_ANYSINK))
    {
        // Handle legacy anysink address (0) here as well
        return APP_ADDR_ANYSINK;
    }
    // No domain conversion is necessary
    return waddr;
}

w_addr_t Addr_to_Waddr(app_addr_t app_addr)
{
    if(app_addr == APP_ADDR_BROADCAST)
    {
        return WADDR_BCAST;
    }
    else if(app_addr == APP_ADDR_ANYSINK)
    {
        // This is never executed. For data RX, the sink's unicast address is
        // always used instead.
        return WADDR_ANYSINK;
    }
    // No domain conversion is necessary
    return app_addr;
}
