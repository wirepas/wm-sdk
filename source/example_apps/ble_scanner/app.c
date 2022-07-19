/* Copyright 2021 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/*
 * \file    app.c
 * \brief   This file is an example for beacon scanner feature
 *
 */

#include "api.h"
#include "node_configuration.h"
#include "ble_scanner.h"
#include "shared_data.h"
#include "stack_state.h"
#include "led.h"
#ifdef DUALMCU_INTERFACE
#include "dualmcu_lib.h"
#endif

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#define DEBUG_LOG_MODULE_NAME "BLE SCANNER"
#define DEBUG_LOG_MAX_LEVEL LVL_INFO
#include "debug_log.h"

#define BLE_SCANNER_CONFIG_REQ_EP   13
#define BLE_SCANNER_CONFIG_REPLY_EP   14

#define BLE_SCANNER_DATA_EP   18

/** Cmd accepted for this app */
typedef enum
{
    SCANNER_CMD_DISABLE = 0, //< Disable scanner
    SCANNER_CMD_ENABLE = 1, //< Enable scanner always
    SCANNER_CMD_ENABLE_PERIODIC = 2 //< Enable scanner periodicaly
} scanner_cmd_e;

typedef struct __attribute__ ((__packed__))
{
    uint8_t channel; // Same as Ble_scanner_channel_e
} enable_payload_t;

typedef struct __attribute__ ((__packed__))
{
    uint16_t period_s;
    uint16_t scan_length_s;
    uint8_t channel; // Same as Ble_scanner_channel_e
} enable_periodic_payload_t;

/** Application message format */
typedef struct __attribute__((packed))
{
    uint32_t id;
    uint8_t cmd_id;
    union
    {
        enable_payload_t                enable;
        enable_periodic_payload_t       enable_periodic;
    } payload;
} cmd_t;

/** Response format */
typedef struct __attribute__((packed))
{
    uint32_t id;
    uint8_t res;
} response_t;

static bool ble_scanner_filter(const app_lib_beacon_rx_received_t * packet)
{
    const uint8_t pattern_enocean[] = {0x4C, 0x2E, 0x00, 0x00, 0x15};

    // Filter enocean switches as an example
    return  packet->length > 5 &&
        !memcmp(pattern_enocean, packet->payload, sizeof(pattern_enocean));
}

static app_lib_data_receive_res_e ble_scanner_configuration_cb(
                                            const shared_data_item_t * item,
                                            const app_lib_data_received_t * data)
{
    Ble_scanner_res_e res;
    cmd_t * cmd_p;
    response_t response;
    Ble_scanner_scanning_config_t conf;

    LOG(LVL_INFO, "Message received on config EP");

    if (data->num_bytes < 5)
    {
        // Too short to contain id + cmd
        return APP_LIB_DATA_RECEIVE_RES_NOT_FOR_APP;
    }

    cmd_p = (cmd_t *) data->bytes;

    switch (cmd_p->cmd_id)
    {
        case SCANNER_CMD_DISABLE:
            res = Ble_scanner_stop();
            break;
        case SCANNER_CMD_ENABLE:
            conf.type = BLE_SCANNER_SCANNING_TYPE_ALWAYS;
            conf.channel = cmd_p->payload.enable.channel;

            // This is a start command
            res = Ble_scanner_start(&conf);
            LOG(LVL_INFO, "Enable always = %d (channel=%d)",
                    res,
                    conf.channel);
            break;
        case SCANNER_CMD_ENABLE_PERIODIC:
            conf.type = BLE_SCANNER_SCANNING_TYPE_PERIODIC;
            conf.period_s = cmd_p->payload.enable_periodic.period_s;
            conf.scan_length_s = cmd_p->payload.enable_periodic.scan_length_s;
            conf.channel = cmd_p->payload.enable_periodic.channel;

            // This is a start command
            res = Ble_scanner_start(&conf);
            LOG(LVL_INFO, "Enable periodic = %d (%ds every %ds on channel=%d))",
                    res,
                    conf.scan_length_s,
                    conf.period_s,
                    conf.channel);
            break;
        default:
            LOG(LVL_DEBUG, "Wrong cmd format");
            return APP_LIB_DATA_RECEIVE_RES_NOT_FOR_APP;
    }

    response.id = cmd_p->id;
    response.res = res;

    // Send reply
    app_lib_data_to_send_t reply = {
        .bytes = (uint8_t *) &response,
        .num_bytes = sizeof(response),
        .dest_address = APP_ADDR_ANYSINK,
        .qos = APP_LIB_DATA_QOS_NORMAL,
        .src_endpoint = BLE_SCANNER_CONFIG_REPLY_EP,
        .dest_endpoint = BLE_SCANNER_CONFIG_REPLY_EP
    };

    // Answer backend with same endpoint
    Shared_Data_sendData(&reply,NULL);

    return APP_LIB_DATA_RECEIVE_RES_HANDLED;
}

/** Filter to receive cmd messages */
static shared_data_item_t ble_scanner_control =
{
    .cb = ble_scanner_configuration_cb,
    .filter =
        {
            .mode = SHARED_DATA_NET_MODE_ALL,
            .src_endpoint = BLE_SCANNER_CONFIG_REQ_EP,
            .dest_endpoint = BLE_SCANNER_CONFIG_REQ_EP,
            .multicast_cb = NULL
        }
};

volatile uint32_t tempo = 0;
static size_t on_beacon_received(Ble_scanner_beacon_t beacons[],
                                 size_t beacons_available)
{
    Led_toggle(0);

    // Handle the beacon one by one
    // No packing yet: could be stored in temporary buffer until a
    // full packet is ready
    app_lib_data_to_send_t data = {
        .bytes = (uint8_t *) beacons,
        .num_bytes = sizeof(Ble_scanner_beacon_t) - BLE_SCANNER_MAX_BEACON_SIZE + beacons[0].length,
        .dest_address = APP_ADDR_ANYSINK,
        .qos = APP_LIB_DATA_QOS_NORMAL,
        .src_endpoint = BLE_SCANNER_DATA_EP,
        .dest_endpoint = BLE_SCANNER_DATA_EP
    };

    // This is just a demo code so if packet is not transmitted,
    // it is lost
    Shared_Data_sendData(&data,NULL);

    return 1;
}


void App_init(const app_global_functions_t * functions)
{
#ifdef DUALMCU_INTERFACE
    Dualmcu_lib_init(1000000, false);
#else
    // Enable log as dualmcu is not enabled
    LOG_INIT();
    // Configure node
    // Basic configuration of the node with a unique node address
    if (configureNodeFromBuildParameters() != APP_RES_OK)
    {
        // Could not configure the node
        // It should not happen except if one of the config value is invalid
        return;
    }
    // Start the stack
    Stack_State_startStack();
#endif

    LOG(LVL_INFO, "Starting BLE Scanner app");
    Led_init();

    Ble_scanner_init(ble_scanner_filter, on_beacon_received);

    // Register data filter to control the configuration remotely
    Shared_Data_addDataReceivedCb(&ble_scanner_control);
}
