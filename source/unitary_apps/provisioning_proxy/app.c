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
#include "shared_data.h"

#define DEBUG_LOG_MODULE_NAME "PROXY APP"
#define DEBUG_LOG_MAX_LEVEL LVL_INFO
#include "debug_log.h"

/** Transmission power to send joining beacons, in dBm */
#define JOINING_TX_POWER        8

/** Define this for new node to be low-latency */
//#define LOW_LATENCY_NODE 1

/** Define this to true to enable local provisioning. */
#define ENABLE_LOCAL_PROVISIONING false

/** Network authentication key. */
static const uint8_t authentication_key[] =  {NET_AUTHEN_KEY};

/** Network encryption key. */
static const uint8_t encryption_key[] =  {NET_CIPHER_KEY};

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
 * \brief   Always accept the new node. Send the same network parameters than
 *          what is configured for the proxy.
 * \param   uid
 *          A pointer to the node UID.
 * \param   uid_len
 *          The size in bytes of the UID
 * \param   method
 *          The provisioning method requested by the new node.
 * \param   param
 *          If returning true, the callback must fill this structure with the
 *          network parameters that will be sent to the new node.
 * \return  true: Send provisioning data to this node; false: discard the node
 *          request and reply with a NACK.
 */
bool start_cb(const uint8_t * uid,
              uint8_t uid_len,
              provisioning_method_e method,
              provisioning_proxy_net_param_t * param)
{
    /* The proxy will send the same network parameters to the new node than
     * the ones it is using.
     */
    param->net_addr = NETWORK_ADDRESS;
    param->net_chan = NETWORK_CHANNEL;
    memcpy(param->auth_key,
           authentication_key,
           APP_LIB_SETTINGS_AES_KEY_NUM_BYTES);
    memcpy(param->enc_key, encryption_key, APP_LIB_SETTINGS_AES_KEY_NUM_BYTES);

    return true;
}


/**
 * \brief   Initialization callback for application
 *
 * This function is called after hardware has been initialized but the
 * stack is not yet running.
 */
void App_init(const app_global_functions_t * functions)
{
    /* Factory key used for secured method if local provisioning is enabled. */
    static const uint8_t factory_key[] = {
                            0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                            0x08, 0X09, 0X0A, 0X0B, 0X0C, 0X0D, 0X0E, 0X0F,
                            0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                            0x08, 0X09, 0X0A, 0X0B, 0X0C, 0X0D, 0X0E, 0X0F
                            };
    provisioning_proxy_conf_t conf =
    {
        .tx_power = JOINING_TX_POWER,
        .payload = NULL,
        .num_bytes = 0,
        .is_local_sec_allowed = ENABLE_LOCAL_PROVISIONING,
        .is_local_unsec_allowed = ENABLE_LOCAL_PROVISIONING,
        .key = factory_key,
        .key_len = 32,
        .start_cb = start_cb
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
    /* Configure new node. */
#ifdef LOW_LATENCY_NODE
    lib_settings->setNodeRole(APP_LIB_SETTINGS_ROLE_HEADNODE_LL);
#else
    lib_settings->setNodeRole(APP_LIB_SETTINGS_ROLE_HEADNODE_LE);
#endif

    Provisioning_Proxy_init(&conf);

    Button_init();
    Button_register_for_event(0, BUTTON_PRESSED, button_0_cb);

    /* Start the stack. */
    lib_state->startStack();
}
