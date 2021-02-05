/* Copyright 2020 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

#define DEBUG_LOG_MODULE_NAME "SHARED SHUTDOWN"
#ifdef DEBUG_SHARED_SYSTEM_LOG_MAX_LEVEL
#define DEBUG_LOG_MAX_LEVEL DEBUG_SHARED_SHUTDOWN_LOG_MAX_LEVEL
#else
#define DEBUG_LOG_MAX_LEVEL LVL_NOLOG
#endif
#include "debug_log.h"
#include "shared_shutdown.h"

/**
 * Maximum shared_system callbacks that can be registered at the same time
 * It is application specific
 */
#ifndef SHARED_SHUTDOWN_MAX_CB
// Must be defined from application
#error "Please define SHARED_SHUTDOWN_MAX_CB from your application makefile"
#endif

/**  List of callbacks */
static app_lib_system_shutdown_cb_f m_cb_shutdown[SHARED_SHUTDOWN_MAX_CB];

static void system_shutdown_cb(void)
{
    Sys_enterCriticalSection();
    for (uint8_t i = 0; i < SHARED_SHUTDOWN_MAX_CB; i++)
    {
        if (m_cb_shutdown[i])
        {
            LOG(LVL_DEBUG, "received shutdown cb (id: %d)", i);
            m_cb_shutdown[i]();
        }
    }
    Sys_exitCriticalSection();
}

app_res_e Shared_Shutdown_init(void)
{
    for (uint8_t i = 0; i < SHARED_SHUTDOWN_MAX_CB; i++)
    {
        m_cb_shutdown[i] = NULL;
    }

    lib_system->setShutdownCb(system_shutdown_cb);
    return APP_RES_OK;
}

app_res_e Shared_Shutdown_addShutDownCb(app_lib_system_shutdown_cb_f callback,
                                        uint16_t * cb_id)
{
    app_res_e res = APP_RES_RESOURCE_UNAVAILABLE;

    if (callback == NULL)
    {
        return APP_RES_INVALID_NULL_POINTER;
    }

    Sys_enterCriticalSection();
    for (uint8_t i = 0; i < SHARED_SHUTDOWN_MAX_CB; i++)
    {
        if (!m_cb_shutdown[i])
        {
            /* One cb found */
            m_cb_shutdown[i] = callback;
            /* Set the id */
            *cb_id = i;
            res = APP_RES_OK;
            break;
        }
    }
    Sys_exitCriticalSection();

    if (res == APP_RES_OK)
    {
        LOG(LVL_DEBUG, "Add shutdown cb (id: %d)", *cb_id);
    }
    else
    {
        LOG(LVL_ERROR, "Cannot add shutdown cb");
    }

    return res;
}

app_res_e Shared_Shutdown_removeShutDownCb(uint16_t cb_id)
{
    app_res_e res = APP_RES_OK;

    LOG(LVL_DEBUG,
        "Remove Shutdown cb (id: %d)",
        cb_id);

    Sys_enterCriticalSection();
    if (m_cb_shutdown[cb_id])
    {
        m_cb_shutdown[cb_id] = NULL;
    }
    else
    {
        res = APP_RES_INVALID_VALUE;
    }
    Sys_exitCriticalSection();

    if (res != APP_RES_OK)
    {
        LOG(LVL_ERROR, "Cannot remove shutdown cb %d", cb_id);
    }

    return res;
}
