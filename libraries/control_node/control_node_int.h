/* Copyright 2020 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */
#ifndef _CONTROL_NODE_INT_H_
#define _CONTROL_NODE_INT_H_

#include "api.h"

/** Maximum packet size. */
#define MAX_PAYLOAD  102

/** Source endpoint used for DA diagnostics. */
#define CONTROL_DIAG_SRC_EP 237

/** Destination endpoint used for DA diagnostics. */
#define CONTROL_DIAG_DEST_EP 237

/** \brief Return codes of control functions. */
typedef enum
{
    /** Operation is a success. */
    CONTROL_RET_OK,
    /** Generic Error. */
    CONTROL_RET_ERROR,
    /** Library not initialized. */
    CONTROL_RET_INVALID_STATE,
    /** Invalid parameters. */
    CONTROL_RET_INVALID_PARAM,
    /** Invalid node role. */
    CONTROL_RET_INVALID_ROLE,
    /** Error when sending packet. */
    CONTROL_RET_SEND_ERROR
} control_node_ret_e;

/** \brief DA Diagnostic packet. */
typedef struct __attribute__ ((packed))
{
    /** lib_otap->getProcessedSeq(). */
    uint8_t proc_otap_seq;
    /** lib_otap->getSeq(). */
    uint8_t stored_otap_seq;
    /** Number of diag packet sent successfully. */
    uint16_t success;
    /** Number of diag packet sending error. */
    uint16_t error;
    /** Time from \ref Control_Node_send call to ack received for packet n-1.
     *  0xFFFFFFFF means last diag packet was not sent (error).
     */
    uint32_t timing_us;
} control_diag_t;

/** \brief Forwarded (by router to Sink) Diagnostic packet. */
typedef struct __attribute__ ((packed))
{
    /** Control node source address. */
    app_addr_t address;
    /** Diag packet sent by control node. */
    control_diag_t diag;
} control_fwd_diag_t;

#endif //_CONTROL_NODE_INT_H_