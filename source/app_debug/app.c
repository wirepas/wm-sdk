/* Copyright 2019 Wirepas Ltd. All Rights Reserved.
 *
 * * See file LICENSE.txt for full license details.
 *
 */

/*
 * \file    app.c
 * \brief   This is an example of using debug prints in an application.
 *
 *  This application provides an example of printig formatted data to the
 *  uart port. This is especially helpful for debugging applications.
 */

#include <stdlib.h>
#include <string.h>
#include "api.h"
#include "node_configuration.h"
#include "uart_print.h"

/** Define baudrate for uart */
#define BAUDRATE            115200

/** Period to send data */
#define DEFAULT_PERIOD_S    10
#define DEFAULT_PERIOD_US   (DEFAULT_PERIOD_S*1000*1000)

/** Time needed to execute the periodic work, in us */
#define EXECUTION_TIME_US   500

/** Period to send measurements, in us */
static uint32_t period_us;

/**
 * \brief   Callback function to demostrate debug prints
 */

static uint32_t send_data(void)
{
    static uint8_t cntr = 0;
    UartPrint_printf("TX: Periodic data = %d \n", cntr);
    cntr++;
    return period_us;
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
    // Initialize uart for application debug prints
    UartPrint_init(BAUDRATE);
    UartPrint_printf("App started\n");

    // Basic configuration of the node with a unique node address
    if (configureNode(getUniqueAddress(),
                      NETWORK_ADDRESS,
                      NETWORK_CHANNEL) != APP_RES_OK)
    {
        // Could not configure the node
        // It should not happen except if one of the config value is invalid
        return;
    }

    // Set a periodic callback to be called after DEFAULT_PERIOD_US
    period_us = DEFAULT_PERIOD_US;
    lib_system->setPeriodicCb(send_data,
                              period_us,
                              EXECUTION_TIME_US);

    // Start the stack
    lib_state->startStack();
}
