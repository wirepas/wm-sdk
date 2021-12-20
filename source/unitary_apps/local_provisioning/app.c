/* Copyright 2021 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/*
 * \file    app.c
 * \brief   This file is an application example to use the local_provisioning
 *          library.
 */

#include "api.h"
#include "button.h"
#include "led.h"
#include "app_scheduler.h"
#include "shared_data.h"
#include "provisioning.h"
#include "local_provisioning.h"
#include "stack_state.h"

#define DEBUG_LOG_MODULE_NAME "LOC PROV APP"
#define DEBUG_LOG_MAX_LEVEL LVL_INFO
#include "debug_log.h"

// Led id depends if node is joining or proxy
static uint8_t m_led_id = 0;

static uint32_t send_data(void)
{
    static uint32_t id = 0; // Value to send

    // Create a data packet to send
    app_lib_data_to_send_t data_to_send = {
        .bytes = (const uint8_t *) &id,
        .num_bytes = sizeof(id),
        .dest_address = APP_ADDR_ANYSINK,
        .src_endpoint = 26, // As an example
        .dest_endpoint = 26,
        .qos = APP_LIB_DATA_QOS_HIGH,
    };

    // Send the data packet
    Shared_Data_sendData(&data_to_send, NULL);

    // Increment value to send
    id++;

    return 10*1000;
}

static uint32_t blink_led(void)
{
    Led_toggle(m_led_id);
    return 500;
}

static void on_prov_proxy_enabled_cb(bool enabled)
{
    if (enabled)
    {
        App_Scheduler_addTask_execTime(blink_led, APP_SCHEDULER_SCHEDULE_ASAP, 10);
    }
    else
    {
        App_Scheduler_cancelTask(blink_led);
        Led_set(m_led_id, true);
    }
}

/**
 * \brief   Callback called at the end of the provisioning attempt
 */
static bool on_joining_end_cb(bool success)
{
    App_Scheduler_cancelTask(blink_led);
    Led_set(m_led_id, true);
    // We don't want to reboot at the end
    return false;
}

/**
 * \brief   Button 0 callback: starts the provisioning process.
 * \param   button_id
 *          Number of the pressed button, unused
 * \param   event
 *          Button event, unused
 */
static void button_start_joining_cb(uint8_t button_id, button_event_e event)
{
    if (Local_provisioning_start_joining(NULL, on_joining_end_cb) == LOCAL_PROVISIONING_RES_SUCCESS)
    {
        // Blink Led 0 to see that node is trying to join.
        // It will end automatically as it will finish with a reboot
        App_Scheduler_addTask_execTime(blink_led, APP_SCHEDULER_SCHEDULE_ASAP, 10);
    }
}

static void button_reset_cb(uint8_t button_id, button_event_e event)
{
    LOG(LVL_INFO, "Perform reset...");
    Local_provisioning_reset_node();
}

void App_init(const app_global_functions_t * functions)
{
    LOG_INIT();
    LOG(LVL_INFO, "Starting");

    Button_init();
    Led_init();
    Local_provisioning_init(NULL, on_prov_proxy_enabled_cb);

    if (Local_provisioning_is_provisioned())
    {
        m_led_id = 1;
        // Node is provisioned, enable proxy mode
        Stack_State_startStack();
        // Register button 1 to reset node config
        Button_register_for_event(1, BUTTON_PRESSED, button_reset_cb);
        // Once provisioned, this app send periodically data as a demo
        App_Scheduler_addTask_execTime(send_data, APP_SCHEDULER_SCHEDULE_ASAP, 200);
    }
    else
    {
        m_led_id = 0;
        Button_register_for_event(0, BUTTON_PRESSED, button_start_joining_cb);
    }

    Led_set(m_led_id, true);
}
