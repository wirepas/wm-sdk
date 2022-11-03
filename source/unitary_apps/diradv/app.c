/* Copyright 2020 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/*
 * \file    app.c
 * \brief   This file is a directed advertiser example application.
 *
 * When application is programmed on device, it has two operating modes which
 * are selected by 'button 0' when booting up the device:
 * - Button 0 pressed: directed advertiser
 * - Button 0 not pressed: CSMA-CA headnode
 *
 * Directed advertiser sends once per minute unicast packet to CSMA-CA headnodes
 * that have route to a sink. If it does not find such CSMA-CA headnodes, it
 * will try again after one minute.
 *
 * CSMA-CA headnode receives the packet sent by directed advertiser and creates
 * another data packet that will be sent to backend (@ref APP_ADDR_ANYSINK).
 */

#include <stdlib.h>
#include <stdio.h>

#include "api.h"
#include "node_configuration.h"
#include "led.h"
#include "button.h"
#include "shared_data.h"
#include "app_scheduler.h"
#include "stack_state.h"

// How often diradv scans and sends data (in useconds)
#define SCAN_PERIOD_MS (60*1000)

// Transmission from advertiser
const uint8_t m_adv_tx_data[] = "Hello from advertiser!";

// Source endpoint to send advertiser data
#define DIRADV_EP_SRC_DATA 248

/**
 * \brief   Transmission definition for advertiser
 */
static app_lib_data_to_send_t m_tx_def_adv =
{
 .bytes = &m_adv_tx_data[0],
 .num_bytes = sizeof(m_adv_tx_data),
 .delay = 0,
 .tracking_id = APP_LIB_DATA_NO_TRACKING_ID,
 .qos = APP_LIB_DATA_QOS_NORMAL,
 .flags = APP_LIB_DATA_SEND_FLAG_TRACK,
 .src_endpoint = DIRADV_EP_SRC_DATA,
 .dest_endpoint = DIRADV_EP_DEST,
};

// Endpoints that headnode generates
#define HN_SRC_EP   5
#define HN_DST_EP   5

/**
 * \brief   Transmission from headnode
 */
static app_addr_t m_adv_address;

/**
 * \brief   Transmission definition for headnode
 */
static app_lib_data_to_send_t m_tx_def_hn =
{
 .bytes = (const uint8_t *) &m_adv_address,
 .num_bytes = sizeof(m_adv_address),
 .delay = 0,
 .tracking_id = APP_LIB_DATA_NO_TRACKING_ID,
 .qos = APP_LIB_DATA_QOS_NORMAL,
 .flags = APP_LIB_DATA_SEND_FLAG_NONE,
 .src_endpoint = HN_SRC_EP,
 .dest_endpoint = HN_DST_EP,
 .dest_address = APP_ADDR_ANYSINK,
};

/**
 * \brief   Acknowledgement content from headnode to advertiser
 */
const uint8_t m_ack_content[] = "ACK";

/**
 * \brief   Tracking id used when sending
 */
static uint8_t m_tracking_id;

/**
 * \brief   Is advertiser scanning or not?
 */
static bool m_is_scanning;

/**
 * \brief       Beacon listen callback. Advertiser has received network beacon
 * \param[in]   beacon
 *              Received network beacon information
 */
static void beacon_listen_cb(const app_lib_state_beacon_rx_t * beacon)
{
    // Skips time-slotted mode devices and devices without route
    if (beacon->is_ll &&
        (beacon->cost != 255) &&
        m_is_scanning)
    {
        // Simply send data to the first device found
        app_lib_data_send_res_e res;
        m_tx_def_adv.dest_address = beacon->address;
        m_tx_def_adv.tracking_id = m_tracking_id++;
        res = Shared_Data_sendData(&m_tx_def_adv, NULL);
        if (res == APP_LIB_DATA_SEND_RES_SUCCESS)
        {
            // Only send one device.
            m_is_scanning = false;
            lib_state->stopScanNbors();
        }
    }
}

/**
 * \brief  Start scanning on advertiser
 */
static uint32_t start_scan(void)
{
    // Start scanning for stack default time
    // Scan will be aborted if we find a neighbor
    m_is_scanning = true;
    lib_state->startScanNbors();
    // Enable led
    Led_set(0, true);
    // And try again later
    return SCAN_PERIOD_MS;
}

/**
 * \brief   Scan ended
 */
static void scan_ended(app_lib_stack_event_e event, void * param_p)
{
    app_lib_state_neighbor_scan_info_t * scan_info;

    scan_info = (app_lib_state_neighbor_scan_info_t *) param_p;

    if (scan_info->complete == true &&
        scan_info->scan_type == SCAN_TYPE_APP_ORIGINATED)
    {
        // Disable LED
        Led_set(0, false);
        m_is_scanning = false;
    }
}

/**
 * \brief   Packet sent
 */
static void data_sent_cb(const app_lib_data_sent_status_t * status)
{
    (void) status;
    // Disable LED
    Led_set(0, false);
}

/**
 * \brief   Callback on headnode. Generate response to backend
 * @param   in
 *          Information on received packet from advertiser
 * @param   out
 *          Acknowledgement to be sent to advertiser
 * @return  always true, i.e. send acknowledgement
 */
static bool acklistener_cb(const ack_gen_input_t * in,
                           ack_gen_output_t * out)
{
    m_adv_address = in->sender;

    // Send packet to sink
    Shared_Data_sendData(&m_tx_def_hn, NULL);

    out->data = (void *) &m_ack_content;
    out->length = sizeof(m_ack_content);
    return true;
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

    // Initialize buttons
    Button_init();
    // Query value of button 1
    bool pressed = false;
    Button_getState(0, &pressed);

    // Initialize leds
    Led_init();

    // Override node role
    if (pressed)
    {
        // Advertiser
        lib_settings->setNodeRole(APP_LIB_SETTINGS_ROLE_ADVERTISER);
        // Set scan callback to find CSMA-CA headnode (with route)
        lib_state->setOnBeaconCb(beacon_listen_cb);
        // Set periodic callback when do scan+transmission
        App_Scheduler_addTask_execTime(start_scan,
                                       SCAN_PERIOD_MS,
                                       1000);
        // Set scan end callback. Turns off the led if nothing was found
        Stack_State_addEventCb(scan_ended, 1 << APP_LIB_STATE_STACK_EVENT_SCAN_STOPPED);
        // Callback when data has been sent. Turns off the led.
        lib_data->setDataSentCb(data_sent_cb);
    }
    else
    {
        lib_settings->setNodeRole(APP_LIB_SETTINGS_ROLE_HEADNODE_LL);
        // Set callback to receive packet from advertiser and generate response
        // to sink
        lib_advertiser->setRouterAckGenCb(acklistener_cb);
    }

    /*
     * Start the stack.
     * This is really important step, otherwise the stack will stay stopped and
     * will not be part of any network. So the device will not be reachable
     * without reflashing it
     */
    lib_state->startStack();
}

