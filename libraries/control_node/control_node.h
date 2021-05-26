/* Copyright 2020 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */
#ifndef _CONTROL_NODE_H_
#define _CONTROL_NODE_H_

#include "control_node_int.h"
#include "api.h"

/**
 * \brief       Callback function type used with \ref
 *              lib_control_node_conf_t.
 *
 * \param[in]   bytes
 *              USER data received in DA ack packet.
 * \param[in]   len
 *              Length of received USER data.
 */
typedef void (*control_node_ack_cb_f)(uint8_t * bytes, uint8_t len);

/**
 * \brief       Configuration structure for Control node.
 */
typedef struct
{
    /** Directed Advertiser diagnostic interval, in milli-seconds.
     *  APP_SCHEDULER_STOP_TASK to disable. As a failsafe, any value under
     *  30sec will disable DA diagnostics.
     */
    uint32_t diag_period_ms;
    /** Set maximum queueing time for advertiser data packets; 0 to disable
     *  the feature.
     */
    uint32_t packet_ttl_ms;
    /** Ack data received callback. */
    control_node_ack_cb_f ack_cb;
}control_node_conf_t;

/**
 * \brief       Initialize Directed Advertiser libray for control node.
 * \param[in]   conf
 *              Pointer to the configuration structure.
 * \return      Result code, normally \ref CONTROL_RET_OK. See
 *              \ref lib_control_node_ret_e for other return code.
 */
control_node_ret_e Control_Node_init(control_node_conf_t * conf);

/**
 * \brief       Send a data packet.
 * \param[in]   data
 *              Data to send
 * \param[in]   sent_cb
 *              DA packet sent callback.
 * \return      Result code, normally \ref CONTROL_RET_OK. See
 *              \ref lib_control_node_ret_e for other return code.
 * \note        data->dest_address willbe overwritten by strongest DA router.
 */
control_node_ret_e Control_Node_send(app_lib_data_to_send_t * data,
                                     app_lib_data_data_sent_cb_f sent_cb);

#endif //_CONTROL_NODE_H_