/* Copyright 2020 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */
#ifndef _CONTROL_APP_COMMON_H_
#define _CONTROL_APP_COMMON_H_

#include <stdint.h>
#include "api.h"

/** \brief Source endpoint for Switch event packets. */
#define SRC_EP_SWITCH 21
/** \brief Destination endpoint for Switch event packets. */
#define DEST_EP_SWITCH 21

/** \brief type of packet sent by the control app. */
typedef enum
{
    CTRL_APP_SWITCH_EVENT,
} control_app_type_e;

/** \brief Switch event packet sent by DA control node. */
typedef struct  __attribute__ ((packed))
{
    /** packet type : CTRL_APP_SWITCH_EVENT. */
    control_app_type_e type;
    /** The control node button state. */
    bool button_pressed;
} control_app_switch_t;

/** \brief Switch event packet forwarded (multicast to light group) by
 *         router.
 */
typedef struct  __attribute__ ((packed))
{
    /** Control node source address. */
    app_addr_t src_addr;
    /** Switch packet sent by control node. */
    control_app_switch_t pkt;
} control_app_switch_fwd_t;


/** \brief Ack data format. Sent to control node. */
typedef struct  __attribute__ ((packed))
{
    /** DA node library parameter (diag_period_ms). */
    uint32_t diag_period_ms;
    /** DA node library parameter (packet_ttl_ms). */
    uint32_t packet_ttl_ms;
} control_app_ack_t;

/** \brief Appconfig format received by router. */
typedef struct  __attribute__ ((packed))
{
    /** Content of ack sent to control node. */
    control_app_ack_t ack;
} control_app_appconfig_t;

#endif //_CONTROL_APP_COMMON_H_