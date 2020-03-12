/* Copyright 2017 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/*
 * \file    app.c
 * \brief   This file is a template for writing a custom application
 */

#include <stdlib.h>

#include "api.h"
#include "node_configuration.h"

/** Period to send data */
#define DEFAULT_PERIOD_S    10
#define DEFAULT_PERIOD_US   (DEFAULT_PERIOD_S*1000*1000)

/** Time needed to execute the periodic work, in us */
#define EXECUTION_TIME_US 500

/** Endpoint to change the sending period value */
#define SET_PERIOD_EP  10

/** Endpoint to send data */
#define DATA_EP        1

/** Period to send measurements, in us */
static uint32_t period_us;

/**
 * \brief   Read the value in decimal from a string
 * \param   bytes
 *          Pointer to the string
 * \param   num_bytes
 *          Number of bytes in the string
 * \return  The parsed value
 */
static uint32_t get_value_from_string(const uint8_t * bytes,
                                      size_t num_bytes)
{
    uint32_t value = 0;
    while ((num_bytes--) > 0)
    {
        char c = (char)(*bytes++);
        if ((c < '0') || (c > '9'))
        {
            // Not a digit
            break;
        }
        value *= 10;
        value += c - '0';
    }

    return value;
}

/**
 * \brief   Periodic callback
 */
static uint32_t send_data(void)
{
    // This function will be called every period_us microseconds by the stack.
    // You can do anything you want for EXECUTION_TIME_US. In this example, a
    // monotonically increasing 32-bit value is sent to the sink.

    static uint32_t id = 0; // Value to send

    // Create a data packet to send
    app_lib_data_to_send_t data_to_send;
    data_to_send.bytes = (const uint8_t *) &id;
    data_to_send.num_bytes = sizeof(id);
    data_to_send.dest_address = APP_ADDR_ANYSINK;
    data_to_send.src_endpoint = DATA_EP;
    data_to_send.dest_endpoint = DATA_EP;
    data_to_send.qos = APP_LIB_DATA_QOS_HIGH;
    data_to_send.delay = 0;
    data_to_send.flags = APP_LIB_DATA_SEND_FLAG_NONE;
    data_to_send.tracking_id = APP_LIB_DATA_NO_TRACKING_ID;

    // Send the data packet
    lib_data->sendData(&data_to_send);

    // Increment value to send
    id++;

    // Inform the stack that this function should be called again in
    // period_us microseconds. By returning APP_LIB_SYSTEM_STOP_PERIODIC,
    // the stack won't call this function again.
    return period_us;
}

/**
 * \brief   Data reception callback
 * \param   data
 *          Received data, \ref app_lib_data_received_t
 * \return  Result code, \ref app_lib_data_receive_res_e
 */
static app_lib_data_receive_res_e dataReceivedCb(
    const app_lib_data_received_t * data)
{
    if ((data->num_bytes < 1) ||
        (data->dest_endpoint != SET_PERIOD_EP))
    {
        // Data was not for this application
        return APP_LIB_DATA_RECEIVE_RES_NOT_FOR_APP;
    }

    // Parse decimal digits to a period value
    uint32_t new_period = get_value_from_string(data->bytes, data->num_bytes);
    period_us = (new_period > 0) ? new_period * 1000 * 1000 :
                                   APP_LIB_SYSTEM_STOP_PERIODIC;

    // Update the period now. The current period will be longer
    // (already elapsed time from last period + new full period).
    lib_system->setPeriodicCb(send_data,
                              period_us,
                              EXECUTION_TIME_US);

    // Data handled successfully
    return APP_LIB_DATA_RECEIVE_RES_HANDLED;
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

    // Set a periodic callback to be called after DEFAULT_PERIOD_US
    period_us = DEFAULT_PERIOD_US;
    lib_system->setPeriodicCb(send_data,
                              period_us,
                              EXECUTION_TIME_US);

    // Set callback for received unicast messages
    lib_data->setDataReceivedCb(dataReceivedCb);

    // Set callback for received broadcast messages
    lib_data->setBcastDataReceivedCb(dataReceivedCb);

    // Start the stack
    lib_state->startStack();
}

