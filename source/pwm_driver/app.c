/* Copyright 2017 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/*
 * \file    app.c
 * \brief   This application is a demo for a pwm driver.
 *          Its purpose is to control a PWM driver from Wirepas Connectivity
 *          The output channel is board dependent and can be changed in
 *          board/$(BOARD)/board.h file
 */
#include "mcu.h"

#include "api.h"
#include "node_configuration.h"

#include <stdlib.h>

#include "pwm.h"

#define EXECUTION_TIME_US 500

// EP to change the PWM value
#define EP_CHANGE_PWM_VAL  10
// EP to get the current PWM value
#define EP_GET_PWM_VAL     11

// Current pwm value
static uint8_t m_current_pwm_value = 0;

/*
 * \brief Work to send message to sink
 */
static uint32_t send_message(void)
{
    app_lib_data_to_send_t data_to_send;

    data_to_send.bytes = (const uint8_t *) &m_current_pwm_value;
    data_to_send.num_bytes = sizeof(m_current_pwm_value);
    data_to_send.dest_address = APP_ADDR_ANYSINK;
    data_to_send.src_endpoint = EP_GET_PWM_VAL;
    data_to_send.dest_endpoint = EP_GET_PWM_VAL;
    data_to_send.qos = APP_LIB_DATA_QOS_HIGH;
    data_to_send.delay = 0;
    data_to_send.flags = APP_LIB_DATA_SEND_FLAG_NONE;
    data_to_send.tracking_id = APP_LIB_DATA_NO_TRACKING_ID;

    lib_data->sendData(&data_to_send);

    return APP_LIB_SYSTEM_STOP_PERIODIC;
}

static app_lib_data_receive_res_e dataReceivedCb(const app_lib_data_received_t * data)
{
    if (data->dest_endpoint == EP_CHANGE_PWM_VAL
        && data->num_bytes == 1)
    {
        uint32_t new_val = *(data->bytes);
        if (new_val > 100)
            new_val = 100;

        // store the current value
        m_current_pwm_value = new_val;

        Pwm_set_value(new_val);
        return APP_LIB_DATA_RECEIVE_RES_HANDLED;
    }
    else if (data->dest_endpoint == EP_GET_PWM_VAL)
    {
        lib_system->setPeriodicCb((app_lib_system_periodic_cb_f) send_message,
                                     0,
                                     EXECUTION_TIME_US);
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
    if (configureNode(getUniqueAddress(),
                      NETWORK_ADDRESS,
                      NETWORK_CHANNEL) != APP_RES_OK)
    {
        // Could not configure the node
        // It should not happen except if one of the config value is invalid
        return;
    }

    // Register for unicast and broadcast messages
    lib_data->setDataReceivedCb(dataReceivedCb);
    lib_data->setBcastDataReceivedCb(dataReceivedCb);

    // Start pwm driver
    Pwm_init();

    // Initialize first value
    Pwm_set_value(m_current_pwm_value);

    // Start the stack
    lib_state->startStack();
}

