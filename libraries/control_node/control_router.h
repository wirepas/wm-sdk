/* Copyright 2020 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */
#ifndef _CONTROL_ROUTER_H_
#define _CONTROL_ROUTER_H_

#include "wms_advertiser.h"
#include "control_node_int.h"


/**
 * \brief       Configuration structure for Control node router.
 */
typedef struct
{
    /** Sets callback function to be called when ack is generated as a
     *  response for advertiser device transmission.
     */
    app_llhead_acklistener_f ack_gen_cb;
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
