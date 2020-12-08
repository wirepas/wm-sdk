/* Copyright 2019 Wirepas Ltd. All Rights Reserved.
 *
 * * See file LICENSE.txt for full license details.
 *
 */

/*
 * \file    app.c
 * \brief   This is an example of using debug logs in an application.
 *
 *  This application provides an example of printing formatted data to the
 *  uart port. This is especially helpful for debugging applications.
 */

#include <stdlib.h>
#include <string.h>
#include "api.h"
#include "node_configuration.h"

/* The module name to be printed each lines. */
#define DEBUG_LOG_MODULE_NAME "DBUG APP"
/* The maximum log level printed for this module. */
#define DEBUG_LOG_MAX_LEVEL LVL_INFO
/* debug_log.h can be included in multiple c files with an other name or log
 * level
 */
#include "debug_log.h"


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
    LOG(LVL_INFO, "TX: Periodic data = %d", cntr);
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
    /* Initialize debug logs over serial (baudrate is 115200). */
    LOG_INIT();
    LOG(LVL_INFO, "App started");

    /* This will only be printed if DEBUG_LOG_MAX_LEVEL is set to LVL_DEBUG. */
    LOG(LVL_DEBUG, "This is a debug message.");

    // Basic configuration of the node with a unique node address
    if (configureNodeFromBuildParameters() != APP_RES_OK)
    {
        /* Could not configure the node. */
        LOG(LVL_ERROR, "Error configuring node.");
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
