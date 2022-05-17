/* Copyright 2019 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/*
 * \file    app.c
 * \brief   This file is an application example to use the provisioning
 *          library.
 */

#include "api.h"
#include "button.h"
#include "node_configuration.h"
#include "app_scheduler.h"
#include "shared_data.h"
#include "provisioning.h"
#include "storage.h"

#define DEBUG_LOG_MODULE_NAME "PROV APP"
#define DEBUG_LOG_MAX_LEVEL LVL_INFO
#include "debug_log.h"

/** Define this for new node to be low-latency */
//#define LOW_LATENCY_NODE 1

/**
 * \brief   Select the joining beacon.
 * \note    In this example, the beacon with the strongest RSSI is selected.
 *          But the application can implement any algorithms.
 * \param   beacons
 *          A pointer to the first received beacon or NULL.
 * \return  Strongest beacon or NULL
 */
const app_lib_joining_received_beacon_t * select_joining_beacon(
    const app_lib_joining_received_beacon_t * beacons)
{
    const app_lib_joining_received_beacon_t * strongest_beacon = NULL;
    int16_t strongest_rssi = INT16_MIN; // RSSI is int8_t, so > INT16_MIN

    while (beacons != NULL)
    {
        if (beacons->rssi > strongest_rssi)
        {
            strongest_beacon = beacons;
            strongest_rssi = beacons->rssi;
        }
        beacons = beacons->next;
    }

    return strongest_beacon;
}

/**
 * \brief   Callback for received User provisioning data.
 *          Provisioning data is received as a map of id:data. This function
 *          is callback for each id that are not reserved by Wirepas.
 * \param   id
 *          Id of the received item.
 * \param   data
 *          Received data.
 * \param   len
 *          Length of the data.
 */
void user_data_cb(uint32_t id, CborType type, uint8_t * data, uint8_t len)
{
    LOG(LVL_INFO, "User Data, id:%d, type:%d, data:",id, type);
    LOG_BUFFER(LVL_DEBUG, data, len);

    return;
}

/**
 * \brief   The end provisioning callback. This function is called at the end
 *          of the provisioning process.
 * \param   result
 *          Result of the provisioning process.
 * \return  True: Apply received network parameters and reboot; False: discard
 *          data and end provisioning process.
 */
bool end_cb(provisioning_res_e res)
{
    LOG(LVL_INFO, "Provisioning ended with result: %d", res);
    return true;
}

/**
 * \brief   Button 0 callback: starts the provisioning process.
 * \param   button_id
 *          Number of the pressed button, unused
 * \param   event
 *          Button event, unused
 */
void button_0_cb(uint8_t button_id, button_event_e event)
{
    Provisioning_start();
}


void App_init(const app_global_functions_t * functions)
{
    app_addr_t addr;
    app_lib_settings_net_addr_t net_addr;
    app_lib_settings_net_channel_t net_channel;
    int8_t uid_len, key_len;
    provisioning_method_e method;
    provisioning_conf_t conf =
    {
        .nb_retry = 3,
#ifdef LOW_LATENCY_NODE
        .timeout_s = 20,
#else
        .timeout_s = 120,
#endif
        /* Method, Unique ID and shared key are loaded from storage module.
         * .method = ,
         * .uid = ,
         * .uid_len = ,
         * .key = ,
         * .key_len = ,
         */
        .end_cb = end_cb,
        .user_data_cb = user_data_cb,
        .beacon_joining_cb = select_joining_beacon
    };

    LOG_INIT();
    LOG(LVL_INFO, "Starting");

    /* Basic configuration of the node with a unique node address. */
    if (configureNodeFromBuildParameters() != APP_RES_OK)
    {
        /* Could not configure the node. It should not happen except if one of
         * the config value is invalid.
         */
        return;
    }

    /* Configure new node. */
#ifdef LOW_LATENCY_NODE
    lib_settings->setNodeRole(APP_LIB_SETTINGS_ROLE_SUBNODE_LL);
#else
    lib_settings->setNodeRole(APP_LIB_SETTINGS_ROLE_SUBNODE_LE);
#endif

    /* Print actual node config. */
    LOG(LVL_DEBUG, "Node network parameters:");
    lib_settings->getNodeAddress(&addr);
    lib_settings->getNetworkAddress(&net_addr);
    lib_settings->getNetworkChannel(&net_channel);
    LOG(LVL_DEBUG, "  - node addr: %08X", addr);
    LOG(LVL_DEBUG, "  - net addr:  %06X", net_addr);
    LOG(LVL_DEBUG, "  - net ch:    %d", net_channel);

    Button_init();
    Button_register_for_event(0, BUTTON_PRESSED, button_0_cb);

    if (!Storage_init())
    {
        LOG(LVL_ERROR, "Error opening provisioning storage.");
    }
    else
    {
        uid_len = Storage_getUID(&conf.uid);
        key_len = Storage_getKey(&conf.key);
        method = Storage_getMethod();

        if (uid_len != -1 && key_len != -1)
        {
            conf.uid_len = uid_len;
            conf.key_len = key_len;
            conf.method = method;
            Provisioning_init(&conf);
        }
        else
        {
            LOG(LVL_ERROR, "Can't retrieve node UID or provisioning Key.");
        }
    }

    /* Start the stack */
    lib_state->startStack();
}
