/* Copyright 2017 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/*
 * \file    app.c
 * \brief   This application gives an example for a basic interrupt
 *          service. When it receives data on EP 12, it starts the
 *          internal temperature measure of the chip and send the
 *          result to a sink on EP 13
 */

#include "api.h"
#include "node_configuration.h"

#include <stdlib.h>
#include <string.h>
#include "shared_data.h"
#include "app_scheduler.h"

#include "mcu.h"

#define EXECUTION_TIME_US 500

/** EndPoint definition */
#define GET_TEMPERATURE_EP  12
#define SEND_TEMPERATURE_EP 13

/** Last read temperature */
static uint32_t m_temperature = 0;

uint32_t send_temperature(void)
{
    app_lib_data_to_send_t data_to_send = {
        .bytes = (const uint8_t *) &m_temperature,
        .num_bytes = sizeof(m_temperature),
        .dest_address = APP_ADDR_ANYSINK,
        .src_endpoint = SEND_TEMPERATURE_EP,
        .dest_endpoint = SEND_TEMPERATURE_EP,
        .qos = APP_LIB_DATA_QOS_HIGH,
    };

    Shared_Data_sendData(&data_to_send, NULL);

    return APP_LIB_SYSTEM_STOP_PERIODIC;
}


static void temp_interrupt_handler(void)
{
    NRF_TEMP->INTENCLR = 1;
    if (NRF_TEMP->EVENTS_DATARDY != 0)
    {
        m_temperature = NRF_TEMP->TEMP;
    }
    else
    {
        m_temperature = 0;
    }

    // Add a task to send the temperature outside of interrup context
    App_Scheduler_addTask_execTime(send_temperature,
                                    APP_SCHEDULER_SCHEDULE_ASAP,
                                    EXECUTION_TIME_US);
}

static app_lib_data_receive_res_e unicastDataReceivedCb(
                        const shared_data_item_t * item,
                        const app_lib_data_received_t * data )
{
    m_temperature = 0;
    NRF_TEMP->TASKS_START = 1;
    NRF_TEMP->INTENSET = 1;
    return APP_LIB_DATA_RECEIVE_RES_HANDLED;
}

/** Unicast messages filter */
static shared_data_item_t unicast_packets_filter =
{
    .cb = unicastDataReceivedCb,
    .filter = {
        .mode = SHARED_DATA_NET_MODE_UNICAST,
        /* Filtering by destination endpoint. */
        .src_endpoint = -1,
        .dest_endpoint = GET_TEMPERATURE_EP,
        .multicast_cb = NULL
    }
};

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

    // Enable interrupt
    lib_system->enableAppIrq(true, TEMP_IRQn, APP_LIB_SYSTEM_IRQ_PRIO_LO, temp_interrupt_handler);

    // Register for unicast messages
    Shared_Data_addDataReceivedCb(&unicast_packets_filter);

    // Start the stack
    lib_state->startStack();
}

