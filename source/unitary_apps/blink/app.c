/* Copyright 2017 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/*
 * \file    app.c
 * \brief   A simple LED blink application
 */

#include <stdlib.h> // For NULL

#include "api.h"
#include "node_configuration.h"
#include "led.h"
#include "button.h"
#include "app_scheduler.h"



/** Duration between timer callbacks */
#define CALLBACK_PERIOD_S   1
#define CALLBACK_PERIOD_MS  (CALLBACK_PERIOD_S * 1000)

/** Time spent in timer callback */
#define EXECUTION_TIME_US   100

/** Let the blinking of LEDs be held off for a short time */
static uint32_t blink_holdoff;

/**
 * \brief   Function that is called periodically by the stack
 *
 * This function toggles the LEDs once per second.
 *
 * System library function registerPeriodicWork() is used to
 * set up the first call to this function in App_init().
 */
static uint32_t blink_func(void)
{
    static uint32_t counter = 0;

    if (blink_holdoff > 0)
    {
        blink_holdoff--;
    }
    else
    {
        // Toggle all LEDs
        uint32_t bits = counter;
        uint_fast8_t num_leds = Led_getNumber();
        for (uint_fast8_t led_id = 0; led_id < num_leds; led_id++)
        {
            Led_set(led_id, bits & 1);
            bits >>= 1;
        }

        counter++;
    }

    return CALLBACK_PERIOD_MS;
}

/**
 * \brief   Callback for button press
 * \param   button_id
 *          Number of button that was pressed
 * \param   event
 *          Always BUTTON_PRESSED here
 *
 * This function is called when a button is pressed down.
 */
static void button_press_func(uint8_t button_id, button_event_e event)
{
    (void) event;

    // Stop LED blinking for five seconds
    blink_holdoff = 5;

    // Turn LED on
    Led_set(button_id, true);
}

/**
 * \brief   Callback for button release
 * \param   button_id
 *          Number of button that was released
 * \param   event
 *          Always BUTTON_RELEASED here
 *
 * This function is called when a button is released.
 */
static void button_release_func(uint8_t button_id, button_event_e event)
{
    (void) event;

    // Stop LED blinking for five seconds
    blink_holdoff = 5;

    // Turn LED off
    Led_set(button_id, false);
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

    // Set up LEDs
    Led_init();

    // Set up buttons
    Button_init();
    uint_fast8_t num_buttons = Button_get_number();
    for (uint_fast8_t button_id = 0; button_id < num_buttons; button_id++)
    {
        Button_register_for_event(button_id,
                                  BUTTON_PRESSED,
                                  button_press_func);
        Button_register_for_event(button_id,
                                  BUTTON_RELEASED,
                                  button_release_func);
    }

    // Set the blink callback to be called
    // immediately after the stack is started
    blink_holdoff = 0;

    App_Scheduler_addTask_execTime(blink_func,
                                   APP_SCHEDULER_SCHEDULE_ASAP,
                                   EXECUTION_TIME_US);

    // Start the stack
    lib_state->startStack();
}
