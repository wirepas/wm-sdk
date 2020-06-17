/* Copyright 2018 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */
#include "app_scheduler.h"
#include "util.h"

#include <string.h>

/**
 * Maximum periodic task that can be registered at the same time
 * It is application specific
 */
#ifndef APP_SCHEDULER_MAX_TASKS
// Must be defined from application
#error "Please define APP_SCHEDULER_MAX_TASKS from your application makefile"
#endif

#define APP_SCHEDULER_ALL_TASKS (APP_SCHEDULER_LIBRARY_TASKS + APP_SCHEDULER_MAX_TASKS)

/**
 * Maximum time in ms for periodic work  to be scheduled due to internal
 * stack maximum delay comparison (~30 minutes). Computed in init function
 */
static uint32_t m_max_time_ms;

/** Measured time on nrf52 (13us) */
#define EXECUTION_TIME_NEEDED_FOR_SCHEDULING_US    20

/** Structure of a task */
typedef struct
{
    task_cb_f                           func; /* Cb of this task */
    app_lib_time_timestamp_coarse_t     next_ts; /* When is the next execution */
    uint32_t                            exec_time_us; /* Time needed for execution */
    bool                                updated; /* Updated in IRQ context? */
} task_t;

/**  List of tasks */
static task_t m_tasks[APP_SCHEDULER_ALL_TASKS];

/** Next task to be executed */
static task_t * m_next_task_p;

/** Forward declaration */
static uint32_t periodic_work(void);

/**
 * \brief   Get a coarse timestamp in future
 * \param   ms
 *          In how many ms is the timestamp in future
 * \return  The computed coarse timestamp
 * \note    Timestamp is rounded up. Ie, 1 ms is 1 coarse period
 *          in future
 */
static app_lib_time_timestamp_coarse_t get_timestamp(uint32_t ms)
{
    app_lib_time_timestamp_coarse_t ts;

    // Initialize timestamp to now
    ts = lib_time->getTimestampCoarse();

    // Handle the case of ms being > 2^25
    // to avoid overflow when multiplication by 128 (2^7)
    // Using uint64_t cast was an option also at the cost of
    // including additional linked library in final image to
    // handle uin64_t arithmetic
    if ((ms >> 25) != 0)
    {
        uint32_t delay_high;
        // Keep only the highest bits
        delay_high = ms & 0xfe000000;

        // Safe to first divide the delay
        ts += ((delay_high / 1000) * 128);

        // Remove highest bits
        ms &= 0x01ffffff;
    }

    ts += ((ms * 128) / 1000);

    // Ceil the value to upper boundary
    // (so in 1ms => ~7.8ms)
    if ((ms * 128) % 1000)
    {
        ts +=1;
    }

    return ts;
}

/**
 * \brief   Execute the selected task if time to do it
 */
static void perform_task(task_t * task)
{
    task_cb_f task_cb = NULL;
    uint32_t next = APP_SCHEDULER_STOP_TASK;

    if (task == NULL)
    {
        // There is an issue, no next task
        return;
    }

    // Needed to know if the task was updated during the cb execution
    task->updated = false;

    task_cb = task->func;
    if (task_cb == NULL)
    {
        return;
    }
    // Execute the task selected
    next = task_cb();

    // Update its next execution time
    Sys_enterCriticalSection();
    if (!task->updated)
    {
        // Task was not modified from IRQ or task itself during execution
        // so we can safely update task
        if (next == APP_SCHEDULER_STOP_TASK)
        {
            // Task doesn't have to be executed again
            // so safe to release it
            task->func = NULL;
        }
        else
        {
            // Compute next execution time
            task->next_ts = get_timestamp(next);
        }
    }
    Sys_exitCriticalSection();
}

/**
 * \brief   Update the next task for execution
 */
static task_t * get_next_task()
{
    task_t * next = NULL;
    for (uint8_t i = 0; i < APP_SCHEDULER_ALL_TASKS; i++)
    {
        if (m_tasks[i].func != NULL)
        {
            if (next == NULL
                || Util_isLtUint32(m_tasks[i].next_ts, next->next_ts))
            {
                // First task found or task is before the elected one
                next = &m_tasks[i];
            }
        }
    }

    return next;
}

/**
 * \brief   Schedule the next selected task
 * \param   task
 *          Selected task
 * \note    Handle the case where task is too far in future
 */
static void schedule_task(task_t * task)
{
    app_lib_time_timestamp_coarse_t now, next;
    uint32_t delay_ms, exec_time_us;

    // Check if task is not too far in future for periodic work
    next = get_timestamp(m_max_time_ms);
    exec_time_us = EXECUTION_TIME_NEEDED_FOR_SCHEDULING_US;
    if (Util_isLtUint32(task->next_ts, next))
    {
        next = task->next_ts;
        // Add extra time for scheduler itself
        exec_time_us = task->exec_time_us + EXECUTION_TIME_NEEDED_FOR_SCHEDULING_US;
    }

    now = lib_time->getTimestampCoarse();
    // Convert it to ms delay
    if (Util_isGtUint32(next, now))
    {
        delay_ms = ((next - now) * 1000) / 128;
    }
    else
    {
        // Task is already ready, schedule ASAP
        delay_ms = 0;
    }

    // Re-configure periodic task to update next execution/
    lib_system->setPeriodicCb(periodic_work,
                              delay_ms * 1000,
                              exec_time_us);
}

