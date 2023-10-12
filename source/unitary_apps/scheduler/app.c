/* Copyright 2018 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/*
 * \file    app.c
 * \brief   This file is an example on how to use the application
 *          scheduler. It contains several examples:
 *              - Single shot task (mainly for interrupt deferred work)
 *              - Periodic task (with different periods)
 *              - Simple state machine
 */

#include <stdlib.h>

#include "api.h"
#include "node_configuration.h"
#include "led.h"
#include "button.h"
#include "app_scheduler.h"

#define DEBUG_LOG_MODULE_NAME "SCHED_EX"
#define DEBUG_LOG_MAX_LEVEL LVL_INFO
#include "debug_log.h"

/*
 * \brief   This task is a periodic task running ~every 50ms
 *          Toggle the led 0
 */
static uint32_t periodic_task_50ms()
{
    Led_toggle(0);
    return 50;
}

/*
 * \brief   This task is a periodic task running ~every 500ms
 *          Toggle the led 1
 */
static uint32_t periodic_task_500ms()
{
    Led_toggle(1);
    return 500;
}

/*
 * \brief   This task is a single shot task
 *          Toggle the led 2, scheduled from button
 *          interrupt context
 */
static uint32_t single_shot_task()
{
    Led_toggle(2);
    return APP_SCHEDULER_STOP_TASK;
}


static uint32_t steps[] = {200, 200, 200, 200, 200, 200,
                           500, 500, 500, 500, 500, 500,
                           200, 200, 200, 200, 200, 200};

/**
 * \brief   This task is a really simple state machine
 *          Toggle led 3 according to steps defined
 */
static uint32_t state_machine_task()
{
    static uint8_t current_step = 0;
    uint32_t next;

    Led_toggle(3);
    next = steps[current_step++];

    if (current_step >= sizeof(steps) / sizeof(steps[0]))
    {
        // Reset the state machien
        current_step = 0;

        // Wait 2s before starting the state machine again
        next = 2000;
    }

    return next;
}

/*
 * \brief   Button handler that will trigger a led toggle one time
 */
static void button_handler(uint8_t button_id, button_event_e event)
{
    App_Scheduler_addTask_execTime(single_shot_task, APP_SCHEDULER_SCHEDULE_ASAP, 20);
    (void) button_id;
    (void) event;
}

/*
 * \brief   Button handler that will trigger the start stop of the state machine
 */
static void button_handler_state_machine(uint8_t button_id, button_event_e event)
{
    static bool started = false;
    app_scheduler_res_e res;

    (void) button_id;
    (void) event;

    if (!started)
    {
        res = App_Scheduler_addTask_execTime(state_machine_task,
                                             APP_SCHEDULER_SCHEDULE_ASAP,
                                             10);

        started = (res == APP_SCHEDULER_RES_OK);
    }
    else
    {
        res = App_Scheduler_cancelTask(state_machine_task);

        started = !(res == APP_SCHEDULER_RES_OK);
    }
}

/**
 * \brief   Initialization callback for application
 *
 * This function is called after hardware has been initialized but the
 * stack is not yet running.
 *
 */
void App_init(const app_global_functions_t * functions)
{
    // Basic configuration of the node with a unique node address
    if (configureNodeFromBuildParameters() != APP_RES_OK)
    {
        // Could not configure the node
        // It should not happen except if one of the config value is invalid
        return;
    }

    LOG_INIT();

    // Launch two periodic task with different period
    App_Scheduler_addTask_execTime(periodic_task_50ms, APP_SCHEDULER_SCHEDULE_ASAP, 5);
    App_Scheduler_addTask_execTime(periodic_task_500ms, APP_SCHEDULER_SCHEDULE_ASAP, 5);

    // Register to button 0 event that will triger a single shot task
    // Will work only if target board support buttons
    Button_register_for_event(0, BUTTON_PRESSED, button_handler);

    // Register to button 1 event that will start/stop a state machine
    // Will work only if target board support buttons
    Button_register_for_event(1, BUTTON_PRESSED, button_handler_state_machine);

    // Start the stack
    lib_state->startStack();

    LOG(LVL_INFO, "Scheduler example app started");
}
