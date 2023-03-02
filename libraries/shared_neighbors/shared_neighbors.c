/* Copyright 2020 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */
#define DEBUG_LOG_MODULE_NAME "SHARED NEIGHBORS"
#ifdef DEBUG_SHARED_NBORS_LOG_MAX_LEVEL
#define DEBUG_LOG_MAX_LEVEL DEBUG_SHARED_NBORS_LOG_MAX_LEVEL
#else
#define DEBUG_LOG_MAX_LEVEL LVL_NOLOG
#endif
#include "debug_log.h"
#include "shared_neighbors.h"

/** Internal structure of a callback for state library network beacon cb */
typedef struct
{
    /** Function is called when stack neighbor scan is stopped */
    app_lib_state_on_beacon_cb_f     cb_beacon;
} beacon_cb_t;

/**  List of callbacks */
static beacon_cb_t m_neighbor_cb[SHARED_NEIGHBORS_MAX_CB];

static void received_beacon_cb(const app_lib_state_beacon_rx_t * beacon)
{
    Sys_enterCriticalSection();
    for (uint8_t i = 0; i < SHARED_NEIGHBORS_MAX_CB; i++)
    {
        if (m_neighbor_cb[i].cb_beacon)
        {
            m_neighbor_cb[i].cb_beacon(beacon);
            LOG(LVL_DEBUG, "received_beacon_cb (id: %d)", i);
        }
    }
    Sys_exitCriticalSection();
}

app_res_e Shared_Neighbors_init(void)
{
    for (uint8_t i = 0; i < SHARED_NEIGHBORS_MAX_CB; i++)
    {
        m_neighbor_cb[i].cb_beacon = NULL;
    }

    return APP_RES_OK;
}

app_res_e Shared_Neighbors_addOnBeaconCb(app_lib_state_on_beacon_cb_f cb_beacon,
                                         uint16_t * cb_id)
{
    app_res_e res = APP_RES_RESOURCE_UNAVAILABLE;

    if (cb_beacon == NULL)
    {
        return APP_RES_INVALID_NULL_POINTER;
    }

    lib_state->setOnBeaconCb(received_beacon_cb);

    Sys_enterCriticalSection();
    for (uint8_t i = 0; i < SHARED_NEIGHBORS_MAX_CB; i++)
    {
        if (!m_neighbor_cb[i].cb_beacon)
        {
            /* One callback found */
            m_neighbor_cb[i].cb_beacon = cb_beacon;
            /* Set the id */
            *cb_id = i;
            res = APP_RES_OK;
            break;
        }
    }
    Sys_exitCriticalSection();

    if (res == APP_RES_OK)
    {
        LOG(LVL_DEBUG, "Add received Shared_Neighbors Beacon cb (id: %d)", *cb_id);
    }
    else
    {
        LOG(LVL_ERROR, "Cannot Add received Shared_Neighbors Beacon cb (id: %d)",
            *cb_id);
    }

    return res;
}

app_res_e Shared_Neighbors_removeBeaconCb(uint16_t cb_id)
{
    app_res_e res = APP_RES_OK;

    LOG(LVL_DEBUG,
        "Remove shared state beacon callback (id: %d)",
        cb_id);

    Sys_enterCriticalSection();
    if (m_neighbor_cb[cb_id].cb_beacon)
    {
        m_neighbor_cb[cb_id].cb_beacon = NULL;
    }
    else
    {
        res = APP_RES_INVALID_CONFIGURATION;
    }
    Sys_exitCriticalSection();

    if (res != APP_RES_OK)
    {
        LOG(LVL_ERROR, "Cannot remove Shared_Neighbors beacon cb %d", cb_id);
    }
    return res;
}
