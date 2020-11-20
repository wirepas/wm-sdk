/* Copyright 2017 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/*
 * \file    app.c
 * \brief   This file is a minimal app to start the stack
 */

#include <stdlib.h>
#include <stdio.h>

#include "api.h"
#include "node_configuration.h"
#include "bme280_wrapper.h"


// How often a measurement must be done
#define MEASUREMENT_PERIOD_S    10
#define MEASUREMENT_PERIOD_US   (MEASUREMENT_PERIOD_S * 1000 * 1000)

static void send_measurement(bme280_wrapper_measurement_t * measurement)
{
    size_t count = 0;
    app_res_e res = lib_state->getRouteCount(&count);
    if (res == APP_RES_OK && count > 0)
    {
        // Send data
        app_lib_data_to_send_t data_to_send;
        data_to_send.bytes = (const uint8_t *) measurement;
        data_to_send.num_bytes = sizeof(bme280_wrapper_measurement_t);
        data_to_send.dest_address = APP_ADDR_ANYSINK;
        data_to_send.src_endpoint = 5;
        data_to_send.dest_endpoint = 5;
        data_to_send.qos = APP_LIB_DATA_QOS_HIGH;
        data_to_send.delay = 0;
        data_to_send.flags = APP_LIB_DATA_SEND_FLAG_NONE;
        data_to_send.tracking_id = APP_LIB_DATA_NO_TRACKING_ID;

        // Send the data packet
        lib_data->sendData(&data_to_send);
    }
}

/**
 * \brief   Periodic work of this app that implement the logic
 *          of reading and sending measurement
 */
static uint32_t periodic_work()
{
    static bool measurement_started = false;

    // Very simple state machine. If no measurement ongoing, start one
    // otherwise read the result of previous started measurement and send it
    if (!measurement_started)
    {
        measurement_started = true;
        return BME280_wrapper_start_measurement() * 1000;
    }
    else
    {
        bme280_wrapper_measurement_t measurement;
        BME280_wrapper_read_measurement(&measurement);
        measurement_started = false;

        send_measurement(&measurement);

        return MEASUREMENT_PERIOD_US;
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


    lib_system->setPeriodicCb(periodic_work, 0, 500);

    BME280_wrapper_init();

    /*
     * Start the stack.
     * This is really important step, otherwise the stack will stay stopped and
     * will not be part of any network. So the device will not be reachable
     * without reflashing it
     */
    lib_state->startStack();
}

