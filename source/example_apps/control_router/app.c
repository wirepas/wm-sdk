/* Copyright 2020 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/*
 * \file    app.c
 * \brief   This file is the control router application to be used with the
 *          control_node application.
 *
 * This application is based on control library that manages all the low level
 * aspects of control router using directed advertiser (controlling DA node
 * diagnostics and otap).
 *
 * When receiving a switch pressed event from a control node, the app will
 * forward the message to the associated control light group using multicast.
 *
 * Control node configuration is received through appconfig (using the shared
 * appconfig library TLV format).
 *
 * [Version 2B][Nb entry 1B][Type 1B][Length 1B][Otap seq 1B][Diag Period ms 4B]
 *                                                           [DA packet TTL 4B]
 * Ex : F6 7E 01 C0 0D 00 60 EA 00 00 FA 00 00 00
 * --> Shared AppConfig Version: 0x7EF6
 * --> Shared AppConfig Number of entry: 1
 * --> Shared AppConfig Type: 0xC0
 * --> Shared AppConfig Length of data: 13 bytes
 * --> No otap (0x00)
 * --> 60sec DA diag period
 * --> 250ms DA max packet ttl
 * Warning: No test is performed on the validity of the config received by
 * app config.
 *
 */

#include <string.h>

#include "api.h"
#include "node_configuration.h"
#include "control_router.h"
#include "shared_data.h"
#include "shared_appconfig.h"
#include "../control_node/common.h"

#define DEBUG_LOG_MODULE_NAME "ROUT APP"
#define DEBUG_LOG_MAX_LEVEL LVL_INFO
#include "debug_log.h"

/** Control router type for shared appconfig. */
#define APP_CONFIG_CTRL_ROUTER_TYPE 0xC0

static bool m_received_config;
static control_app_ack_t m_ack_config;
static uint8_t m_ack_seq;
static uint16_t m_filter_id;


/** Forward declaration of switch packet received callback. */
static app_lib_data_receive_res_e received_switch_cb(
                                        const shared_data_item_t * item,
                                        const app_lib_data_received_t * data);

/** Forward declaration of appconfig received callback. */
void received_appconfig (uint16_t type, uint8_t length, uint8_t * value_p);

/** Switch packet received filter and callback. */
static shared_data_item_t m_switch_received_item = {
    .cb = received_switch_cb,
    .filter = {
        .mode = SHARED_DATA_NET_MODE_UNICAST,
        .src_endpoint = SRC_EP_SWITCH,
        .dest_endpoint = DEST_EP_SWITCH,
        .multicast_cb = NULL
    }
};

/** Appconfig received filter and callback. */
shared_app_config_filter_t m_da_appconfig_item = {
    .type = APP_CONFIG_CTRL_ROUTER_TYPE,
    .cb = received_appconfig
};

/**
 * \brief   Switch event packet received callbacks.
 *          Forward switch packet to the good multicast light group.
 *
 * \param   item
 *          Pointer to the filter item that initiated the callback.
 * \param   data
 *          Pointer to the received data.
 * \return  Always APP_LIB_DATA_RECEIVE_RES_HANDLED.
 */
static app_lib_data_receive_res_e received_switch_cb(
                                        const shared_data_item_t * item,
                                        const app_lib_data_received_t * data)
{
    control_app_switch_t * swtch = (control_app_switch_t *) &(data->bytes[0]);
    control_app_switch_fwd_t swtch_fwd;
    app_lib_data_to_send_t tx_switch_def =
    {
        .bytes = (const uint8_t *)&swtch_fwd,
        .num_bytes = sizeof(control_app_switch_fwd_t),
        .delay = 0,
        .tracking_id = APP_LIB_DATA_NO_TRACKING_ID,
        .qos = APP_LIB_DATA_QOS_NORMAL,
        /* This packet will only be received by CSMA nodes. */
        .flags = APP_LIB_DATA_SEND_FLAG_UNACK_CSMA_CA,
        .src_endpoint = SRC_EP_SWITCH,
        .dest_endpoint = DEST_EP_SWITCH,
        .dest_address = APP_ADDR_BROADCAST
    };

    LOG(LVL_INFO, "Received Switch Event.");

    if (data->num_bytes == sizeof(control_app_switch_t))
    {
        app_lib_data_send_res_e res;

        swtch_fwd.src_addr = data->src_address;
        memcpy(&swtch_fwd.pkt, swtch, sizeof(control_app_switch_t));

        /* Send packet to light groups. */
        res = Shared_Data_sendData(&tx_switch_def, NULL);
        if (res != APP_LIB_DATA_SEND_RES_SUCCESS)
        {
            LOG(LVL_ERROR, "Error sending data (res:%d)", res);
        }
    }
    else
    {
        LOG(LVL_ERROR, "Invalid packet length.");
    }

    return APP_LIB_DATA_RECEIVE_RES_HANDLED;
}

