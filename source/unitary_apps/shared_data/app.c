/* Copyright 2019 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/**
 * @file    app.c
 * @brief   This file demonstrate the use of the shared_data library.
 *          The following packet filters where added :
 *          - Unicast packet on endpoints [10:10]
 *          - All types of packet (Uni/Multi/Broadcast on endpoints [20:20]
 *          - Multicast packets on group#2 (0x80000002) on any endpoints.
 *
 *          Each packets sent on endpoints [20:20] will modify filter to
 *          [30:30] and vice-versa.
 */

#include <stdio.h>

#include "api.h"
#include "node_configuration.h"
#include "shared_data.h"
#include "app_scheduler.h"

#define DEBUG_LOG_MODULE_NAME "DATA APP"
#define DEBUG_LOG_MAX_LEVEL LVL_INFO
#include "debug_log.h"

static app_lib_data_receive_res_e shared_data_cb(
                                            const shared_data_item_t * item,
                                            const app_lib_data_received_t * data);

static app_lib_data_receive_res_e shared_data_cb_filter4(
                                            const shared_data_item_t * item,
                                            const app_lib_data_received_t * data);

static bool filter_multicast_cb(app_addr_t group_addr);

/** Allow only packet in Unicast on endpoints [10, 10] */
static shared_data_item_t packet_1 =
{
    .cb = shared_data_cb,
    .filter = {
                .mode = SHARED_DATA_NET_MODE_UNICAST,
                .src_endpoint = 10,
                .dest_endpoint = 10,
                .multicast_cb = NULL
              }
};

/** Allow any type of packet on endpoints [20, 20] */
static shared_data_item_t packet_2 =
{
    .cb = shared_data_cb,
    .filter = {
                .mode = SHARED_DATA_NET_MODE_ALL,
                .src_endpoint = 20,
                .dest_endpoint = 20,
                .multicast_cb = NULL
              }
};

/** Allow packet in Multicast on any endpoints */
static shared_data_item_t packet_3 =
{
    .cb = shared_data_cb,
    .filter = {
                .mode = SHARED_DATA_NET_MODE_MULTICAST,
                .src_endpoint = SHARED_DATA_UNUSED_ENDPOINT,
                .dest_endpoint = SHARED_DATA_UNUSED_ENDPOINT,
                .multicast_cb = filter_multicast_cb
              }
};

/** Allow unicast packet on endpoint [11, 11] to demonstrate full buffer */
static shared_data_item_t packet_4 =
{
    .cb = shared_data_cb_filter4,
    .filter = {
                .mode = SHARED_DATA_NET_MODE_UNICAST,
                .src_endpoint = 11,
                .dest_endpoint = 11,
                .multicast_cb = NULL
              }
};

static void sent_cb(const app_lib_data_sent_status_t * status)
{
    LOG(LVL_INFO, "Packet sent (src ep: %u, dest ep: %u, tracking_id: %d, "
                  "success: %d).",
                  status->src_endpoint,
                  status->dest_endpoint,
                  status->tracking_id,
                  (uint8_t)status->success);
}

static uint32_t enable_reception()
{
    LOG(LVL_INFO, "Enable reception again");
    Shared_Data_readyToReceive(&packet_4);
    return APP_SCHEDULER_STOP_TASK;
}

static app_lib_data_receive_res_e shared_data_cb_filter4(
                                            const shared_data_item_t * item,
                                            const app_lib_data_received_t * data)
{
    /* Messages will be offered two times the message
     *  - First refuse it no space
     *  - Second, handle it
     */
    static bool blocked = false;
    if (blocked)
    {
        blocked = false;
        return APP_LIB_DATA_RECEIVE_RES_HANDLED;
    }
    else
    {
        blocked = true;
        LOG(LVL_INFO, "Block reception");
        /*
        * Any received packet on this EP will block reception for 30s
        */
        App_Scheduler_addTask_execTime(enable_reception, 30 * 1000, 100);
        return APP_LIB_DATA_RECEIVE_RES_NO_SPACE;
    }
}

static app_lib_data_receive_res_e shared_data_cb(
                                            const shared_data_item_t * item,
                                            const app_lib_data_received_t * data)
{
    LOG(LVL_INFO, "packet received with filter (src ep: %u, dest ep: %u, mode: %u).",
                  item->filter.src_endpoint,
                  item->filter.dest_endpoint,
                  (uint8_t)item->filter.mode);


    /* This code here demonstrate that item can be modified directly from
     * packet callback.
     */
    if(item == &packet_2)
    {
        packet_2.cb = shared_data_cb;
        packet_2.filter.mode = SHARED_DATA_NET_MODE_ALL;
        if(data->dest_endpoint == 20)
        {
            packet_2.filter.dest_endpoint = 30;
            packet_2.filter.src_endpoint = 30;
        }
        else
        {
            packet_2.filter.dest_endpoint = 20;
            packet_2.filter.src_endpoint = 20;
        }
        Shared_Data_addDataReceivedCb(&packet_2);
    }

    /* Send back the received packet. */
    app_lib_data_to_send_t data_to_send;
    data_to_send.bytes = data->bytes;
    data_to_send.num_bytes = data->num_bytes;
    data_to_send.dest_address = data->src_address;
    /* Add a callback to be notified when the packet is sent. */
    data_to_send.qos = APP_LIB_DATA_QOS_HIGH;
    data_to_send.flags = APP_LIB_DATA_SEND_FLAG_NONE;
    data_to_send.src_endpoint = data->src_endpoint;
    data_to_send.dest_endpoint = data->dest_endpoint;

    Shared_Data_sendData(&data_to_send, sent_cb);
    LOG(LVL_INFO, "Sent packet (tracking_id: %d).", data_to_send.tracking_id);

    return APP_LIB_DATA_RECEIVE_RES_HANDLED;
}

static bool filter_multicast_cb(app_addr_t group_addr)
{
    LOG(LVL_INFO, "Multicast filter (group_addr: %u).", group_addr);

    /* Accept packet on Multicast group #2. */
    if (group_addr == 0x80000002)
    {
        LOG(LVL_INFO, "Accept.");
        return true;
    }
    else
    {
        LOG(LVL_INFO, "Reject.");
        return false;
    }
}

/**
 * @brief   Initialization callback for application
 *
 * This function is called after hardware has been initialized but the
 * stack is not yet running.
 *
 */
void App_init(const app_global_functions_t * functions)
{
    // Initialize uart for application debug prints
    LOG_INIT();
    LOG(LVL_INFO, "Shared Data example started.");

    // Basic configuration of the node with a unique node address
    if (configureNodeFromBuildParameters() != APP_RES_OK)
    {
        // Could not configure the node
        // It should not happen except if one of the config value is invalid
        return;
    }

    Shared_Data_addDataReceivedCb(&packet_1);
    Shared_Data_addDataReceivedCb(&packet_2);
    Shared_Data_addDataReceivedCb(&packet_3);
    Shared_Data_addDataReceivedCb(&packet_4);

    // Start the stack
    lib_state->startStack();
}
