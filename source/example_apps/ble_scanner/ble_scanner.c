/* Copyright 2021 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/*
 * \file    ble_scanner
 * \brief   This lib control the ble scanning feature from the stack
 */

#include "api.h"
#include "app_scheduler.h"
#include "ble_scanner.h"

#include <string.h>

#define DEBUG_LOG_MODULE_NAME "BLE_SCANNER_LIB"
#define DEBUG_LOG_MAX_LEVEL LVL_INFO
#include "debug_log.h"

/* Is module initialized */
static bool m_initialized = false;

/* Max number of eeacons to store */
#define BLE_SCAN_QUEUE_SIZE 16

/* Intermediate buffer to store beacons between radio and task */
Ble_scanner_beacon_t m_ble_data[BLE_SCAN_QUEUE_SIZE];

/* FIFO management*/
static uint8_t m_insert_index = 0;
static uint8_t m_free_rooms = BLE_SCAN_QUEUE_SIZE;

/* Module caller callbacks*/
static ble_scanner_filter_cb m_app_beacon_filter = NULL;
static ble_scanner_beacon_received_cb m_beacon_received_cb = NULL;

/* Current config set by caller */
static Ble_scanner_scanning_config_t m_current_config;

static app_lib_beacon_rx_channels_mask_e get_beacon_rx_channel(Ble_scanner_channel_e channel)
{
    switch(channel)
    {
        case BLE_SCANNER_CHANNEL_37:
            return APP_LIB_BEACON_RX_CHANNEL_37;
        case BLE_SCANNER_CHANNEL_38:
            return APP_LIB_BEACON_RX_CHANNEL_38;
        case BLE_SCANNER_CHANNEL_39:
            return APP_LIB_BEACON_RX_CHANNEL_39;
        case BLE_SCANNER_CHANNEL_ANY:
        default:
            return APP_LIB_BEACON_RX_CHANNEL_ALL;
    }
}

static uint32_t process_ble_beacon(void)
{
    uint8_t index;
    uint8_t beacons_available = 0;
    size_t beacons_to_handle, beacons_handled = 0;

    beacons_available = BLE_SCAN_QUEUE_SIZE - m_free_rooms;
    if (beacons_available == 0)
    {
        return APP_SCHEDULER_STOP_TASK;
    }

    // Determine start index
    // Protect reading to have m_free_rooms and m_insert_index coherent
    lib_system->enterCriticalSection();
    index = (m_insert_index + m_free_rooms) % BLE_SCAN_QUEUE_SIZE;
    lib_system->exitCriticalSection();

    // Notify callback of beacon available
    // To avoid copies, only send beacons that are
    // contiguous in memory
    if (index + beacons_available < BLE_SCAN_QUEUE_SIZE)
    {
        beacons_to_handle = beacons_available;
    }
    else
    {
        beacons_to_handle = BLE_SCAN_QUEUE_SIZE - index;
    }

    beacons_handled = m_beacon_received_cb(&m_ble_data[index], beacons_to_handle);

    // Protect update to m_free_rooms to avoid races with IRQ as addition is not atomic
    lib_system->enterCriticalSection();
    m_free_rooms += beacons_handled;
    lib_system->exitCriticalSection();

    // Shedule task again in case there was still something in queue
    return APP_SCHEDULER_SCHEDULE_ASAP;
}

static uint32_t toggle_scanner_mode(void)
{
    // Toogle beacon scanner depending on config and current state
    if (lib_beacon_rx->isScannerStarted())
    {
        if (lib_beacon_rx->stopScanner() != APP_RES_OK)
        {
           // Cannot stop, try again in 1s
           return 1000;
        }
        LOG(LVL_DEBUG, "Scanner stopped");
        return (m_current_config.period_s - m_current_config.scan_length_s) * 1000;
    }
    else
    {
        if (lib_beacon_rx->startScanner(get_beacon_rx_channel(m_current_config.channel)) != APP_RES_OK)
        {
            // Cannot start, try again in 1s
            return 1000;
        }

        LOG(LVL_DEBUG, "Scanner started");
        return m_current_config.scan_length_s * 1000;
    }
}

