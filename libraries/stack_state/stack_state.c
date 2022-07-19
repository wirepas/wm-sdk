/* Copyright 2021 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

#define DEBUG_LOG_MODULE_NAME "STACK_STATE"
#ifdef DEBUG_STACK_STATE_LOG_MAX_LEVEL
#define DEBUG_LOG_MAX_LEVEL DEBUG_STACK_STATE_LOG_MAX_LEVEL
#else
#define DEBUG_LOG_MAX_LEVEL LVL_NOLOG
#endif
#include "debug_log.h"
#include "stack_state.h"


/* Note: It may happen that CB are not needed even if library is used to start/
 *  stop the stack. That's why most of the code is under:
 *  #if STACK_STATE_CB != 0
 *  ...
 *  #endif
 */

static bool m_initialized = false;

#if STACK_STATE_CB != 0
/**  List of callbacks */
static stack_state_event_cb_f m_stack_state_cbs[STACK_STATE_CB];

static void notify_modules(stack_state_event_e event)
{
    Sys_enterCriticalSection();
    for (uint8_t i = 0; i < STACK_STATE_CB; i++)
    {
        if (m_stack_state_cbs[i] != NULL)
        {
            m_stack_state_cbs[i](event);
        }
    }
    Sys_exitCriticalSection();
}

static void system_shutdown_cb(void)
{
    LOG(LVL_DEBUG, "received shutdown cb");
    notify_modules(STACK_STATE_STOPPED_EVENT);
}
#endif

app_res_e Stack_State_init(void)
{
    if (m_initialized)
    {
        LOG(LVL_DEBUG, "Stack_State_init: already initialized)");
        return APP_RES_OK;
    }

#if STACK_STATE_CB != 0
    for (uint8_t i = 0; i < STACK_STATE_CB; i++)
    {
        m_stack_state_cbs[i] = NULL;
    }


    lib_system->setShutdownCb(system_shutdown_cb);
#endif
    LOG(LVL_DEBUG, "Stack_State_init (%d)", STACK_STATE_CB);
    m_initialized = true;
    return APP_RES_OK;
}

app_res_e Stack_State_startStack()
{
    if (!m_initialized)
    {
        return APP_RES_INVALID_VALUE;
    }
    app_res_e res = lib_state->startStack();
#if STACK_STATE_CB != 0
    if (res == APP_RES_OK)
    {
        // Notifying other registered module from here as
        // stack doesn't have a callback for that
        notify_modules(STACK_STATE_STARTED_EVENT);
    }
#endif
    return res;
}

app_res_e Stack_State_stopStack()
{
    if (!m_initialized)
    {
        return APP_RES_INVALID_VALUE;
    }
    return lib_state->stopStack();
}

bool Stack_State_isStarted()
{
    return lib_state->getStackState() == APP_LIB_STATE_STARTED;
}

app_res_e Stack_State_addEventCb(stack_state_event_cb_f callback)
{
    if (!m_initialized)
    {
        return APP_RES_INVALID_VALUE;
    }
#if STACK_STATE_CB != 0
    app_res_e res = APP_RES_RESOURCE_UNAVAILABLE;
    int free_slot = -1;

    if (callback == NULL)
    {
        return APP_RES_INVALID_NULL_POINTER;
    }

    Sys_enterCriticalSection();
    for (uint8_t i = 0; i < STACK_STATE_CB; i++)
    {
        if (m_stack_state_cbs[i] == NULL)
        {
            /* One free room found */
            free_slot = i;
            continue;
        }
        else if (m_stack_state_cbs[i] == callback)
        {
            /* Callback already present */
            res = APP_RES_OK;
            break;
        }
    }

    if (res != APP_RES_OK && free_slot >= 0)
    {
        /* Callback was not already present and a free room was found */
        m_stack_state_cbs[free_slot] = callback;
        res = APP_RES_OK;
    }
    Sys_exitCriticalSection();

    if (res == APP_RES_OK)
    {
        LOG(LVL_DEBUG, "Add stack state event cb (0x%x)", callback);
    }
    else
    {
        LOG(LVL_ERROR, "Cannot add stack state event cb (0x%x)", callback);
    }
    return res;
#else
    return APP_RES_RESOURCE_UNAVAILABLE;
#endif
}

app_res_e Stack_State_removeEventCb(stack_state_event_cb_f callback)
{
    if (!m_initialized)
    {
        return APP_RES_INVALID_VALUE;
    }
#if STACK_STATE_CB != 0
    app_res_e res = APP_RES_INVALID_VALUE;

    LOG(LVL_DEBUG, "Removing event cb (0x%x)", callback);

    Sys_enterCriticalSection();
    for (uint8_t i = 0; i < STACK_STATE_CB; i++)
    {
        if (m_stack_state_cbs[i] == callback)
        {
            m_stack_state_cbs[i] = NULL;
            res = APP_RES_OK;
        }
    }
    Sys_exitCriticalSection();

    if (res != APP_RES_OK)
    {
        LOG(LVL_ERROR, "Cannot remove event cb (0x%x)", callback);
    }

    return res;
#else
    return APP_RES_INVALID_VALUE;
#endif
}
