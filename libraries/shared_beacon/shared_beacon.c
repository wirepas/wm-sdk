/* Copyright 2020 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */
#define DEBUG_LOG_MODULE_NAME "SHARED BEACONTX"
#ifdef DEBUG_SHARED_BEACONTX_LOG_MAX_LEVEL
#define DEBUG_LOG_MAX_LEVEL DEBUG_SHARED_BEACONTX_LOG_MAX_LEVEL
#else
#define DEBUG_LOG_MAX_LEVEL LVL_NOLOG
#endif
#include "debug_log.h"
#include <stdio.h>
#include <string.h>
#include "shared_beacon.h"

/** Internal structure of a shared beacon */
typedef struct
{
    uint16_t                    interval_ms; /* Requested interval in ms */
    bool                        in_use;      /* Is the shared beacon used */
} shared_beacon_t;

/** check that initialization is done */
static bool m_init_done = false;
/** check for first time beacon tx rf enable */
static bool m_beacons_enabled;
/** table used to see which indexes are reserved and list intervals requested.
 * No need to make it configurable at build time as it will be max 8*3
 */
static shared_beacon_t m_beacon_index[APP_LIB_BEACON_TX_MAX_INDEX+1];
/** used common beacon tx sending interval for all enabled beacons */
static uint32_t m_min_interval_used_ms;

shared_beacon_res_e Shared_Beacon_init(void)
{
    LOG(LVL_INFO, "Shared_Beacon_init \n");
    if (m_init_done)
    {
        // Initialization already done
        return SHARED_BEACON_RES_OK;
    }
    memset(&m_beacon_index[0], 0, sizeof(m_beacon_index));
    m_init_done = true;
    m_beacons_enabled = false;
    m_min_interval_used_ms = APP_LIB_BEACON_TX_MAX_INTERVAL;
    /** clearBeacon returns always APP_RES_OK */
    lib_beacon_tx->clearBeacons();
    return SHARED_BEACON_RES_OK;
}

shared_beacon_res_e Shared_Beacon_startBeacon(uint16_t interval_ms,
                                              int8_t * power,
                                              app_lib_beacon_tx_channels_mask_e
                                              channels_mask,
                                              const uint8_t * content,
                                              uint8_t length,
                                              uint8_t * shared_beacon_index)
{
    bool found = false;
    uint8_t n;
    app_res_e res = APP_RES_OK;

    if (!m_init_done)
    {
        LOG(LVL_ERROR, "Shared_Beacon_startBeacon error !m_init_done \n");
        return SHARED_BEACON_INIT_NOT_DONE;
    }

    Sys_enterCriticalSection();

    for (n = 0; n <= APP_LIB_BEACON_TX_MAX_INDEX; n++)
    {
        if (!m_beacon_index[n].in_use)
        {
            found = true;
            m_beacon_index[n].in_use = true;
            break;
        }
    }

    if (!found)
    {
        LOG(LVL_ERROR, "Shared_Beacon_startBeacon-no free index \n");
        Sys_exitCriticalSection();
        return SHARED_BEACON_INDEX_NOT_AVAILABLE;
    }

    res |= lib_beacon_tx->setBeaconPower(n, power);
    res |= lib_beacon_tx->setBeaconChannels(n, channels_mask);
    res |= lib_beacon_tx->setBeaconContents(n, content, length);

    if (res != APP_RES_OK)
    {
        LOG(LVL_ERROR, "Shared_Beacon_startBeacon-lib_beacon_tx error: %d\n",
            res);
        m_beacon_index[n].in_use = false;
        Sys_exitCriticalSection();
        return SHARED_BEACON_INVALID_PARAM;
    }

   /** If multiple beacon tx instances - there is selected shortest interval */
    if (interval_ms < m_min_interval_used_ms)
    {
        m_min_interval_used_ms = interval_ms;
    }

    /** Save requested interval for later check if beacons are stopped */
    m_beacon_index[n].interval_ms = interval_ms;

    res |=
        lib_beacon_tx->setBeaconInterval(m_min_interval_used_ms);
    *shared_beacon_index = n;

    if (!m_beacons_enabled)
    {
        m_beacons_enabled = true;
        res |= lib_beacon_tx->enableBeacons(true);
    }

    LOG(LVL_INFO, "Shared_Beacon_startBeacon, interval ms: %d, index %d\n",
        m_min_interval_used_ms, n);
    Sys_exitCriticalSection();
    return SHARED_BEACON_RES_OK;
}

/**
 * @brief   Selects the smallest requested interval among active beacons.
 * @return  interval in ms
 */
static uint32_t smallest_interval(void)
{
    uint8_t n;
    uint32_t interval_ms = APP_LIB_BEACON_TX_MAX_INTERVAL;

    for (n = 0; n <= APP_LIB_BEACON_TX_MAX_INDEX; n++)
    {
        if (m_beacon_index[n].in_use &&
           (m_beacon_index[n].interval_ms < interval_ms))
        {
            interval_ms = m_beacon_index[n].interval_ms;
        }
    }

    return interval_ms;
}

shared_beacon_res_e Shared_Beacon_stopBeacon(uint8_t shared_beacon_index)
{
    app_res_e res;

    if (!m_init_done)
    {
        LOG(LVL_ERROR, "Shared_Beacon_stopBeacon - init not done");
        return SHARED_BEACON_INIT_NOT_DONE;
    }

    if (shared_beacon_index > APP_LIB_BEACON_TX_MAX_INDEX)
    {
        LOG(LVL_ERROR, "Shared_Beacon_stopBeacon error wrong parametere");
        return SHARED_BEACON_INVALID_PARAM;
    }

    Sys_enterCriticalSection();

    if (!m_beacon_index[shared_beacon_index].in_use)
    {
        Sys_exitCriticalSection();
        LOG(LVL_ERROR, "Shared_Beacon_stopBeacon error index not available");
        return SHARED_BEACON_INDEX_NOT_AVAILABLE;
    }

    /** Writing NULL, 0 to index disables beacon tx for that given index as
     *  no lib_beacon_tx interface to disable specially only one instance
     *  if multiple instances configured
     */
    LOG(LVL_INFO, "Shared_Beacon_stopBeacon, shared_beacontx_index: %d\n",
            shared_beacon_index);
    res =
        lib_beacon_tx->setBeaconContents(shared_beacon_index, NULL, 0);
    m_beacon_index[shared_beacon_index].in_use = false;

    /** Selects new interval from active beacons  */
    m_min_interval_used_ms = smallest_interval();
    lib_beacon_tx->setBeaconInterval(m_min_interval_used_ms);
    LOG(LVL_INFO, "Shared_Beacon_stopBeacon, m_min_interval_used_ms: %u\n",
            m_min_interval_used_ms);

    Sys_exitCriticalSection();

    if (res != APP_RES_OK)
    {
        LOG(LVL_ERROR, "Shared_Beacon_stopBeacon - error index: %d\n",
            shared_beacon_index);
        return SHARED_BEACON_INDEX_NOT_ACTIVE;
    }

    return SHARED_BEACON_RES_OK;
}