/**
 * \brief   Periodic callback that will run every tick or when an
 *          asynchronous action is added
 */
static uint32_t periodic_work(void)
{
    app_lib_time_timestamp_coarse_t now;

    now = lib_time->getTimestampCoarse();
    // Is it time to execute selected task?
    if (m_next_task_p != NULL
        && Util_isGtOrEqUint32(now, m_next_task_p->next_ts))
    {
        // A task is ready
        perform_task(m_next_task_p);
    }

    // Update next task
    m_next_task_p = get_next_task();
    if (m_next_task_p != NULL)
    {
        // Update periodic work according to next task
        schedule_task(m_next_task_p);
    }

    // Return time: it is only valid if schedule_task was not called.
    // schedule_task calls setPeriodicCb that will set the next execution time
    // instead of this return value per design (if setPeriodicWork is called
    // from periodic work, or from interrupt while a periodic work is executed
    // it takes the ownership of periodic work)
    return APP_LIB_SYSTEM_STOP_PERIODIC;
}

/**
 * \brief   Add task to task table
 * \param   task_p
 *          task to add
 * \return  true if task was correctly added
 */
static bool add_task_to_table(task_t * task_p)
{
    bool res = false;
    task_t * first_free = NULL;

    // Under critical section to avoid writing the same task
    Sys_enterCriticalSection();
    for (uint8_t i = 0; i < APP_SCHEDULER_ALL_TASKS; i++)
    {
        // First check if task already exist
        if (m_tasks[i].func == task_p->func)
        {
            // Task found, just update the next timestamp and exit
            m_tasks[i].next_ts = task_p->next_ts;
            m_tasks[i].updated = true;
            res = true;
            break;
        }

        // Check for first free room in case task is not found
        if (m_tasks[i].func == NULL && first_free == NULL)
        {
            first_free = &m_tasks[i];
        }
    }

    if (!res && first_free != NULL)
    {
        memcpy(first_free, task_p, sizeof(task_t));
        res = true;
    }

    Sys_exitCriticalSection();
    return res;
}

/**
 * \brief   Remove task to task table
 * \param   cb
 *          cb associated to the task
 * \return  Pointer to the removed task
 */
static task_t * remove_task_from_table(task_cb_f cb)
{
    task_t * removed_task = NULL;

    Sys_enterCriticalSection();
    for (uint8_t i = 0; i < APP_SCHEDULER_ALL_TASKS; i++)
    {
        if (m_tasks[i].func == cb)
        {
            m_tasks[i].func = NULL;
            m_tasks[i].updated = true;
            removed_task = &m_tasks[i];
            break;
        }
    }

    Sys_exitCriticalSection();

    return removed_task;
}

void App_Scheduler_init()
{
    // Maximum time to postpone the periodic work
    m_max_time_ms = lib_time->getMaxHpDelay() / 1000;
    m_next_task_p = NULL;

    for (uint8_t i = 0; i < APP_SCHEDULER_ALL_TASKS; i++)
    {
        m_tasks[i].func = NULL;
    }
}

app_scheduler_res_e App_Scheduler_addTask_execTime(task_cb_f cb,
                                                   uint32_t delay_ms,
                                                   uint32_t exec_time_us)
{
    task_t new_task;
    new_task.func = cb;
    new_task.next_ts = get_timestamp(delay_ms);
    new_task.exec_time_us = exec_time_us;

    if (!add_task_to_table(&new_task))
    {
        return APP_SCHEDULER_RES_NO_MORE_TASK;
    }

    // Check if next task must be updated
    if (m_next_task_p == NULL || Util_isLtUint32(new_task.next_ts, m_next_task_p->next_ts))
    {
        // Call our periodic work to update the next task ASAP
        // with short exec time as no task will be executed
        m_next_task_p = NULL;
        lib_system->setPeriodicCb(periodic_work,
                                  0,
                                  EXECUTION_TIME_NEEDED_FOR_SCHEDULING_US);
    }
    return APP_SCHEDULER_RES_OK;
}

app_scheduler_res_e App_Scheduler_cancelTask(task_cb_f cb)
{
    task_t * removed_task = remove_task_from_table(cb);
    if (removed_task != NULL)
    {
        if (removed_task == m_next_task_p)
        {
            // Call our periodic work to update the next task ASAP
            // with short exec time as no task will be executed
            m_next_task_p = NULL;
            lib_system->setPeriodicCb(periodic_work,
                                      0,
                                      EXECUTION_TIME_NEEDED_FOR_SCHEDULING_US);
        }
        return APP_SCHEDULER_RES_OK;
    }
    else
    {
        return APP_SCHEDULER_RES_UNKNOWN_TASK;
    }
}
