/* Copyright 2020 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/*
 * \file    app.c
 * \brief   This file is the control router application to be used with the
 *          control_node application.
 */

#include <string.h>

#include "api.h"
#include "node_configuration.h"
#include "control_router.h"
#include "app_scheduler.h"
#include "shared_data.h"
#include "shared_appconfig.h"
#include "led.h"
#include "../control_node/common.h"

#define DEBUG_LOG_MODULE_NAME "ROUT APP"
#define DEBUG_LOG_MAX_LEVEL LVL_INFO
#include "debug_log.h"

/** Control router type for shared appconfig. */
#define APP_CONFIG_CTRL_ROUTER_TYPE 0xC0

/** \brief  Maximum execution time for light task (max measured = 21us)*/
#define LIGHT_TASK_EXEC_TIME_US   30

/** The delay to turn on/off the light. It is used to synchronize the lights in
 *  the network. This delay is compensated by the travel time. The default
 *  value is 250ms. It equals to the TTL of the packet sent by the control
 *  node.
 */
#define MAX_LIGHT_TURN_ON_DELAY_MS 250

/** Convert time from 1/128th second to milliseconds. */
#define COARSE_TO_MS(delay)    ((1000u * (delay)) >> 7)

/** Convert time from milliseconds to from 1/128th second. */
#define MS_TO_COARSE(delay)    (((delay) * 128) / 1000u)

static bool m_received_config;
static control_app_ack_t m_ack_config;
static uint16_t m_filter_id;

/** The state of the light (on or off). */
static bool m_light_state;


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
        .mode = SHARED_DATA_NET_MODE_ALL,
        .src_endpoint = SRC_EP_SWITCH,
        .dest_endpoint = DEST_EP_SWITCH,
        .multicast_cb = NULL
    }
};

/** Appconfig received filter and callback. */
shared_app_config_filter_t m_da_appconfig_item = {
    .type = APP_CONFIG_CTRL_ROUTER_TYPE,
    .cb = received_appconfig,
    .call_cb_always = false
};

/**
 * \brief       The light task.
 *              Turn on/off the led
 *
 * \return      APP_SCHEDULER_STOP_TASK
 */
static uint32_t light_task(void)
{
    LOG(LVL_INFO, "Turn %s light (defered task).",
                  m_light_state?"On":"Off");

    Led_set(0, m_light_state);

    return APP_SCHEDULER_STOP_TASK;
}

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
    // Switch event sent by a control node.
    if (data->num_bytes == sizeof(control_app_switch_t))
    {
        app_lib_data_send_res_e res;
        control_app_switch_t * swtch = (control_app_switch_t *) &(data->bytes[0]);
        control_app_switch_fwd_t swtch_fwd;
        app_lib_data_to_send_t tx_switch_def =
        {
            .bytes = (const uint8_t *)&swtch_fwd,
            .num_bytes = sizeof(control_app_switch_fwd_t),
            /* Propagate the travel time of the packet. */
            .delay = data->delay,
            .tracking_id = APP_LIB_DATA_NO_TRACKING_ID,
            .qos = APP_LIB_DATA_QOS_NORMAL,
            /* This packet will only be received by CSMA nodes. */
            .flags = APP_LIB_DATA_SEND_FLAG_UNACK_CSMA_CA,
            .src_endpoint = SRC_EP_SWITCH,
            .dest_endpoint = DEST_EP_SWITCH,
            .dest_address = APP_ADDR_BROADCAST
        };

        LOG(LVL_INFO, "Received Switch %s Event (delay:%d).",
                      swtch->button_pressed?"On":"Off",
                      COARSE_TO_MS(data->delay));
        m_light_state = swtch->button_pressed;

        swtch_fwd.src_addr = data->src_address;
        memcpy(&swtch_fwd.pkt, swtch, sizeof(control_app_switch_t));

        /* Send packet to light groups. */
        res = Shared_Data_sendData(&tx_switch_def, NULL);
        if (res != APP_LIB_DATA_SEND_RES_SUCCESS)
        {
            LOG(LVL_ERROR, "Error sending data (res:%d)", res);
        }
    }

    // Switch event forwarded by a control router.
    else if (data->num_bytes == sizeof(control_app_switch_fwd_t))
    {
        control_app_switch_fwd_t * swtch_fwd =
                                (control_app_switch_fwd_t *) &(data->bytes[0]);

        LOG(LVL_INFO, "Received a forwarded Switch %s Event (delay:%d).",
                      swtch_fwd->pkt.button_pressed?"On":"Off",
                      COARSE_TO_MS(data->delay));
        m_light_state = swtch_fwd->pkt.button_pressed;
    }
    else
    {
        LOG(LVL_ERROR, "Invalid packet length.");
        return APP_LIB_DATA_RECEIVE_RES_HANDLED;
    }

    // Light delay already expired turn on/off the light immediately.
    if (COARSE_TO_MS(data->delay) >= MAX_LIGHT_TURN_ON_DELAY_MS)
    {
        LOG(LVL_INFO, "Turn %s light immediately.",
                      m_light_state?"On":"Off");
        Led_set(0, m_light_state);
    }
    else
    {
        app_scheduler_res_e res;

        // Exec task in 250ms - travel_time.
        res = App_Scheduler_addTask_execTime(
                        light_task,
                        MAX_LIGHT_TURN_ON_DELAY_MS - COARSE_TO_MS(data->delay),
                        LIGHT_TASK_EXEC_TIME_US);
        if (res != APP_SCHEDULER_RES_OK)
        {
            LOG(LVL_ERROR, "Error adding Task (res:%d)", res);
        }
    }

    return APP_LIB_DATA_RECEIVE_RES_HANDLED;
}

/**
 * \brief       Generate response to control node. Send App config.
 * \param[in]   in
 *              Information about received advertiser packet
 * \param[out]  out
 *              Generated USER data for acknowledgement
 * \return      true: send acknowledgement, false otherwise.
 */
static bool acklistener_cb(const ack_gen_input_t * in,
                           ack_gen_output_t * out)
{
    if (m_received_config)
    {
        LOG(LVL_INFO, "Generate ACK");
        out->data = (void *) &m_ack_config;
        out->length = sizeof(control_app_ack_t);

        return true;
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

    app_res = lib_settings->setNodeRole(APP_LIB_SETTINGS_ROLE_AUTOROLE_LL);

    if (app_res != APP_RES_OK)
    {
        LOG(LVL_ERROR, "Error setting node role (res:%d)", app_res);
        return;
    }

    Led_init();

    ctrl_ret = Control_Router_init(&conf);
    if (ctrl_ret != CONTROL_RET_OK)
    {
        LOG(LVL_ERROR, "Error initializing control library (ret:%d)",
                       ctrl_ret);
    }

    app_res = Shared_Data_addDataReceivedCb(&m_switch_received_item);
    if (app_res != APP_RES_OK)
    {
        LOG(LVL_ERROR, "Error adding Data callback (res:%d)", app_res);
    }

    app_cfg_res = Shared_Appconfig_addFilter(&m_da_appconfig_item,
                                             &m_filter_id);
    if (app_cfg_res != SHARED_APP_CONFIG_RES_OK)
    {
        LOG(LVL_ERROR, "Error adding AppConfig filter (res:%d)", app_cfg_res);
    }

    m_light_state = false;
    m_received_config = false;

    /*
     * Start the stack.
     * This is really important step, otherwise the stack will stay stopped and
     * will not be part of any network. So the device will not be reachable
     * without reflashing it
     */
    lib_state->startStack();
}