/**
 * \brief       Generate response to control node. Send App config.
 * \param[in]   in
 *              Information about received advertiser packet
 * \param[out]  seq
 *              Expected scratchpad sequence number if otap is requested;
 *              0, if no otap needed.
 * \param[out]  out
 *              Generated USER data for acknowledgement
 * \return      always true, i.e. send acknowledgement
 */
static bool acklistener_cb(const ack_gen_input_t * in,
                           app_lib_otap_seq_t * seq,
                           ack_gen_output_t * out)
{
    if (m_received_config)
    {
        LOG(LVL_INFO, "Generate ACK");
        *seq = m_ack_seq;
        out->data = (void *) &m_ack_config;
        out->length = sizeof(control_app_ack_t);

        return true;
    }
    else
    {
        *seq = CONTROL_NO_OTAP_SEQ;
    }

    return false;
}

/**
 * \brief       Received Appconfig, contains the Control node configuration.
 *
 * \param[in]   type
 *              The type that match the filtering
 * \param[in]   length
 *              The length of this TLV entry.
 * \param[in]   value_p
 *              Received Appconfig data for Control router
 */
void received_appconfig(uint16_t type, uint8_t length, uint8_t * value_p)
{
    if (length != sizeof(control_app_appconfig_t))
    {
        LOG(LVL_ERROR, "Appconfig, invalid length.");
        return;
    }

    control_app_appconfig_t * pkt = (control_app_appconfig_t *) value_p;

    m_received_config = true;

    m_ack_seq = pkt->app_seq;
    m_ack_config.diag_period_ms = pkt->ack.diag_period_ms;
    m_ack_config.packet_ttl_ms = pkt->ack.packet_ttl_ms;

    LOG(LVL_INFO, "Received appconfig");
    LOG(LVL_DEBUG, "  - diag_period_ms: %u", m_ack_config.diag_period_ms);
    LOG(LVL_DEBUG, "  - packet_ttl_ms: %u", m_ack_config.packet_ttl_ms);
}

/**
 * \brief   Initialization callback for application
 *
 * This function is called after hardware has been initialized but the
 * stack is not yet running.
 *
 */
void App_init(const app_global_functions_t * functions)
{
    app_res_e app_res;
    control_node_ret_e ctrl_ret;
    shared_app_config_res_e app_cfg_res;
    control_router_conf_t conf = {
        .ack_gen_cb = acklistener_cb
    };

    LOG_INIT();
    LOG(LVL_INFO, "Starting Control Router");

    // Basic configuration of the node with a unique node address
    if (configureNodeFromBuildParameters() != APP_RES_OK)
    {
        // Could not configure the node
        // It should not happen except if one of the config value is invalid
        return;
    }

    app_res = lib_settings->setNodeRole(
                app_lib_settings_create_role(APP_LIB_SETTINGS_ROLE_HEADNODE,
                                             APP_LIB_SETTINGS_ROLE_FLAG_LL));
    if (app_res != APP_RES_OK)
    {
        LOG(LVL_ERROR, "Error setting node role (res:%d)", app_res);
        return;
    }

    Shared_Data_init();  // Always return APP_RES_OK
    Shared_Appconfig_init();  // Always return APP_RES_OK
    ctrl_ret = Control_Router_init(&conf);
    if (ctrl_ret != CONTROL_RET_OK)
    {
        LOG(LVL_ERROR, "Error initializing control library (ret:%d)",
                       ctrl_ret);
        return;
    }

    app_res = Shared_Data_addDataReceivedCb(&m_switch_received_item);
    if (app_res != APP_RES_OK)
    {
        LOG(LVL_ERROR, "Error adding Data callback (res:%d)", app_res);
        return;
    }

    app_cfg_res = Shared_Appconfig_addFilter(&m_da_appconfig_item,
                                             &m_filter_id);
    if (app_cfg_res != SHARED_APP_CONFIG_RES_OK)
    {
        LOG(LVL_ERROR, "Error adding AppConfig filter (res:%d)", app_cfg_res);
        return;
    }

    m_received_config = false;

    /*
     * Start the stack.
     * This is really important step, otherwise the stack will stay stopped and
     * will not be part of any network. So the device will not be reachable
     * without reflashing it
     */
    lib_state->startStack();
}
