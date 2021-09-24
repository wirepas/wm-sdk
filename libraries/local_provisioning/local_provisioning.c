/* Copyright 2021 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/*
 * \file    local_provisioning
 * \brief   This lib is a wrapper on top of generic provisioning lib
 *          to specificaly address local provisioning need
 */

#include "api.h"
#include "app_scheduler.h"
#include "shared_data.h"
#include "provisioning.h"
#include "shared_appconfig.h"
#include "local_provisioning.h"
#include "node_configuration.h"
#include "stack_state.h"

#define DEBUG_LOG_MODULE_NAME "LOC PROV LIB"
#define DEBUG_LOG_MAX_LEVEL LVL_DEBUG
#include "debug_log.h"

/**
 * App config ID use to configure the provisioning
 */
#define PROVISIONING_TLV_TYPE 0xC4

/** Filter id registered */
static uint16_t m_filter_id;

/** Is reset node asked on reboot */
static bool m_reset_node = false;

/** Pre shared key used to encrypt/decryp provisioning messages */
static uint8_t m_psk[32];

/** Pre sharred key given by app when initializing app */
static uint8_t m_psk_init[32];

static local_provisioning_proxy_enabled_cb m_on_provisioning_proxy_enabled_cb = NULL;

/** If the library is started without a key, this key will be used
 * as a default key in local provisioning.
 * Using default key cannot be considered as a secured mechanism as
 * this default key will be public from GH.
 * Key is used instead of UNSECURE joining mode, to test only one path.
 *
 * \note Please do not modify this default key. If you need a different key
 *       please set from your app when doing the initialisation of this lib
 */
static const uint8_t default_psk_key[32] = {
    0xcc, 0x25, 0x37, 0xd1, 0x7c, 0x6c, 0x12, 0x80,
    0x16, 0xbd, 0x8a, 0xf5, 0xe9, 0xdd, 0x97, 0x28,
    0x98, 0x47, 0xd7, 0x83, 0xa5, 0xf7, 0xde, 0xec,
    0xc7, 0x0a, 0x0c, 0x31, 0xe9, 0x56, 0x42, 0xa3
    };

/**
 * Structure to define binary app config format. Depending on the received size,
 * mapping will be done.
 */
// TODO depending on possible option, a more futur proof format may be used.
// Something like enable + optional bit field (uint16_t) to tell if optional fields
// are set or not (keys,...). But with only two format for now, it should be enough
typedef struct  __attribute__((packed))
{
    uint8_t  enable_local_provisioning;  // Not using bool, just to ensure 1 byte field
} provisioning_app_config_t;

typedef struct  __attribute__((packed))
{
    uint8_t  enable_local_provisioning;  // Not using bool, just to ensure 1 byte field
    uint8_t  psk[32];
} provisioning_app_config_with_key_t;

/**
 * \brief   Select the joining beacon based on RSSI
 * \param   beacons
 *          A pointer to the first received beacon or NULL.
 * \return  Strongest beacon or NULL
 */
