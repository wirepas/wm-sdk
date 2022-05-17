/* Copyright 2020 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

#include "shared_offline.h"
#include "app_scheduler.h"
#include "api.h"

#include <string.h>

#define DEBUG_LOG_MODULE_NAME "SHARED_OFF"
#ifdef DEBUG_SHARED_OFFLINE_LOG_MAX_LEVEL
#define DEBUG_LOG_MAX_LEVEL DEBUG_SHARED_OFFLINE_LOG_MAX_LEVEL
#else
#define DEBUG_LOG_MAX_LEVEL LVL_NOLOG
#endif
#include "debug_log.h"

/** Structure of a task */
typedef struct
{
    bool                     in_use; /* Is this in use */
    offline_setting_conf_t   cbs; /* Cbs for this module */
    uint32_t                 online_deadline_ts; /* online deadline timestamp */
    bool                     ready_to_enter_offline;
} module_t;

typedef enum
{
    STATE_ONLINE,
    STATE_ENTERING_OFFLINE,
    STATE_OFFLINE
} offline_state_e;

static offline_state_e m_offline_state = STATE_ONLINE;

static bool m_delayed_wakeup = false;

/**  List of registered moduled */
static module_t m_modules[SHARED_OFFLINE_MAX_MODULES];

/**  Is library initialized */
static bool m_initialized = false;

/**  When did we enter sleep last time */
static uint32_t m_last_enterring_offline_ts;

static uint32_t m_next_online_ts;

/** Arbitrary time to sleep. Stack has a limit of 7 days */
#define TIME_TO_SLEEP 6*24*3600

/** Delay in ms to check if stack is really entered offline */
#define POLING_INTERVAL_OFFLINE_MS 1000

/**
 * \brief   Callback called when stack enters online
 */
static void on_stack_online_cb(void)
{
    uint32_t now = lib_time->getTimestampS();

    Sys_enterCriticalSection();

    m_offline_state = STATE_ONLINE;

    // Stack entering online state, time to notify all modules
    // But before it, reset every modules readiness to enter offline
    // They will all have to enter it again explicitly
    for (uint8_t i = 0; i < SHARED_OFFLINE_MAX_MODULES; i++)
    {
        if (m_modules[i].in_use)
        {
            m_modules[i].ready_to_enter_offline = false;
            if (m_modules[i].cbs.on_online_event != NULL)
            {
                uint32_t delay;
                uint32_t deadline = m_modules[i].online_deadline_ts;
                if ( deadline == SHARED_OFFLINE_INFINITE_DELAY)
                {
                    // Module didn't have any deadline
                    delay = SHARED_OFFLINE_INFINITE_DELAY;
                }
                else if ( deadline < now)
                {
                    /* Should never happen that we miss a deadline, but just
                     * in case of rounding */
                    delay = 0;
                }
                else
                {
                    delay = deadline - now;
                }
                m_modules[i].cbs.on_online_event(delay);
            }
        }
    }

    Sys_exitCriticalSection();
}

/**
 * \brief  Call to notify the modules that we are online
 */
static uint32_t wakeup_stack_task()
{
    if (m_offline_state == STATE_ENTERING_OFFLINE)
    {
        // Wakeup will happen latter from polling task
        LOG(LVL_DEBUG, "Wakeup asked when enterring offline");
        m_delayed_wakeup = true;
        return APP_SCHEDULER_STOP_TASK;
    }

    if (lib_sleep->wakeupStack() != APP_RES_OK)
    {
        // Something went wrong, waking up stack while it was not offline
        LOG(LVL_ERROR, "Waking up stack failed. Current state is %d", lib_sleep->getSleepState());
    }
    return APP_SCHEDULER_STOP_TASK;
}

static void call_offline_cbs()
{
    uint32_t now = lib_time->getTimestampS();
    uint32_t delay_s;
    if (m_next_online_ts < now)
    {
        // It took more time to enter sleep state than the expected
        // sleep period
        delay_s = 0;
    }
    else
    {
        delay_s = m_next_online_ts - now;
    }

    Sys_enterCriticalSection();
    for (uint8_t i = 0; i < SHARED_OFFLINE_MAX_MODULES; i++)
    {
        if (m_modules[i].in_use
            && m_modules[i].cbs.on_offline_event != NULL)
        {
            m_modules[i].cbs.on_offline_event(delay_s);
        }
    }

    Sys_exitCriticalSection();
}

