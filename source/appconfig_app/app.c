/* Copyright 2019 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/*
 * \file    app.c
 * \brief   This is an example of receiving application configuration from
 *          the network and change application behavior accordingly.
 *
 *          Configuration messages are sent by the sink(s) and spread to the
 *          whole network. Also every new node joining the network receives
 *          the configuration data from its neighbors without the need for
 *          end-to-end polling.
 */

#include <stdlib.h>
#include <string.h>
#include "api.h"
#include "node_configuration.h"

/** Period to send data */
#define DEFAULT_PERIOD_S    10
#define DEFAULT_PERIOD_US   (DEFAULT_PERIOD_S*1000*1000)

/** Time needed to execute the periodic work, in us */
#define EXECUTION_TIME_US   500

/** Endpoint to send data */
#define DATA_EP             1

/** Period to send measurements, in us */
static uint32_t period_us;

/**
 *  In this example the periodic data transfer interval is changed according
 *  to received configuration data.
 *  Comfiguration data structure is application dependend, in this example it
 *  is structured as follows:
 *  data[0]: Configuration ID
 *           1: Application specific configuration
 *           2: Reserved for configuring Ipv6 functionality
 *  data[1]: length of the configuration data, always 1 in this case
 *  data[2]: new periodic data transfer interval in secods
 *  e.g. Application data pattern "01 01 0f" changes the interval to 15 seconds
 *
 *  This structure is used to store application specific configuration
 *  received from the network.
 * */
typedef struct
{
   uint8_t  seq;         // sequency number of received app config 0 - 254
   uint16_t interval;    // Diagnostic data transmission interval [s]
   uint8_t  data[APP_LIB_DATA_MAX_APP_CONFIG_NUM_BYTES]; // Received configs
}appConf_t;

appConf_t appConf;

/**
 * \brief   Callback function to send incremental counter to any sink at
 *          periodic interval.
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
 * \brief   Application configuration reception callback
 * \param   bytes
 *          New app config data
 * \param   seq
 *          New app config data sequence number
 * \param   interval
 *          New app config data diagnostic interval, in seconds
 */
static void appConfigReceivedCb(const uint8_t * bytes,
                                 uint8_t seq,
                                 uint16_t interval)
{
    // Seq and inteval are stored but not used in this example
    appConf.seq = seq;
    appConf.interval = interval;
    // Store latest config, first 3 bytes are meanigful in this example
    memcpy(&appConf.data, bytes, 3);
    // Check data validity
    if (appConf.data[0] == 1 && appConf.data[1] == 1 && appConf.data[2] > 0)
    {
        // Set new periodic data transfer interval
        period_us = appConf.data[2] * 1000 * 1000;
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

    // Set a periodic callback to be called after DEFAULT_PERIOD_US
    period_us = DEFAULT_PERIOD_US;
    lib_system->setPeriodicCb(send_data,
                              period_us,
                              EXECUTION_TIME_US);

    // Set callback for received app config messages
    lib_data->setNewAppConfigCb(appConfigReceivedCb);

    // Start the stack
    lib_state->startStack();
}

