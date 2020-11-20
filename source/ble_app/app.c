/* Copyright 2018 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/*
 * \file    app.c
 * \brief   This file is a template for using Bluetooth Scanner feature
 *
 * \note    Send a number to a node, to start listening to Eddystone beacons.
 *          "numbers" of theses Beacon will be forwarded to Sink.
 */

#include "api.h"
#include "node_configuration.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>
#include <sl_list.h>

/** BLE advertising PDU has a maximum length of 37 bytes */
#define MAX_BLE_PKT_SIZE 37

/** Endpoint to change the number of requested BT Beacon */
#define SET_NUM_BTLE_EP  10

/** Endpoint to send data */
#define DATA_EP           1

/** Time to process Beacon in the Queue */
#define EXECUTION_TIME_US 100

/** Number of Beacon forwarded to sink */
static uint8_t m_btle_forwarded;

/** Bluetooth data Queue Head */
sl_list_head_t m_btle_queue;

/** Bluetooth Queue depth */
#define BLE_SCAN_QUEUE_SIZE 8

/** Bluetooth data Data */
typedef struct {
    sl_list_t list;
    uint8_t btle_data[MAX_BLE_PKT_SIZE];
    uint8_t btle_length;
    uint8_t btle_type;
    uint8_t btle_rssi;
} ble_data_t;

ble_data_t m_ble_data[BLE_SCAN_QUEUE_SIZE];

/* Number of BT beacon to be forwarded to sink  */
static uint8_t m_num_ble;

/** Node configuration : must be LowLatency Headnode or Subnode */
#define NODE_ROLE \
    app_lib_settings_create_role(APP_LIB_SETTINGS_ROLE_HEADNODE, APP_LIB_SETTINGS_ROLE_FLAG_LL)

/** Bluetooth Channels used for scanning beacon */
/** Could be channel 37, 38, 39 or any mix of them */
#define BEACON_RX_CHAN APP_LIB_BEACON_RX_CHANNEL_ALL

/** Eddystone Header */
const uint8_t  EDDYSTONE_OFFSET = 0x9;
const uint8_t  EDDYSTONE_LEN    = 0x3;          /* Length       */
const uint8_t  EDDYSTONE_TYPE   = 0x3;          /* Service List */
const uint8_t  EDDYSTONE_UUID[] = {0xAA, 0xFE}; /* Eddystone ID */


/**
 * \brief   Functions used to forward Bluetooth Adv. packet to Sink
 */
static void send_data(ble_data_t * beacon)
{
    app_lib_data_to_send_t data_to_send;

    data_to_send.bytes = (const uint8_t *)(beacon->btle_data);
    data_to_send.num_bytes = beacon->btle_length;
    data_to_send.dest_address = APP_ADDR_ANYSINK;
    data_to_send.src_endpoint = DATA_EP;
    data_to_send.dest_endpoint = DATA_EP;
    data_to_send.qos = APP_LIB_DATA_QOS_HIGH;
    data_to_send.delay = 0;
    data_to_send.flags = APP_LIB_DATA_SEND_FLAG_NONE;
    data_to_send.tracking_id = APP_LIB_DATA_NO_TRACKING_ID;

    lib_data->sendData(&data_to_send);
}

/**
 * \brief   called by BLEdataReceivedCb, using setPeriodicCb mechanism.
 *          It's the place where beacon should be processed.
 */
static uint32_t process_ble_beacon(void)
{
    ble_data_t * beacon = NULL;
    size_t routeToSink = 0;

    lib_state->getRouteCount(&routeToSink);

    /* When we enable scanner feature, we loose connectivity:
     * the time needed to move to a BT advertisement channel
     * it's better to avoid sending data during this time */
    if (routeToSink)
    {
        /* we got a route to the sink */
        /* Check if we have a BT Beacon to Forward */
        do
        {
            lib_system->enterCriticalSection();
            beacon = (ble_data_t *)sl_list_pop_front(&m_btle_queue);
            lib_system->exitCriticalSection();

            if (beacon != NULL)
            {
                send_data(beacon);

                /* Free entry in Beacon Queue */
                beacon->btle_length = 0;
                m_btle_forwarded++;
            }
        } while ((beacon != NULL) && (m_btle_forwarded < m_num_ble));
    }

    if (m_btle_forwarded >= m_num_ble)
    {
        /* We got enough BLE Beacons */
        lib_beacon_rx->stopScanner();
    }

    /* Beacon Queue is now empty */
    return APP_LIB_SYSTEM_STOP_PERIODIC;
}