static const app_lib_joining_received_beacon_t * default_rssi_joining_beacon_selection(
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
 * \brief   The end provisioning callback. This function is called at the end
 *          of the provisioning process.
 * \param   result
 *          Result of the provisioning process.
 * \return  True: Apply received network parameters and reboot; False: discard
 *          data and end provisioning process.
 */
static bool end_cb(provisioning_res_e res)
{
    LOG(LVL_INFO, "Provisioning ended with result: %d", res);
    if (res != PROV_RES_SUCCESS)
    {
        LOG(LVL_ERROR, "Provisioning failed, stop stack to avoid extra power consumption");
        LOG_FLUSH(LVL_INFO);
        Stack_State_stopStack();
        return false;
    }
    return true;
}

static bool start_cb(const uint8_t * uid,
                     uint8_t uid_len,
                     provisioning_method_e method,
                     provisioning_proxy_net_param_t * param)
{
    app_res_e res;
    res = lib_settings->getEncryptionKey(param->enc_key);
    if (res != APP_RES_OK)
    {
        LOG(LVL_ERROR,"Cannot set encryption key %d", res);
        return false;
    }

    res = lib_settings->getAuthenticationKey(param->auth_key);
    if (res != APP_RES_OK)
    {
        LOG(LVL_ERROR,"Cannot set authentication key %d", res);
        return false;
    }

    res = lib_settings->getNetworkAddress(&param->net_addr);
    if (res != APP_RES_OK)
    {
        LOG(LVL_ERROR,"Cannot set network address %d", res);
        return false;
    }

    res = lib_settings->getNetworkChannel(&param->net_chan);
    if (res != APP_RES_OK)
    {
        LOG(LVL_ERROR,"Cannot set network channel %d", res);
        return false;
    }

    LOG(LVL_INFO, "Sending config to neighbor");

    return true;
}

/**
 * \brief   Application configuration reception callback
 * \param   bytes
 *          New app config data
 * \param   seq
 *          New app config data sequence number
 * \param   interval
 *          New app config data diagnostic interval, in seconds
 */
static void appConfigTLVReceivedCb(uint16_t type,
                                   uint8_t length,
                                   uint8_t * value_p)
{
    bool enabled = false;
    if (length == 0)
    {
        // App config received not specifying provisioning behavior
        // or wrong format
        // In both case, just disbale the provisioning
        LOG(LVL_WARNING, "App config without info for us", length);
        Provisioning_Proxy_stop();
        return;
    }
    else if (length == sizeof(provisioning_app_config_t))
    {
        provisioning_app_config_t *config = (provisioning_app_config_t *) value_p;
        enabled = config->enable_local_provisioning != 0;
        memcpy(m_psk, m_psk_init, sizeof(m_psk));
    }
    else if (length == sizeof(provisioning_app_config_with_key_t))
    {
        provisioning_app_config_with_key_t *config = (provisioning_app_config_with_key_t *) value_p;
        enabled = config->enable_local_provisioning != 0;
        memcpy(m_psk, config->psk, sizeof(m_psk));
    }
    else
    {
        LOG(LVL_ERROR, "App config with wrong format (length = %d)", length);
        Provisioning_Proxy_stop();
        return;
    }

    LOG(LVL_INFO,
        "New provisioning configuration local enabled: %d",
        enabled);

    if (enabled)
    {
        provisioning_proxy_conf_t conf =
        {
            .tx_power = 8,
            .payload = NULL,
            .num_bytes = 0,
            .is_local_sec_allowed = true,
            .is_local_unsec_allowed = false,
            .key = m_psk,
            .key_len = sizeof(m_psk),
            .start_cb = start_cb
        };

        // Disable first in case conf was changed
        Provisioning_Proxy_stop();
        Provisioning_Proxy_init(&conf);
        Provisioning_Proxy_start();
    }
    else
    {
        Provisioning_Proxy_stop();
    }

    if (m_on_provisioning_proxy_enabled_cb != NULL)
    {
        m_on_provisioning_proxy_enabled_cb(enabled);
    }
}

static uint32_t add_filter_for_proxy(void)
{
    shared_app_config_filter_t app_config_filter;
    // Prepare the app_config filter
    app_config_filter.type = PROVISIONING_TLV_TYPE;
    app_config_filter.cb = appConfigTLVReceivedCb;
    app_config_filter.call_cb_always = true;

    Shared_Appconfig_addFilter(&app_config_filter, &m_filter_id);
    LOG(LVL_INFO, "Filter added for TLV with id=%d", m_filter_id);

    return APP_SCHEDULER_STOP_TASK;
}

/**
 * @brief Callback when shutdown is going to happen to reset the
 *        configuration
 */
static uint32_t provisioning_start_task(void)
{
    Provisioning_start();
    return APP_SCHEDULER_STOP_TASK;
}

static void enable_proxy(void)
{
    if (!Local_provisioning_is_provisioned())
    {
        // A node that is not fully provisionned
        // cannot enter proxy mode
        return;
    }

    // add filter for proxy asynchronously in case we are not started yet
    // In fact if a sink, adding filter may trigger immediately cb and enabling
    // joining beacon will work only after stack is really started (app_init has returned)
    App_Scheduler_addTask_execTime(add_filter_for_proxy, APP_SCHEDULER_SCHEDULE_ASAP, 50);
}

/**
 * @brief Callback when shutdown is going to happen to reset the
 *        configuration
 */
static void on_stack_state_event_cb(stack_state_event_e event)
{
    if (event == STACK_STATE_STOPPED_EVENT && m_reset_node)
    {
        lib_settings->resetAll();
    }
    else if (event == STACK_STATE_STARTED_EVENT)
    {
        // Someone started the stack, enable the proxy mode
        // if possible.
        enable_proxy();
    }
}

Local_provisioning_res_e Local_provisioning_init(const uint8_t psk[32],
                                                 local_provisioning_proxy_enabled_cb on_proxy_enabled_cb)
{
    m_reset_node = false;
    if (psk == NULL)
    {
        psk = default_psk_key;
    }

    m_on_provisioning_proxy_enabled_cb = on_proxy_enabled_cb;
    // No key specified, use the default one
    memcpy(m_psk_init, psk, sizeof(m_psk_init));

    Stack_State_addEventCb(on_stack_state_event_cb);

    return LOCAL_PROVISIONING_RES_SUCCESS;
}

bool Local_provisioning_is_provisioned()
{
    app_lib_settings_net_addr_t net_add;
    app_lib_settings_net_channel_t net_chan;
    // Node is considered provisioned if keys are set and
    // network address and channels
    return lib_settings->getEncryptionKey(NULL) == APP_RES_OK &&
           lib_settings->getAuthenticationKey(NULL) == APP_RES_OK &&
           lib_settings->getNetworkAddress(&net_add) == APP_RES_OK &&
           lib_settings->getNetworkChannel(&net_chan) == APP_RES_OK;
}

Local_provisioning_res_e Local_provisioning_start_joining(local_provisioning_joining_beacon_selection_f cb)
{
    // In local provisioning, node UID can be anything as it just have to be unique localy
    uint32_t uid = getUniqueId();
    app_lib_settings_role_t my_role;

    provisioning_conf_t conf =
    {
        .nb_retry = 3,
        .timeout_s = 20, /* It is local provisioning so answer is only one hop */
        .method = PROV_METHOD_SECURED, /* Always use secure method with default key if keys not set */
        .uid = (const uint8_t *) &uid,
        .uid_len = sizeof(uid),
        .key = m_psk_init,
        .key_len = sizeof(m_psk_init),
        .end_cb = end_cb,
        // No user data cb received from local provisioning
        .user_data_cb = NULL,
        .beacon_joining_cb = default_rssi_joining_beacon_selection
    };

    if (cb != NULL)
    {
        // Overide default joining beacon callback
        conf.beacon_joining_cb = cb;
    }

    my_role = app_lib_settings_create_role(APP_LIB_SETTINGS_ROLE_HEADNODE,
                                           APP_LIB_SETTINGS_ROLE_FLAG_AUTOROLE);

    if (Local_provisioning_is_provisioned())
    {
        // Node is already provisioned, and must be unprovisioned first
        return LOCAL_PROVISIONING_RES_ALREADY_PROVISIONED;
    }
    // Give any valid config to node in order to start the stack and
    // be able to scan for beacons.
    OverrideNodeConfig(getUniqueAddress(),
                    my_role,
                    0xE68FA4,
                    18);

    if (Provisioning_init(&conf) != PROV_RET_OK)
    {
        return LOCAL_PROVISIONING_RES_WRONG_STATE;
    }

    // Before trying to start a provisioning session, stack must be started
    // Error should never happen as we override node config above
    if (Stack_State_startStack() != APP_RES_OK)
    {
        return LOCAL_PROVISIONING_RES_INTERNAL_ERROR;
    }

    // Start provisioning in 2 seconds as starting the stack will
    // triger a normal network scan by design making our first joining
    // beacon failling always
    if (App_Scheduler_addTask_execTime(provisioning_start_task, 2 * 1000, 50) != APP_SCHEDULER_RES_OK)
    {
        return LOCAL_PROVISIONING_RES_INTERNAL_ERROR;
    }
    return LOCAL_PROVISIONING_RES_SUCCESS;
}

void Local_provisioning_reset_node()
{
    if (Stack_State_isStarted())
    {
        m_reset_node = true;
        Stack_State_stopStack();
    }
    else
    {
        // Call reset cb directly as we are already in stopped state
        lib_settings->resetAll();
    }
}
