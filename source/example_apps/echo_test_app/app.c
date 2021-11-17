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
#include <string.h>
#include <stdio.h>

#include "api.h"
#include "node_configuration.h"
#include "hal_api.h"
#include "pack.h"

#include "leds.h"
#include "uart_controller.h"
#include "cmd.h"

/** Period to send data */
#define DEFAULT_PERIOD_S    5
#define DEFAULT_PERIOD_US   (DEFAULT_PERIOD_S*1000*1000)

/** Time needed to execute the periodic work, in us */
#define EXECUTION_TIME_US 500

/** Endpoint to change the sending period value */
#define SET_PERIOD_EP  10

/** Endpoint to send data */
#define DATA_EP        13

/** Endpoint to obtain data from the cloud and send echo packet */
#define CLOUD_EP       14

/** Period to send measurements, in us */
static uint32_t period_us;


/**
 * \brief   Periodic callback
 */
static uint32_t send_data(void)
{
    // This function will be called every period_us microseconds by the stack.
    // You can do anything you want for EXECUTION_TIME_US. In this example, a
    // monotonically increasing 32-bit value is sent to the sink.
    static uint32_t count = 0;
    uint8_t data[4];
    memset(data, 0, sizeof(data));

    Pack_packLe(data, count, 4);

    count++;

    // Create a data packet to send
    app_lib_data_to_send_t data_to_send;
    data_to_send.bytes = data;
    data_to_send.num_bytes = sizeof(data);
    data_to_send.dest_address = APP_ADDR_ANYSINK;
    data_to_send.src_endpoint = DATA_EP;
    data_to_send.dest_endpoint = DATA_EP;
    data_to_send.qos = APP_LIB_DATA_QOS_HIGH;
    data_to_send.delay = 0;
    data_to_send.flags = APP_LIB_DATA_SEND_FLAG_NONE;
    data_to_send.tracking_id = APP_LIB_DATA_NO_TRACKING_ID;

    // Send the data packet
    lib_data->sendData(&data_to_send);

    led_toggle(LED_1_PIN);
    if(get_wrp_print_msg_st())
        uart_fprintf("Send data - %08x\r\n", data);

    // Inform the stack that this function should be called again in
    // period_us microseconds. By returning APP_LIB_SYSTEM_STOP_PERIODIC,
    // the stack won't call this function again.
    return period_us;
}

static void send_echo(const app_lib_data_received_t * data)
{
    // Create a data packet to send
    app_lib_data_to_send_t data_to_send;
    data_to_send.bytes = data->bytes;
    data_to_send.num_bytes = data->num_bytes;
    data_to_send.dest_address = data->src_address; // TODO: consider APP_ADDR_ANYSINK as destination
    data_to_send.src_endpoint = data->dest_endpoint;
    data_to_send.dest_endpoint = data->dest_endpoint;
    data_to_send.qos = APP_LIB_DATA_QOS_HIGH;
    data_to_send.delay = 0;
    data_to_send.flags = APP_LIB_DATA_SEND_FLAG_NONE;
    data_to_send.tracking_id = APP_LIB_DATA_NO_TRACKING_ID;

    // Send the data packet
    lib_data->sendData(&data_to_send);
    led_toggle(LED_4_PIN);
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
    led_toggle(LED_2_PIN);
    if(data->dest_endpoint == CLOUD_EP)
    {
        led_toggle(LED_3_PIN);
         if(get_wrp_print_msg_st())
            uart_fprintf("Source addr - %X (%d)\r\n", data->src_address, data->src_address);
        send_echo(data);
        return APP_LIB_DATA_RECEIVE_RES_HANDLED;
    }
    if(get_wrp_print_msg_st()){
        uart_fprintf("Source addr (a general packet) - %X (%d)\r\n", data->src_address, data->src_address);
        uart_fprintf("Dst endpoint - %X (%d)\r\n", data->dest_endpoint, data->dest_endpoint);
    }
    return APP_LIB_DATA_RECEIVE_RES_NOT_FOR_APP;
}

uint32_t setSysPeriodicCb(void)
{
    // Set a periodic callback to be called after DEFAULT_PERIOD_US
    period_us = DEFAULT_PERIOD_US;
    lib_system->setPeriodicCb(send_data,
                              period_us,
                              EXECUTION_TIME_US);
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
    // Open Wirepas public API
    API_Open(functions);
    HAL_Open();

    // Basic configuration of the node with a unique node address
    if (configureNodeFromBuildParameters() != APP_RES_OK)
    {
        // Could not configure the node
        // It should not happen except if one of the config value is invalid
        return;
    }

    setup_uart(uart_rx_cb);

    configure_leds();

    setSysPeriodicCb();

    // Set callback for received unicast messages
    lib_data->setDataReceivedCb(dataReceivedCb);

    // Set callback for received broadcast messages
    lib_data->setBcastDataReceivedCb(dataReceivedCb);

    if(get_auto_start()){
        // Start the stack
        lib_state->startStack();
    }
}
