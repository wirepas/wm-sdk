/* Copyright 2017 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/*
 * \file    app.c
 * \brief   Bluetooth BLE beacon test application
 *
 * \note    Send "url www.whatever.com" to a device, to start
 *          transmitting Eddystone URL beacons every 0.333 seconds at
 *          full power and on every Bluetooth LE advertising channel
 */

#include "api.h"
#include "node_configuration.h"

#include <stdlib.h> // For NULL
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>

#include "shared_data.h"

#define DATA_EP 5

static const uint8_t beacon_hdr[] =
{
    0x42,                                   /* Non-connectable beacon */
    0x10, 0x32, 0x54, 0x76, 0x00, 0xc0,     /* Random addr (top two bits set) */
    0x02, 0x01, 0x04,                       /* Bluetooth LE beacon only */
    0x03, 0x03, 0xaa, 0xfe,                 /* Eddystone URL beacon */
    0x06, 0x16, 0xaa, 0xfe, 0x10, 0xf7,     /* URL length, URL */
    0x00                                    /* http:// */
};

static size_t makeBeacon(uint8_t * buffer,
                         const uint8_t * url,
                         size_t url_num_bytes)
{
    // Check URL length
    if ((sizeof(beacon_hdr) + url_num_bytes) >= APP_LIB_BEACON_TX_MAX_NUM_BYTES)
    {
        // URL too long
        return 0;
    }

    // Copy header to beacon
    memcpy(buffer, beacon_hdr, sizeof(beacon_hdr));

    app_addr_t addr;
    app_res_e res = lib_settings->getNodeAddress(&addr);
    if (res != APP_RES_OK)
    {
        // Could not get node address
        return 0;
    }

    // Modify random Bluetooth LE beacon address with node address
    buffer[1] = (uint8_t)((addr >>  0) & 0xff);
    buffer[2] = (uint8_t)((addr >>  8) & 0xff);
    buffer[3] = (uint8_t)((addr >> 16) & 0xff);
    buffer[4] = (uint8_t)((addr >> 24) & 0xff);

    // Fix URL length
    buffer[14] += url_num_bytes;

    // Copy URL to beacon
    // TODO: Support other than http:// URL scheme
    // TODO: Support URL replacements (.com/, etc.)
    memcpy(buffer + sizeof(beacon_hdr), url, url_num_bytes);

    return sizeof(beacon_hdr) + (size_t)url_num_bytes;
}

/* Generic callback function */
static app_lib_data_receive_res_e dataReceivedFunc(
    const shared_data_item_t * item,
    const app_lib_data_received_t * data)
{
    app_lib_data_to_send_t data_to_send;

    // Construct an Eddystone URL beacon
    uint8_t beacon[APP_LIB_BEACON_TX_MAX_NUM_BYTES];
    size_t beacon_num_bytes;

    // Any endpoint will do, but the message must start with "url "
    if ((data->num_bytes <= 4) ||
        (strncmp((const char *)(data->bytes), (const char *)"url ", 4) != 0))
    {
        // Data was not for this application
        return APP_LIB_DATA_RECEIVE_RES_NOT_FOR_APP;
    }
    else if ((beacon_num_bytes = makeBeacon(beacon,
                                            &((data->bytes)[4]),
                                            (data->num_bytes - 4))) == 0)
    {
        // Data was invalid in some way
        data_to_send.bytes = (const uint8_t *)"ERR";
        data_to_send.num_bytes = 3;
        data_to_send.dest_address = APP_ADDR_ANYSINK;
        data_to_send.src_endpoint = 0;
        data_to_send.dest_endpoint = 0;
        data_to_send.qos = APP_LIB_DATA_QOS_NORMAL;
        data_to_send.delay = 0;
        data_to_send.flags = APP_LIB_DATA_SEND_FLAG_NONE;
        data_to_send.tracking_id = APP_LIB_DATA_NO_TRACKING_ID;

        lib_data->sendData(&data_to_send);

        return APP_LIB_DATA_RECEIVE_RES_HANDLED;
    }

    // Set default parameters for beacons: one beacon at full
    // power on all advertising channels, three times a second
    lib_beacon_tx->clearBeacons();
    lib_beacon_tx->setBeaconInterval(333);          // 333 ms
    int8_t power = 8;                               // 8 dBm
    lib_beacon_tx->setBeaconPower(0, &power);
    lib_beacon_tx->setBeaconChannels(
        0, APP_LIB_BEACON_TX_CHANNELS_ALL);         // All channels
    lib_beacon_tx->setBeaconContents(0, beacon, beacon_num_bytes);
    lib_beacon_tx->enableBeacons(true);

    // Send response
    data_to_send.bytes = (const uint8_t *)"OK";
    data_to_send.num_bytes = 2;
    data_to_send.dest_address = APP_ADDR_ANYSINK;
    data_to_send.src_endpoint = 0;
    data_to_send.dest_endpoint = 0;
    data_to_send.qos = APP_LIB_DATA_QOS_NORMAL;
    data_to_send.delay = 0;
    data_to_send.flags = APP_LIB_DATA_SEND_FLAG_NONE;
    data_to_send.tracking_id = APP_LIB_DATA_NO_TRACKING_ID;

    Shared_Data_sendData(&data_to_send, NULL);

    return APP_LIB_DATA_RECEIVE_RES_HANDLED;
}



/** All messages filter */
static shared_data_item_t all_packets_filter =
{
    .cb = dataReceivedFunc,
    .filter = {
        .mode = SHARED_DATA_NET_MODE_ALL,
        /* Filtering by source endpoint. */
        .src_endpoint = DATA_EP,
        /* Filtering by destination endpoint. */
        .dest_endpoint = DATA_EP,
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

    Shared_Data_addDataReceivedCb(&all_packets_filter);
    lib_state->startStack();
}
