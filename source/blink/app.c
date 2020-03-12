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


/** Duration between timer callbacks */
#define CALLBACK_PERIOD_S   1
#define CALLBACK_PERIOD_US  (CALLBACK_PERIOD_S * 1000000UL)

/** Time spent in timer callback */
#define EXECUTION_TIME_US   100

/** LED number to blink */
#define LED_NUMBER          0

/** LED state for blinking */
static bool led_state = false;

/**
 * \brief   Function that is called periodically by the stack
 *
 * System library function registerPeriodicWork() is used to
 * set up the first call to this function in App_init().
 */
static uint32_t blink_func(void)
{
    // Toggle LED state
    if (led_state)
    {
        led_state = false;
    }
    else
    {
        led_state = true;
    }

    // Led_toggle could be used instead (it is just an example)
    Led_set(LED_NUMBER, led_state);

    return CALLBACK_PERIOD_US;
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
    if (configureNode(getUniqueAddress(),
                      NETWORK_ADDRESS,
                      NETWORK_CHANNEL) != APP_RES_OK)
    {
        // Could not configure the node
        // It should not happen except if one of the config value is invalid
        return;
    }

    // Set up LED GPIO
    Led_init();

    // Turn on LED initially
    blink_func();

    // Set the periodic callback to be called by the
    // stack after CALLBACK_PERIOD_US microseconds
    lib_system->setPeriodicCb(blink_func,
                              CALLBACK_PERIOD_US,
                              EXECUTION_TIME_US);

    // Start the stack
    lib_state->startStack();
}
