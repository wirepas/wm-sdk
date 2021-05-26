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

/**
 * Maximum shared_state callbacks that can be registered at the same time.
 * It is application specific.
 */
#ifndef SHARED_NEIGHBORS_MAX_CB
// Must be defined from application
#error "Please define SHARED_NEIGHBORS_MAX_CB from your application makefile"
#endif

/** Internal structure of a callback for state library network beacon cb */
typedef struct
{
    /** Function is called when stack neighbor scan is stopped */
    app_lib_state_on_beacon_cb_f     cb_beacon;
} beacon_cb_t;

/** Internal structure of a callback for state library network scan*/
typedef struct
{
    /** Function is called when stack neighbor scan is stopped */
    app_lib_state_on_scan_nbors_cb_f cb_scanned_neighbor;
} scan_cb_t;

/**  List of callbacks */
static beacon_cb_t m_neighbor_cb[SHARED_NEIGHBORS_MAX_CB];
static scan_cb_t m_neighbor_scan[SHARED_NEIGHBORS_MAX_CB];

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

static void received_network_scan_end()
{
    Sys_enterCriticalSection();

    for (uint8_t i = 0; i < SHARED_NEIGHBORS_MAX_CB; i++)
    {
        if (m_neighbor_scan[i].cb_scanned_neighbor)
        {
            m_neighbor_scan[i].cb_scanned_neighbor();
            LOG(LVL_DEBUG, "received_network_scan_end callback (id: %d)", i);
        }
    }
    Sys_exitCriticalSection();
}

app_res_e Shared_Neighbors_init(void)
{
    for (uint8_t i = 0; i < SHARED_NEIGHBORS_MAX_CB; i++)
    {
        m_neighbor_cb[i].cb_beacon = NULL;
        m_neighbor_scan[i].cb_scanned_neighbor = NULL;
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

app_res_e Shared_Neighbors_addScanNborsCb
                     (app_lib_state_on_scan_nbors_cb_f cb_scanned_neighbor,
                      uint16_t * cb_id)
{
    app_res_e res = APP_RES_RESOURCE_UNAVAILABLE;
    uint8_t free_slot = SHARED_NEIGHBORS_MAX_CB;

    LOG(LVL_DEBUG,
        "Shared_State_AddScanNborsCb (id: %d)",
        cb_id);

    if (cb_scanned_neighbor == NULL)
    {
        return APP_RES_INVALID_NULL_POINTER;
    }

    lib_state->setOnScanNborsCb(received_network_scan_end,
                                APP_LIB_STATE_SCAN_NBORS_ALL);

    Sys_enterCriticalSection();
    for (uint8_t i = 0; i < SHARED_NEIGHBORS_MAX_CB; i++)
    {
        if (m_neighbor_scan[i].cb_scanned_neighbor == cb_scanned_neighbor)
        {
            /* Callback already registered, just return same id again */
            *cb_id = i;
            res = APP_RES_OK;
            break;
        } else if (!m_neighbor_scan[i].cb_scanned_neighbor) {
            /* There is a free room */
            /* NB: the way it is implemented, id allocation will start from the end */
            free_slot = i;
        }
    }

    // Check if cb not already set and free room found
    if (res != APP_RES_OK && free_slot < SHARED_NEIGHBORS_MAX_CB)
    {
        /* One callback found */
        m_neighbor_scan[free_slot].cb_scanned_neighbor = cb_scanned_neighbor;
        /* Set the id */
        *cb_id = free_slot;
        res = APP_RES_OK;
    }
    Sys_exitCriticalSection();

    if (res == APP_RES_OK)
    {
        LOG(LVL_DEBUG, "Add received State ScanNborsCb (id: %d)", *cb_id);
    }
    else
    {
        LOG(LVL_ERROR, "Cannot Add received Shared_Neighbors ScanNborsCb (id: %d)",
            *cb_id);
    }

    return res;
}

app_res_e Shared_Neighbors_removeScanNborsCb(uint16_t cb_id)
{
    LOG(LVL_DEBUG,
        "Remove shared state ScanNbors cb callback (id: %d)",
        cb_id);

    app_res_e res = APP_RES_OK;

    if (cb_id >= SHARED_NEIGHBORS_MAX_CB)
    {
        /* Impossible to get id bigger than that */
        return APP_RES_INVALID_VALUE;
    }

    Sys_enterCriticalSection();
    if (m_neighbor_scan[cb_id].cb_scanned_neighbor)
    {
        m_neighbor_scan[cb_id].cb_scanned_neighbor = NULL;
    }
    else
    {
        res = APP_RES_INVALID_CONFIGURATION;
    }
    Sys_exitCriticalSection();

    if (res != APP_RES_OK)
    {
        LOG(LVL_ERROR, "Cannot remove Shared_Neighbors ScanNbors callback %d", cb_id);
    }

    return res;
}
