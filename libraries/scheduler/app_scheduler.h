/* Copyright 2018 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/**
 * @file app_scheduler.h
 *
 * Application scheduler library. Allows scheduling of multiple application
 * tasks concurrently.
 *
 * @note    Unlike most services, this library is safe to be used from
 *          @ref fast_interrupt "fast interrupt execution context"
 */

#ifndef _APP_SCHEDULER_H_
#define _APP_SCHEDULER_H_

#include <stdint.h>
#include <stdbool.h>
#include "api.h"

/**
 * \brief   Task callback to be registered
 * \return  Delay before being executed again in ms
 * \note    Return value in ms is aligned on system coarse timesatmp
 *          boundaries that has a 1/128s granularity. So asking 3 or 7 ms will
 *          result in same scheduling.
 *          If a better accuracy is needed, hardware timer must be used
 */
typedef uint32_t (*task_cb_f)();

/**
 * \brief   Value to return from task to remove it
 */
#define APP_SCHEDULER_STOP_TASK     ((uint32_t)(-1))

/**
 * \brief   Value to return from task or as initial time to be executed ASAP
 */
#define APP_SCHEDULER_SCHEDULE_ASAP (0)

/**
 * \brief   List of return code
 */
typedef enum
{
    /** Operation is successful */
    APP_SCHEDULER_RES_OK = 0,
    /** No more tasks available */
    APP_SCHEDULER_RES_NO_MORE_TASK = 1,
    /** Trying to cancel a task that doesn't exist */
    APP_SCHEDULER_RES_UNKNOWN_TASK = 2,
    /** Using the library without previous initialization */
    APP_SCHEDULER_RES_UNINITIALIZED = 3,
    /** Requested time is too long */
    APP_SCHEDULER_RES_TOO_LONG_EXECUTION_TIME = 4
} app_scheduler_res_e;

/**
 * \brief   Initialize scheduler
 * \note    If App scheduler is enabled in app, @ref App_Scheduler_init is
 *          automatically called before App_init
 * \note    If App scheduler is used in application, the periodicWork offered
 *          by system library MUST NOT be used outside of this module
 */
void App_Scheduler_init(void);

/**
 * \brief   Add a task
 *
 * Example on use:
 *
 * @code
 *
 * static uint32_t periodic_task_50ms()
 * {
 *     ...
 *     return 50;
 * }
 *
 * static uint32_t periodic_task_500ms()
 * {
 *     ...
 *     return 500;
 * }
 *
 * void App_init(const app_global_functions_t * functions)
 * {
 *     // Launch two periodic task with different period
 *     App_Scheduler_addTask_execTime(periodic_task_50ms, APP_SCHEDULER_SCHEDULE_ASAP, 10);
 *     App_Scheduler_addTask_execTime(periodic_task_500ms, APP_SCHEDULER_SCHEDULE_ASAP, 10);
 *     ...
 *     // Start the stack
 *     lib_state->startStack();
 * }
 * @endcode
 *
 *
 * \param   cb
 *          Callback to be called from main periodic task.
 *          Same cb can only be added once. Calling this function with an already
 *          registered cb will update the next scheduled time.
 * \param   delay_ms
 *          delay in ms to be scheduled (0 to be scheduled asap)
 * \param   exec_time_us
 *          Maximum execution time required for the task to be executed
 * \return  True if able to add, false otherwise
 */
app_scheduler_res_e App_Scheduler_addTask_execTime(task_cb_f cb, uint32_t delay_ms, uint32_t exec_time_us);

#ifdef APP_SCHEDULER_MAX_EXEC_TIME_US
/**
 * \brief   Add a task without execution time (deprecated)
 * \param   cb
 *          Callback to be called from main periodic task.
 *          Same cb can only be added once. Calling this function with an already
 *          registered cb will update the next scheduled time.
 * \param   delay_ms
 *          delay in ms to be scheduled (0 to be scheduled asap)
 * \return  True if able to add, false otherwise
 *
 * \note    This call is deprecated and you should use @ref App_Scheduler_addTask_execTime instead
 */
static inline app_scheduler_res_e App_Scheduler_addTask(task_cb_f cb, uint32_t delay_ms)
{
    return App_Scheduler_addTask_execTime(cb, delay_ms, APP_SCHEDULER_MAX_EXEC_TIME_US);
}
#endif

/**
 * \brief   Cancel a task
 * \param   cb
 *          Callback already registered from App_Scheduler_addTask.
 * \return  True if able to cancel, false otherwise (not existing)
 */
app_scheduler_res_e App_Scheduler_cancelTask(task_cb_f cb);

#endif //_APP_SCHEDULER_H_