/**
 * \brief   Called when the Stack got a Bluetooth beacon.
 *          This function runs in interrupt context !
 *          execution time must be as short as possible
 * \param   packet
 *          Received BLE Beacon
 */
static void BLEdataReceivedCb(const app_lib_beacon_rx_received_t * packet)
{
    uint16_t offset = EDDYSTONE_OFFSET;

    ble_data_t * data = NULL;


    /* Here, some filtering should be implemented to avoid processing
     * every Bluetooth Beacon
     * As an example, we filter the incoming Bluetooth beacon to process
     * only Eddystone URL beacon.
     * They can be recognized by there special header at address 9 to 12
     * in the Beacon payload received.
     */
    if ((packet->payload[offset++] == EDDYSTONE_LEN )    &&
        (packet->payload[offset++] == EDDYSTONE_TYPE)    &&
        (packet->payload[offset++] == EDDYSTONE_UUID[0]) &&
        (packet->payload[offset]   == EDDYSTONE_UUID[1]))
    {
        /* Look for a free place to store incoming Eddystone Beacon */
        for (uint8_t idx=0; idx<BLE_SCAN_QUEUE_SIZE; idx++)
        {
            if (m_ble_data[idx].btle_length == 0)
            {
                data = &m_ble_data[idx];
                break;
            }
        }

        /* No free place, discard the oldest and use it */
        if (data == NULL)
        {
            data = (ble_data_t *)sl_list_pop_front(&m_btle_queue);
        }

        if (data == NULL)
        {
            return; /* Should not occur */
        }

        data->btle_length = packet->length;
        data->btle_type   = packet->type;
        data->btle_rssi   = packet->rssi;

        memcpy(data->btle_data,
               packet->payload,
               data->btle_length);

        sl_list_push_back(&m_btle_queue, (sl_list_t *)data);

        /* BLE Beacon received, run periodic work as soon as possible */
        lib_system->setPeriodicCb(process_ble_beacon,
                                  0,
                                  EXECUTION_TIME_US);
    }
    else
    {
        /* Nothing:
         * It's not an Eddystone beacon, and we do not process it */
    }
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
    bool btle_started = lib_beacon_rx->isScannerStarted();

    if ((data->num_bytes != 1) ||
        (data->dest_endpoint != SET_NUM_BTLE_EP))
    {
        // Data was not for this application
        return APP_LIB_DATA_RECEIVE_RES_NOT_FOR_APP;
    }

    // Parse decimal digits a number of BT Beacon wanted
    m_num_ble = data->bytes[0];
    m_btle_forwarded = 0;

    if ((!btle_started)&&(m_num_ble>0))
    {
        for (uint8_t idx=0; idx<BLE_SCAN_QUEUE_SIZE; idx++)
        {
            m_ble_data[idx].btle_length = 0;
        }

        sl_list_init(&m_btle_queue);

        // Start BT Sanner now, until we forward m_num_ble beacon to the sink.
        lib_beacon_rx->startScanner(BEACON_RX_CHAN);

    }

    // Data handled successfully
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
    // Basic configuration of the node with a unique node address
    if (configureNodeFromBuildParameters() != APP_RES_OK)
    {
        // Could not configure the node
        // It should not happen except if one of the config value is invalid
        return;
    }

    // Configure node as Headnode or Subnode, low-latency
    // This call force the role, and prevent RemoteAPI to change it
    lib_settings->setNodeRole(NODE_ROLE);

    // Set callback for received BT Beacon.
    // This callback will be call in Interrupt Context !
    lib_beacon_rx->setBeaconReceivedCb(BLEdataReceivedCb);

    // Set callback for received unicast messages
    lib_data->setDataReceivedCb(dataReceivedCb);

    // Start the stack
    lib_state->startStack();
}
