/* Copyright 2021 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 */

/*
 * \file    app.c
 * \brief
 */

#include <stdlib.h>

#include "api.h"
#include "node_configuration.h"
#include "app_scheduler.h"
#include "button.h"
#include "led.h"

// ID of the persistent area from nrf52840_app.ini file
#define APP_PERSISTENT_MEMORY_AREA_ID 0x8AE573BA

// Exagerated timeout (1s) for flash access to avoid infinite loop
#define FLASH_BUSY_TIMEOUT_US   (1 * 1000 * 1000)

#define DEBUG_LOG_MODULE_NAME "BLE_DUAL_BOOT"
#define DEBUG_LOG_MAX_LEVEL LVL_INFO

#include "debug_log.h"

static bool active_wait_for_end_of_operation(int32_t timeout_us)
{
    app_lib_time_timestamp_hp_t timeout;
    timeout = lib_time->addUsToHpTimestamp(lib_time->getTimestampHp(),
                                           timeout_us);

    while (lib_memory_area->isBusy(APP_PERSISTENT_MEMORY_AREA_ID) &&
           lib_time->isHpTimestampBefore(lib_time->getTimestampHp(), timeout));

    // Successful if not busy anymore (no timeout).
    return !lib_memory_area->isBusy(APP_PERSISTENT_MEMORY_AREA_ID);
}

static bool switch_back_to_second_app()
{
    uint32_t sector_base = 0;
    size_t num_block = 1;
    if (lib_memory_area->startErase(APP_PERSISTENT_MEMORY_AREA_ID, &sector_base, &num_block) 
        != APP_LIB_MEM_AREA_RES_OK)
    {
        LOG(LVL_ERROR, "Cannot switch back");
        return false;
    }

    if (!active_wait_for_end_of_operation(FLASH_BUSY_TIMEOUT_US))
    {
        LOG(LVL_ERROR, "Cannot switch back (Flash busy to erase)");
        return false;
    }

    if (lib_state->getStackState() == 0)
    {
        LOG(LVL_INFO, "Waiting for watchdog...");
        // Watchdog is enabled, wait for it to fire
        // otherwise it will fire during second app execution
        // If second app also feed the WD, it is safe to skip the
        // next line and directly stop the stack
        while(1);
    }
    lib_state->stopStack();

    return true;
}

/**
 * \brief Button handler to switch back to the second app
 */
void onButtonPressed(uint8_t button_id,
                     button_event_e event)
{
    switch_back_to_second_app();
}

/**
 * \brief Led animation to demonstrate that we are on Wirepas side
 */
uint32_t led_animation_state_machine(void)
{
    static const uint32_t period[] = {100, 200, 300, 200};
    static uint8_t state = 0;
    uint8_t max_led = Led_getNumber();

    // Switch off current led
    Led_set(state % max_led, false);
    state ++;
    // Switch on next one
    Led_set(state % max_led, true);

    return period[(state / max_led) % 4];
}

/**
 * \brief   Initialization callback for application
 */
void App_init(const app_global_functions_t * functions)
{
    LOG_INIT();
    LOG(LVL_INFO, "App_init");
    Button_init();
    Led_init();

    // Register for Button 0 release event to switch to other app
    Button_register_for_event(0, BUTTON_RELEASED, onButtonPressed);

    // Start a led animation to show that we are on Wirepas side
    App_Scheduler_addTask_execTime(led_animation_state_machine, APP_SCHEDULER_SCHEDULE_ASAP, 20);

    // Basic configuration of the node with a unique node address
    if (configureNodeFromBuildParameters() != APP_RES_OK)
    {
        // Could not configure the node
        // It should not happen except if one of the config value is invalid
        return;
    }

    // Start the stack
    lib_state->startStack();
    LOG(LVL_INFO, "Normal Wirepas Execution");
}

