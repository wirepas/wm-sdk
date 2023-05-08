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
#include "shared_appconfig.h"
#include "api.h"
#include "node_configuration.h"
#include "shared_data.h"
#include "app_scheduler.h"

#define DEBUG_LOG_MODULE_NAME "MAIN_APP"
#define DEBUG_LOG_MAX_LEVEL LVL_INFO
#include "debug_log.h"

/** Period to send data */
#define DEFAULT_PERIOD_S    10
#define DEFAULT_PERIOD_MS   (DEFAULT_PERIOD_S*1000)

/** Time needed to execute the periodic work, in us */
#define EXECUTION_TIME_US   500

/** Endpoint to send data */
#define DATA_EP             1

/** Period to send measurements, in us */
static uint32_t m_period_ms;

/** In this example the app will register for this random type
 *  for sub app config content */
#define CUSTOM_TLV_TYPE 0x12

/** Filter id registered */
static uint16_t m_filter_id;

/** Filter id for raw app_config */
static uint16_t m_raw_filter_id;

/** Filter id for all app_config */
static uint16_t m_all_filter_id;

/**
 *  In this example the periodic data transfer interval is changed according
 *  to received configuration data.
 *  Configuration data structure is application dependent, in this example it
 *  is structured as followed:
 *      A sequence number and a period
 * */
typedef struct  __attribute__((packed))
{
   uint8_t  seq;         // sequence number (app specific)
   uint16_t interval;    // period in second for app to send data
} custom_app_config_t;

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
    data_to_send.flags = APP_LIB_DATA_SEND_FLAG_NONE;
    data_to_send.tracking_id = APP_LIB_DATA_NO_TRACKING_ID;

    // Send the data packet
    Shared_Data_sendData(&data_to_send, NULL);

    // Increment value to send
    id++;

    // Inform the stack that this function should be called again in
    // period_us microseconds. By returning APP_LIB_SYSTEM_STOP_PERIODIC,
    // the stack won't call this function again.
    return m_period_ms;
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
static void appConfigTLVReceivedCb(uint16_t type,
                                   uint8_t length,
                                   uint8_t * value_p)
{
    custom_app_config_t * config;

    if (type != CUSTOM_TLV_TYPE)
    {
        // It should never happen as we registered only this type with this cb
        LOG(LVL_ERROR, "Wrong app config type");
        return;
    }

    if (length == 0)
    {
        LOG(LVL_INFO, "App config received but not for us");
        return;
    }

    if (length != sizeof(custom_app_config_t))
    {
        // Wrong size
        LOG(LVL_ERROR, "Wrong app config size");
        return;
    }

    config = (custom_app_config_t *) value_p;

    LOG(LVL_INFO,
        "New app configuration seq=%d, interval_s=%d",
        config->seq,
        config->interval);

    // Set new periodic data transfer interval
    m_period_ms = config->interval * 1000;
}

static void appConfigRawReceivedCb(uint16_t type,
                                   uint8_t length,
                                   uint8_t * value_p)
{
    if (type != SHARED_APP_CONFIG_INCOMPATIBLE_FILTER)
    {
        // It should never happen as we register for raw type only with this cb
        LOG(LVL_ERROR, "Wrong app config raw type");
        return;
    }

    LOG(LVL_INFO, "New RAW app configuration len=%d\n", length);
}

static void appConfigTLVAllReceivedCb(uint16_t type,
                                      uint8_t length,
                                      uint8_t * value_p)
{

    LOG(LVL_INFO, "New app configuration all filters type=%d len=%d\n", type, length);
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
    shared_app_config_filter_t app_config_filter;

    LOG_INIT();

    // Basic configuration of the node with a unique node address
    if (configureNodeFromBuildParameters() != APP_RES_OK)
    {
        // Could not configure the node
        // It should not happen except if one of the config value is invalid
        return;
    }

    LOG(LVL_DEBUG, "App starting");

    // Prepare the app_config filter
    app_config_filter.type = CUSTOM_TLV_TYPE;
    app_config_filter.cb = appConfigTLVReceivedCb;
    app_config_filter.call_cb_always = true;

    Shared_Appconfig_addFilter(&app_config_filter, &m_filter_id);
    LOG(LVL_INFO, "Filter added for TLV with id=%d\n", m_filter_id);

    // Prepare a second app_config filter
    app_config_filter.type = SHARED_APP_CONFIG_INCOMPATIBLE_FILTER;
    app_config_filter.cb = appConfigRawReceivedCb;
    app_config_filter.call_cb_always = false;

    Shared_Appconfig_addFilter(&app_config_filter, &m_raw_filter_id);
    LOG(LVL_INFO, "Filter added for TLV with id=%d\n", m_raw_filter_id);

    // Prepare a third app_config filter
    app_config_filter.type = SHARED_APP_CONFIG_ALL_TYPE_FILTER;
    app_config_filter.cb = appConfigTLVAllReceivedCb;
    app_config_filter.call_cb_always = false;

    Shared_Appconfig_addFilter(&app_config_filter, &m_all_filter_id);
    LOG(LVL_INFO, "Filter added for TLV with id=%d\n", m_all_filter_id);

    // Set a periodic callback to be called after DEFAULT_PERIOD_MS
    m_period_ms = DEFAULT_PERIOD_MS;

    App_Scheduler_addTask_execTime(send_data,
                                  m_period_ms,
                                  EXECUTION_TIME_US);
    // Start the stack
    lib_state->startStack();
}

