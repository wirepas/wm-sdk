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

#include "mcu.h"

#define EXECUTION_TIME_US 500

/** EndPoint definition */
#define GET_TEMPERATURE_EP  12
#define SEND_TEMPERATURE_EP 13

/** Current temperature */
static uint32_t temperature = 0;

static void set_string_from_value(uint32_t value,
                                  uint8_t * bytes,
                                  uint8_t max_bytes)
{
    for (int i = max_bytes - 1; i >= 0; i--)
    {
        bytes[i] = value % 10 + 0x30;
        value /= 10;
    }
}

/** Decimal conversion */
static const uint8_t decimal[4] = { 0, 25, 50, 75 };

uint32_t send_temperature(void)
{
    app_lib_data_to_send_t data_to_send;
    uint8_t answer[13] = "TEMP is 00,00";

    // Temperature is given with a 0,25 C degree granularity
    set_string_from_value(temperature >> 2, &answer[8], 2);
    set_string_from_value(decimal[temperature % 4], &answer[11], 2);

    data_to_send.bytes = (const uint8_t *) &answer;
    data_to_send.num_bytes = 13;
    data_to_send.dest_address = APP_ADDR_ANYSINK;
    data_to_send.src_endpoint = 0;
    data_to_send.dest_endpoint = SEND_TEMPERATURE_EP;
    data_to_send.qos = APP_LIB_DATA_QOS_HIGH;
    data_to_send.delay = 0;
    data_to_send.flags = APP_LIB_DATA_SEND_FLAG_NONE;
    data_to_send.tracking_id = APP_LIB_DATA_NO_TRACKING_ID;

    lib_data->sendData(&data_to_send);

    return APP_LIB_SYSTEM_STOP_PERIODIC;
}

static void temp_interrupt_handler(void)
{
    NRF_TEMP->INTENCLR = 1;
    if (NRF_TEMP->EVENTS_DATARDY != 0)
    {
        temperature = NRF_TEMP->TEMP;
    }
    else
    {
        temperature = 0;
    }

    lib_system->setPeriodicCb((app_lib_system_periodic_cb_f) send_temperature,
                              0,
                              EXECUTION_TIME_US);
}

static void start_temperature_measurement()
{
    temperature = 0;
    NRF_TEMP->TASKS_START = 1;
    NRF_TEMP->INTENSET = 1;
}

static app_lib_data_receive_res_e unicastDataReceivedCb(const app_lib_data_received_t * data)
{
    if (data->dest_endpoint == GET_TEMPERATURE_EP)
    {
        start_temperature_measurement();
        return APP_LIB_DATA_RECEIVE_RES_HANDLED;
    }

    return APP_LIB_DATA_RECEIVE_RES_NOT_FOR_APP;
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

    // Enable interrupt
    lib_system->enableAppIrq(false, TEMP_IRQn, APP_LIB_SYSTEM_IRQ_PRIO_LO, temp_interrupt_handler);

    // Register for unicast messages
    lib_data->setDataReceivedCb(unicastDataReceivedCb);

    // Start the stack
    lib_state->startStack();
}

