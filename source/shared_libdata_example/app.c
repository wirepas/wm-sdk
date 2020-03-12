/* Copyright 2019 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/*
 * \file    app.c
 * \brief   This file demonstrate the use of the shared_libdata library.
 */

#include <stdio.h>

#include "api.h"
#include "node_configuration.h"
#include "shared_libdata.h"

#define DEBUG_LOG_MODULE_NAME "DATA APP"
#define DEBUG_LOG_MAX_LEVEL LVL_INFO
#include "debug_log.h"

app_lib_data_receive_res_e shared_libdata_cb(const shared_libdata_item_t * item,
                                             const app_lib_data_received_t * data);

/* Allow only packet in Unicast on endpoints [10, 10] */
shared_libdata_item_t packet_1 =
{
    .cb = shared_libdata_cb,
    .filter = {
                .mode = SHARED_LIBDATA_NET_MODE_ALL,
                .src_endpoint = 10,
                .dest_endpoint = 10,
                .multicast_group = SHARED_LIBDATA_UNUSED_MULTISCAST
              }
};

/* Allow packet in Unicast or broadcast on endpoints [20, 20] */
shared_libdata_item_t packet_2 =
{
    .cb = shared_libdata_cb,
    .filter = {
                .mode = SHARED_LIBDATA_NET_MODE_ALL,
                .src_endpoint = 20,
                .dest_endpoint = 20,
                .multicast_group = SHARED_LIBDATA_UNUSED_MULTISCAST
              }
};

void sent_cb(const app_lib_data_sent_status_t * status)
{
    LOG(LVL_INFO, "Packet sent - src ep: %u, dest ep: %u, succes %u",
                  status->src_endpoint,
                  status->dest_endpoint,
                  (uint8_t)status->success);
}

app_lib_data_receive_res_e shared_libdata_cb(const shared_libdata_item_t * item,
                                             const app_lib_data_received_t * data)
{
    LOG(LVL_INFO, "Received packet - src ep: %u, dest ep: %u, mode %u",
                  item->filter.src_endpoint,
                  item->filter.dest_endpoint,
                  (uint8_t)item->filter.mode);


    /* This code here demonstrate that item can be modified directly from
     * packet callback.
     */
    if(item == &packet_2)
    {
        packet_2.cb = shared_libdata_cb;
        packet_2.filter.mode = SHARED_LIBDATA_NET_MODE_ALL;
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
        Shared_LibData_addDataReceivedCb(&packet_2);
    }

    /* Send back the received packet. */
    app_lib_data_to_send_t data_to_send;
    data_to_send.bytes = data->bytes;
    data_to_send.num_bytes = data->num_bytes;
    data_to_send.dest_address = data->src_address;
    data_to_send.delay = 0;
    /* Add a callback to be notified when the packet is sent. */
    data_to_send.qos = APP_LIB_DATA_QOS_HIGH;
    data_to_send.flags = APP_LIB_DATA_SEND_FLAG_NONE;
    data_to_send.src_endpoint = data->src_endpoint;
    data_to_send.dest_endpoint = data->dest_endpoint;

    Shared_LibData_sendData(&data_to_send, sent_cb);

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
    // Open Wirepas public API
    API_Open(functions);

    // Initialize uart for application debug prints
    LOG_INIT();
    LOG(LVL_INFO, "Shared LibData example started");

    // Basic configuration of the node with a unique node address
    if (configureNode(getUniqueAddress(),
                      NETWORK_ADDRESS,
                      NETWORK_CHANNEL) != APP_RES_OK)
    {
        // Could not configure the node
        // It should not happen except if one of the config value is invalid
        return;
    }

    Shared_LibData_init();

    Shared_LibData_addDataReceivedCb(&packet_1);
    Shared_LibData_addDataReceivedCb(&packet_2);

    // Start the stack
    lib_state->startStack();
}
