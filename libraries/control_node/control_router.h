/* Copyright 2020 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */
#ifndef _CONTROL_ROUTER_H_
#define _CONTROL_ROUTER_H_

#include "control_node_int.h"

/**
 * \brief       Callback function type used with \ref lib_control_router_conf_t
 *
 * \param[in]   in
 *              Information about received advertiser packet.
 * \param[out]  seq
 *              Expected scratchpad sequence number if otap is requested;
 *              \ref CONTROL_NO_OTAP_SEQ, if no otap needed.
 * \param[out]  out
 *              Generated USER data for acknowledgement
 * \return      true: output generated, false: use default acknowledgement
 */
typedef bool (*control_router_send_ack_cb_f)(const ack_gen_input_t * in,
                                             app_lib_otap_seq_t * seq,
                                             ack_gen_output_t * out);

/**
 * \brief       Configuration structure for Control node router.
 */
typedef struct
{
    /** Sets callback function to be called when ack is generated as a
     *  response for advertiser device transmission.
     */
    control_router_send_ack_cb_f ack_gen_cb;
} control_router_conf_t;

/**
 * \brief       Initialize Directed Advertiser libray for router node.
 * \param[in]   conf
 *              Pointer to the configuration structure.
 * \return      Result code, normally \ref CONTROL_RET_OK. See
 *              \ref lib_control_node_ret_e for other return code.
 */
control_node_ret_e Control_Router_init(control_router_conf_t * conf);

#endif //_CONTROL_ROUTER_H_
