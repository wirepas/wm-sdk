/* Copyright 2019 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

#include "provisioning.h"
#include "provisioning_int.h"

#define DEBUG_LOG_MODULE_NAME "PROXY LIB"
#define DEBUG_LOG_MAX_LEVEL LVL_INFO
#include "debug_log.h"

/** Copy of the configuration passed during initialization. */
static provisioning_proxy_conf_t m_conf;
/** Joining proxy library is startet. */
static bool m_started;
/** Joining proxy library is initialized. */
static bool m_init = false;

provisioning_ret_e Provisioning_Proxy_init(provisioning_proxy_conf_t * conf)
{
    LOG(LVL_DEBUG, "%s, Configuration:",__func__);
    LOG(LVL_DEBUG, " - Tx Power (dBm) : %d", conf->tx_power);
    LOG(LVL_DEBUG, " - Payload :");
    LOG_BUFFER(LVL_DEBUG, conf->payload, conf->num_bytes);

    /* (re)Initialization is possible only if joining beacons are disabled. */
    if (m_init && m_started)
    {
        LOG(LVL_ERROR, "%s : PROV_RET_INVALID_STATE.", __func__);
        return PROV_RET_INVALID_STATE;
    }

    /* Parameters check. */
    if ((conf->payload == NULL && conf->num_bytes != 0) ||
        (conf->payload != NULL && conf->num_bytes == 0))
    {
        LOG(LVL_ERROR, "Init, PROV_RET_INVALID_PARAM");
        return PROV_RET_INVALID_PARAM;
    }

    m_started = false;
    m_init = true;

    return PROV_RET_OK;
}

provisioning_ret_e Provisioning_Proxy_start(void)
{
    LOG(LVL_INFO, "Start sending joining beacons.");

    if (!m_init || m_started)
    {
        LOG(LVL_ERROR, "%s : PROV_RET_INVALID_STATE.", __func__);
        return PROV_RET_INVALID_STATE;
    }

    app_lib_joining_beacon_tx_param_t param =
    {
        .interval = JOINING_TX_INTERVAL,
        .addr = JOINING_NETWORK_ADDRESS,
        .channel = JOINING_NETWORK_CHANNEL,
        .tx_power = m_conf.tx_power,
        .type = JOINING_BEACON_TYPE,
        .payload = m_conf.payload,
        .payload_num_bytes = m_conf.num_bytes
    };

    if (lib_joining->startJoiningBeaconTx(&param) != APP_RES_OK)
    {
        LOG(LVL_ERROR, "%s : PROV_RET_JOINING_LIB_ERROR.", __func__);
        return PROV_RET_JOINING_LIB_ERROR;
    }

    m_started = true;

    return PROV_RET_OK;
}

provisioning_ret_e Provisioning_Proxy_stop(void)
{
    LOG(LVL_INFO, "Stop sending joining beacons.");

    if (!m_init || !m_started)
    {
        LOG(LVL_ERROR, "%s : PROV_RET_INVALID_STATE.", __func__);
        return PROV_RET_INVALID_STATE;
    }

    if (lib_joining->stopJoiningBeaconTx() != APP_RES_OK)
    {
        LOG(LVL_ERROR, "%s : PROV_RET_JOINING_LIB_ERROR.", __func__);
        return PROV_RET_JOINING_LIB_ERROR;
    }

    m_started = false;

    return PROV_RET_OK;
}
