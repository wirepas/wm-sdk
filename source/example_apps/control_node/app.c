/* Copyright 2020 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/*
 * \file    app.c
 * \brief   This file is the control node example application using Directed
 *          Advertiser.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "api.h"
#include "node_configuration.h"
#include "button.h"
#include "shared_data.h"
#include "control_node.h"
#include "common.h"

#define DEBUG_LOG_MODULE_NAME "NODE APP"
#define DEBUG_LOG_MAX_LEVEL LVL_INFO
#include "debug_log.h"

/** \brief This structure defines the application configuration parameters. */
typedef struct
{
    /** Control library config (diag_period_ms). */
    uint32_t diag_period_ms;
    /** Control library config (packet_ttl_ms). */
    uint32_t packet_ttl_ms;
} control_app_conf_t;

/** App default config when none is received through DA acknowledge. */
static const control_app_conf_t DEFAULT_CONF =
{
    .diag_period_ms = CONF_DIAG_PERIOD_MS,
    .packet_ttl_ms = CONF_PKT_TTL_MS,
};

/** The application configuration. */
static control_app_conf_t m_conf;

/**
 * \brief       Packet sent callback.
 *
 * \param[in]   status
 *              Status of the sent packet.
 */
static void data_sent_cb(const app_lib_data_sent_status_t * status)
{
    LOG(LVL_DEBUG, "APP - Packet sent (success: %d)", status->success);
}

/**
 * \brief       Callback for button press
 *              Send switch Event packet when button 1 is pressed.
 * \param[in]   button_id
 *              Number of button that was pressed
 * \param[in]   event
 *              Always BUTTON_PRESSED here
 */
static void switch_event(uint8_t button_id, button_event_e event)
{
    control_node_ret_e res;
    control_app_switch_t pkt = {
        .type = CTRL_APP_SWITCH_EVENT,
    };
    static bool light_state = false;

    /* This simulate the turn On / turn Off the light event. */
    light_state = !light_state;
    pkt.button_pressed = light_state;

    app_lib_data_to_send_t tx_switch_def =
    {
        .bytes = (const uint8_t *)&pkt,
        .num_bytes = sizeof(control_app_switch_t),
        .delay = 0,
        .tracking_id = APP_LIB_DATA_NO_TRACKING_ID,
        .qos = APP_LIB_DATA_QOS_NORMAL,
        .flags = APP_LIB_DATA_SEND_FLAG_NONE,
        .src_endpoint = SRC_EP_SWITCH,
        .dest_endpoint = DEST_EP_SWITCH,
    };

    LOG(LVL_INFO, "Send switch event (%s)", light_state?"On":"Off");

    res = Control_Node_send(&tx_switch_def, data_sent_cb);
    if (res != CONTROL_RET_OK)
    {
        LOG(LVL_ERROR, "Error sending Switch Event (res:%d)", res);
    }
}

/**
 * \brief       Callback for received DA Acknowledge
 *              Update application config with data received in ack packet.
 * \param[in]   bytes
 *              USER data received in DA ack packet.
 * \param[in]   len
 *              Length of received USER data.
 */
static void control_node_ack_cb(uint8_t * bytes, uint8_t len)
{

    control_app_ack_t * pkt = (control_app_ack_t *) bytes;
    control_app_conf_t conf;
    control_node_conf_t lib_conf = {
        .ack_cb = control_node_ack_cb,
    };

    LOG(LVL_INFO, "Received ACK");

    if (len != sizeof(control_app_ack_t))
    {
        LOG(LVL_ERROR, "   - Invalid ACK length");
        /* Invalid packet size. */
        return;
    }

    conf.diag_period_ms = pkt->diag_period_ms;
    conf.packet_ttl_ms = pkt->packet_ttl_ms;

    if (memcmp(&conf, &m_conf, sizeof(control_app_conf_t)))
    {
        control_node_ret_e ctrl_ret;

        LOG(LVL_INFO, "Update config.");
        LOG(LVL_DEBUG, "  - diag_period_ms: %u", conf.diag_period_ms);
        LOG(LVL_DEBUG, "  - packet_ttl_ms: %u", conf.packet_ttl_ms);
        /* Config changed; re-init app and library.*/
        memcpy(&m_conf, &conf, sizeof(control_app_conf_t));
        lib_conf.diag_period_ms = m_conf.diag_period_ms;
        lib_conf.packet_ttl_ms = m_conf.packet_ttl_ms;
        ctrl_ret = Control_Node_init(&lib_conf);
        if (ctrl_ret != CONTROL_RET_OK)
        {
            LOG(LVL_ERROR, "Error re-initializing control library (ret:%d)",
                        ctrl_ret);
            return;
        }
    }
    else
    {
        LOG(LVL_DEBUG, "Keep same config.");
    }
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
    button_res_e button_res;
    control_node_conf_t conf = {
        .ack_cb = control_node_ack_cb,
    };

    /* Copy defaults to App config struct. */
    memcpy(&m_conf, &DEFAULT_CONF, sizeof(control_app_conf_t));

    /* Initialize DA library with defaults. */
    conf.diag_period_ms = m_conf.diag_period_ms;
    conf.packet_ttl_ms = m_conf.packet_ttl_ms;

    LOG_INIT();
    LOG(LVL_INFO, "Starting Control Node");
    LOG(LVL_DEBUG, "  - diag_period_ms: %u", m_conf.diag_period_ms);
    LOG(LVL_DEBUG, "  - packet_ttl_ms: %u", m_conf.packet_ttl_ms);

    // Basic configuration of the node with a unique node address
    if (configureNodeFromBuildParameters() != APP_RES_OK)
    {
        // Could not configure the node
        // It should not happen except if one of the config value is invalid
        return;
    }

    Button_init();

    app_res = lib_settings->setNodeRole(APP_LIB_SETTINGS_ROLE_ADVERTISER);
    if (app_res != APP_RES_OK)
    {
        LOG(LVL_ERROR, "Error setting node role (res:%d)", app_res);
        return;
    }
    ctrl_ret = Control_Node_init(&conf);
    if (ctrl_ret != CONTROL_RET_OK)
    {
        LOG(LVL_ERROR, "Error initializing control library (ret:%d)",
                       ctrl_ret);
        return;
    }

    /* Button pressed callback. */
    button_res = Button_register_for_event(0, BUTTON_PRESSED, switch_event);
    if (button_res != BUTTON_RES_OK)
    {
        // It is likely the board doesn't have any button. The switch event
        // functionnality will not work.
        LOG(LVL_WARNING, "Error registering button event (res:%d)", button_res);
    }

    /* Start the stack. */
    lib_state->startStack();
}
