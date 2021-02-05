/* Copyright 2019 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/*
 * \file    app.c
 * \brief   This file is an application example to use the provisioning proxy
 *          library.
 */

#include "api.h"
#include "button.h"
#include "node_configuration.h"
#include "provisioning.h"

#define DEBUG_LOG_MODULE_NAME "PROXY APP"
#define DEBUG_LOG_MAX_LEVEL LVL_INFO
#include "debug_log.h"

/** Transmission power to send joining beacons, in dBm */
#define JOINING_TX_POWER        8

/** Define this for new node to be low-latency */
//#define LOW_LATENCY_NODE 1

/** Sending joining beacons is enabled or not. */
static bool beacon_tx_enabled = false;

/**
 * \brief   Button 0 callback: start or stop sending joining beacons
 * \param   button_id
 *          Number of the pressed button, unused
 * \param   event
 *          Button event, unused
 */
static void button_0_cb(uint8_t button_id, button_event_e event)
{
    (void)button_id;
    (void)event;

    /* Turn joining beacon transmission on or off. */
    if (beacon_tx_enabled)
    {
        Provisioning_Proxy_stop();
        beacon_tx_enabled = false;
    }
    else
    {
        Provisioning_Proxy_start();
        beacon_tx_enabled = true;
    }
}

/**
 * \brief   Initialization callback for application
 *
 * This function is called after hardware has been initialized but the
 * stack is not yet running.
 */
void App_init(const app_global_functions_t * functions)
{
    provisioning_proxy_conf_t conf =
    {
        .tx_power = JOINING_TX_POWER,
        .payload = NULL,
        .num_bytes = 0,
    };

    LOG_INIT();
    LOG(LVL_INFO, "Starting");

    /* Basic configuration of the node with a unique node address. */
    if (configureNodeFromBuildParameters() != APP_RES_OK)
    {
        /* Could not configure the node. It should not happen except if one
         * of the config value is invalid.
         */
        return;
    }

    /* Proxy must be configured as Headnode. */
    lib_settings->setNodeRole(
                app_lib_settings_create_role(APP_LIB_SETTINGS_ROLE_HEADNODE,
#ifdef LOW_LATENCY_NODE
                                             APP_LIB_SETTINGS_ROLE_FLAG_LL
#else
                                             0
#endif
                                            ));

    Provisioning_Proxy_init(&conf);

    Button_init();
    Button_register_for_event(0, BUTTON_PRESSED, button_0_cb);

    /* Start the stack. */
    lib_state->startStack();
}