static uint32_t poll_for_offline()
{
    if (lib_sleep->getSleepState() == APP_LIB_SLEEP_STARTED
        && m_offline_state == STATE_ENTERING_OFFLINE)
    {
        LOG(LVL_DEBUG, "Polling, in offline");
        m_offline_state = STATE_OFFLINE;

        // Time to call the offline cbs
        call_offline_cbs();

        if (m_delayed_wakeup)
        {
            LOG(LVL_DEBUG, "Execute wakeup asked when entering offline");
            // A delayed wakeup was asked while we were entering sleep
            // so it was not executed.
            // Executing it now as we are really offline now
            wakeup_stack_task();
            m_delayed_wakeup = false;
        }
        return APP_SCHEDULER_STOP_TASK;
    }

    LOG(LVL_DEBUG, "Polling, still waiting for offline");
    // Recheck in 1 second
    return POLING_INTERVAL_OFFLINE_MS;
}

/**
 * \brief   Function to enter offline.
 *          This lib manages its own scheduling and will wakeup stack
 *          asynchronously when required
 */
static bool enter_offline_mode_locked(uint32_t delay_s)
{
    uint32_t now = lib_time->getTimestampS();
    bool res;
    LOG(LVL_INFO, "Enter offline for %d s", delay_s);
    // Schedule task to wakeup stack
    if (App_Scheduler_addTask_execTime(wakeup_stack_task,
                                       delay_s * 1000,
                                       100)
            != APP_SCHEDULER_RES_OK)
    {
        return false;
    }

    // Update next wakeup variable
    m_next_online_ts = now + delay_s;

    if (lib_sleep->getSleepState() == APP_LIB_SLEEP_STARTED)
    {
        LOG(LVL_INFO, "Stack already offline, nothing more to do");
        // Stack was already in offline mode
        // nothing else to do
        return true;
    }

    // Explicitly ask for a very long period. This lib manages its own
    // scheduling and will wakeup stack asynchronously when required
    res = lib_sleep->sleepStackforTime(TIME_TO_SLEEP, 0) == APP_RES_OK;

    // Call offline cbs only when we are really in offline,
    // it will happen from stack cbs in next release but for now,
    // it requires a polling task
    if (res)
    {
        m_last_enterring_offline_ts = now;
        m_offline_state = STATE_ENTERING_OFFLINE;
        LOG(LVL_DEBUG, "Start polling for state");
        App_Scheduler_addTask_execTime(poll_for_offline,
                                       POLING_INTERVAL_OFFLINE_MS,
                                       100);
    }
    else
    {
        LOG(LVL_ERROR, "Cannot enter sleep %d", res);
    }

    return res;
}

static bool enter_online_mode()
{
    m_next_online_ts = 0;
    LOG(LVL_INFO, "Enter online async");
    return App_Scheduler_addTask_execTime(wakeup_stack_task,
                                          APP_SCHEDULER_SCHEDULE_ASAP,
                                          100) == APP_SCHEDULER_RES_OK;
}

static bool is_ready_to_sleep_locked(uint32_t * next_online_deadline_ts)
{
    bool res = true;
    uint32_t deadline = SHARED_OFFLINE_INFINITE_DELAY;

    for (uint8_t i = 0; i < SHARED_OFFLINE_MAX_MODULES; i++)
    {
        if (m_modules[i].in_use)
        {
            if (!m_modules[i].ready_to_enter_offline)
            {
                // Someone is not ready to enter offline
                res = false;
                break;
            }

            // Module is ready to sleep, update next deadline
            if (m_modules[i].online_deadline_ts < deadline)
            {
                deadline = m_modules[i].online_deadline_ts;
            }
        }
    }

    // Value is valid only if res is True
    *next_online_deadline_ts = deadline;
    return res;
}

static void evaluate_sleep()
{
    uint32_t next_online_ts;
    uint32_t now = lib_time->getTimestampS();
    Sys_enterCriticalSection();

    if (is_ready_to_sleep_locked(&next_online_ts))
    {
        uint32_t delay;
        if (now > next_online_ts)
        {
            // Should never happen
            delay = 0;
        }
        else
        {
            delay = next_online_ts - now;
        }

        // We may be already sleeping but it will update the
        // wakeup delay
        enter_offline_mode_locked(delay);
    }

    Sys_exitCriticalSection();
}

shared_offline_res_e Shared_Offline_init(void)
{
    if (m_initialized)
    {
        return SHARED_OFFLINE_RES_OK;
    }

    for (uint8_t i = 0; i < SHARED_OFFLINE_MAX_MODULES; i++)
    {
        m_modules[i].in_use = false;
    }

    lib_sleep->setOnWakeupCb(on_stack_online_cb);
    m_next_online_ts = 0;
    m_last_enterring_offline_ts = 0;
    m_offline_state = STATE_ONLINE;
    m_initialized = true;
    return SHARED_OFFLINE_RES_OK;
}

