/* Copyright 2020 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */
#include <string.h>

#include "control_router.h"
#include "shared_data.h"
#include "tlv.h"

#define DEBUG_LOG_MODULE_NAME "CTR ROUT"
#define DEBUG_LOG_MAX_LEVEL LVL_NOLOG
#include "debug_log.h"

/** Forward declaration of DA received packet callback. */
app_lib_data_receive_res_e data_received (const shared_data_item_t * item,
                                          const app_lib_data_received_t * data);

/** DA diagnostics packet received filter and callback. */
static shared_data_item_t m_data_received_item = {
    .cb = data_received,
    .filter = {
        .mode = SHARED_DATA_NET_MODE_UNICAST,
        .src_endpoint = CONTROL_DIAG_SRC_EP,
        .dest_endpoint = CONTROL_DIAG_DEST_EP,
        .multicast_cb = NULL
    }
};

/** Copy of the configuration passed to control router library. */
static control_router_conf_t m_conf;
/** Has the library been initialinized */
static bool m_initialized = false;

/**
 * \brief       Packet reveived from DA node callback
 * \param[in]   item
 *              Pointer to the filter item that initiated the callback.
 * \param[in]   data
 *              Pointer to the received data.
 * \return      Result code, \ref app_lib_data_receive_res_e.
 */
app_lib_data_receive_res_e data_received (const shared_data_item_t * item,
                                          const app_lib_data_received_t * data)
{
    control_fwd_diag_t diag;
    app_lib_data_send_res_e res;

    LOG(LVL_DEBUG, "Received DA diagnostic packet.");

    if (!m_initialized)
    {
        /* This should not happen. As callback is set in init. */
        return APP_LIB_DATA_RECEIVE_RES_NOT_FOR_APP;
    }

    if (data->num_bytes != sizeof(control_diag_t))
    {
        LOG(LVL_ERROR, "    Invalid DIAG length (len:%d).", data->num_bytes);
    }

    diag.address = data->src_address;
    memcpy(&diag.diag, data->bytes, sizeof(control_diag_t));

    app_lib_data_to_send_t send_data =
    {
        .bytes = (const uint8_t *) &diag,
        .num_bytes = sizeof(control_fwd_diag_t),
        .delay = 0,
        .tracking_id = APP_LIB_DATA_NO_TRACKING_ID,
        .qos = APP_LIB_DATA_QOS_NORMAL,
        .flags = APP_LIB_DATA_SEND_FLAG_NONE,
        .src_endpoint = CONTROL_DIAG_SRC_EP,
        .dest_endpoint = CONTROL_DIAG_DEST_EP,
        .dest_address = APP_ADDR_ANYSINK,
    };

    /* Forward diagnostic to the sink. */
    LOG(LVL_DEBUG, "    DIAG (forward to Sink).");
    res = Shared_Data_sendData(&send_data, NULL);
    if (res != APP_LIB_DATA_SEND_RES_SUCCESS)
    {
        LOG(LVL_DEBUG, "Error forwarding diagnostics (res:%d).",
                        res);
    }

    return APP_LIB_DATA_RECEIVE_RES_HANDLED;
}

control_node_ret_e Control_Router_init(control_router_conf_t * conf)
{
    app_res_e app_res;
    app_lib_settings_role_t role;

    LOG(LVL_INFO, "Control Router Init");

    /* Check parameters. */
    if (conf->ack_gen_cb == NULL)
    {
        LOG(LVL_ERROR, "Invalid parameters.");
        return CONTROL_RET_INVALID_PARAM;
    }

    if (lib_settings->getNodeRole(&role) != APP_RES_OK)
    {
        /* The node role must be set*/
        return CONTROL_RET_INVALID_ROLE;
    }

    /* Copy config to local. */
    memcpy(&m_conf, conf, sizeof(control_router_conf_t));

    /* Set callback to receive packet from advertiser and generate response
     * to sink.
     */
    lib_advertiser->setRouterAckGenCb(m_conf.ack_gen_cb);
    app_res = Shared_Data_addDataReceivedCb(&m_data_received_item);
    if (app_res != APP_RES_OK)
    {
        LOG(LVL_ERROR, "Error adding Data callback (res:%d)", app_res);
        return CONTROL_RET_ERROR;
    }

    m_initialized = true;

    return CONTROL_RET_OK;
}