static void BLEdataReceivedCb(const app_lib_beacon_rx_received_t * packet)
{
    if (m_app_beacon_filter != NULL && !m_app_beacon_filter(packet))
    {
        /* Packet is filtered by app filtering */
        return;
    }

    /* Not filtered out, insert it in the FIFO */
    /* Overriding older entry if needed */
    m_ble_data[m_insert_index].length = packet->length;
    m_ble_data[m_insert_index].type   = packet->type;
    m_ble_data[m_insert_index].rssi   = packet->rssi;

    memcpy(m_ble_data[m_insert_index].data,
           packet->payload,
           packet->length);

    m_insert_index = (m_insert_index + 1) % BLE_SCAN_QUEUE_SIZE;
    if (m_free_rooms > 0)
    {
        m_free_rooms--;
    }

    App_Scheduler_addTask_execTime(process_ble_beacon,
                                   APP_SCHEDULER_SCHEDULE_ASAP,
                                   500);

}

Ble_scanner_res_e Ble_scanner_init(ble_scanner_filter_cb beacon_filter_cb,
                                   ble_scanner_beacon_received_cb beacon_received_cb)
{
    lib_beacon_rx->setBeaconReceivedCb(BLEdataReceivedCb);
    m_app_beacon_filter = beacon_filter_cb;
    m_beacon_received_cb = beacon_received_cb;

    m_insert_index = 0;
    m_free_rooms = BLE_SCAN_QUEUE_SIZE;

    m_initialized = true;
    return BLE_SCANNER_RES_SUCCESS;
}

Ble_scanner_res_e Ble_scanner_start(Ble_scanner_scanning_config_t * config)
{
    if (!m_initialized)
    {
        return BLE_SCANNER_RES_UNINTIALLIZED;
    }

    // Check config is valid
    if (config->type == BLE_SCANNER_SCANNING_TYPE_PERIODIC)
    {
        // Check that interval is correct
        if (config->scan_length_s > config->period_s)
        {
            return BLE_SCANNER_RES_INVALID_PARAM;
        }

        if (App_Scheduler_addTask_execTime(toggle_scanner_mode,
                                    APP_SCHEDULER_SCHEDULE_ASAP,
                                    100) != APP_SCHEDULER_RES_OK)
        {
            LOG(LVL_ERROR, "Cannot add task to toggle scanner");
            return BLE_SCANNER_RES_INTERNAL_ERROR;
        }

        // Save new config
        m_current_config = *config;

        // Disable scanner to start
        lib_beacon_rx->stopScanner();
    }
    else
    {
        // Stop task an scanner in case it was already in periodic mode
        App_Scheduler_cancelTask(toggle_scanner_mode);
        lib_beacon_rx->stopScanner();

        app_res_e res = lib_beacon_rx->startScanner(get_beacon_rx_channel(m_current_config.channel));
        LOG(LVL_INFO, "Starting scanner %d", res);
        if (res != APP_RES_OK)
        {
            LOG(LVL_ERROR, "Cannot start scanner");
            return BLE_SCANNER_RES_INTERNAL_ERROR;
        }
    }

    return BLE_SCANNER_RES_SUCCESS;
}

Ble_scanner_res_e Ble_scanner_stop()
{
    if (!m_initialized)
    {
        return BLE_SCANNER_RES_UNINTIALLIZED;
    }

    // Cancel task in case it was active
    App_Scheduler_cancelTask(toggle_scanner_mode);
    // Do it in whatever state we were
    lib_beacon_rx->stopScanner();

    // Clean our buffer
    m_insert_index = 0;
    m_free_rooms = BLE_SCAN_QUEUE_SIZE;

    return BLE_SCANNER_RES_SUCCESS;
}