shared_offline_res_e Shared_Offline_register(uint8_t * id_p,
                                             offline_setting_conf_t cbs)
{
    app_res_e res;
    bool added = false;
    if (!m_initialized)
    {
        return SHARED_OFFLINE_RES_UNINITIALIZED;
    }

    // Check if NRLS is possible on this node.
    // As there is no explicit test in nrls lib, just test it ourself
    // Ask to enter sleep with invalid period. Depending on answer we
    // may determine if we have the right role. It works as role is checked
    // before values are checked
    res = lib_sleep->sleepStackforTime(0, 0);
    if (res == APP_RES_INVALID_CONFIGURATION)
    {
        // Role is wrong
        return SHARED_OFFLINE_RES_WRONG_ROLE;
    }

    Sys_enterCriticalSection();
    for (uint8_t i = 0; i < SHARED_OFFLINE_MAX_MODULES; i++)
    {
        if (!m_modules[i].in_use)
        {
            m_modules[i].in_use = true;
            memcpy(&m_modules[i].cbs, &cbs, sizeof(offline_setting_conf_t));
            m_modules[i].ready_to_enter_offline = false;
            *id_p = i;
            added = true;
            break;
        }
    }
    Sys_exitCriticalSection();

    if (added)
    {
        // If we were offline, just enter back online
        // as a new arbitrer registered
        if (lib_sleep->getSleepState() == APP_LIB_SLEEP_STARTED)
        {
            enter_online_mode();
        }
        return SHARED_OFFLINE_RES_OK;
    }
    else
    {
        return SHARED_OFFLINE_RES_NO_MORE_ROOM;
    }
}


shared_offline_res_e Shared_Offline_unregister(uint8_t id)
{
    if (!m_initialized)
    {
        return SHARED_OFFLINE_RES_UNINITIALIZED;
    }

    if (id >= SHARED_OFFLINE_MAX_MODULES || !m_modules[id].in_use)
    {
        return SHARED_OFFLINE_RES_WRONG_ID;
    }

    m_modules[id].in_use = false;

    // One of the arbitrer was removed, check if we can sleep now
    // or update our target
    evaluate_sleep();

    return SHARED_OFFLINE_RES_OK;
}

shared_offline_res_e Shared_Offline_enter_offline_state(uint8_t id,
                                                        uint32_t delay_s)
{
    uint32_t now = lib_time->getTimestampS();

    if (!m_initialized)
    {
        return SHARED_OFFLINE_RES_UNINITIALIZED;
    }

    if (id >= SHARED_OFFLINE_MAX_MODULES || !m_modules[id].in_use)
    {
        return SHARED_OFFLINE_RES_WRONG_ID;
    }

    Sys_enterCriticalSection();

    // Set deadline for this task
    if (delay_s == SHARED_OFFLINE_INFINITE_DELAY)
    {
        m_modules[id].online_deadline_ts = SHARED_OFFLINE_INFINITE_DELAY;
    }
    else
    {
        // We assume here that the 136 years wrapping is not an issue!
        // No need to test that now + delay_s may overflow
        m_modules[id].online_deadline_ts = now + delay_s;
    }

    m_modules[id].ready_to_enter_offline = true;

    evaluate_sleep();

    Sys_exitCriticalSection();

    return SHARED_OFFLINE_RES_OK;
}


shared_offline_res_e Shared_Offline_enter_online_state(uint8_t id)
{
    if (!m_initialized)
    {
        return SHARED_OFFLINE_RES_UNINITIALIZED;
    }

    if (id >= SHARED_OFFLINE_MAX_MODULES || !m_modules[id].in_use)
    {
        return SHARED_OFFLINE_RES_WRONG_ID;
    }

    enter_online_mode();

    return SHARED_OFFLINE_RES_OK;
}


shared_offline_status_e Shared_Offline_get_status(uint32_t * elapsed_s_p,
                                                  uint32_t * remaining_s_p)
{
    uint32_t now = lib_time->getTimestampS();
    if (!m_initialized)
    {
        return SHARED_OFFLINE_STATUS_UNINITIALIZED;
    }

    if (lib_sleep->getSleepState() == APP_LIB_SLEEP_STARTED)
    {
        *elapsed_s_p = now - m_last_enterring_offline_ts;
        *remaining_s_p = m_next_online_ts - now;

        return SHARED_OFFLINE_STATUS_OFFLINE;
    }
    return SHARED_OFFLINE_STATUS_ONLINE;
}
