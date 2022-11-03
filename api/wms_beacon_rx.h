/* Copyright 2018 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/**
 * @file wms_beacon_rx.h
 *
 * Application library for Bluetooth LE beacon RX
 *
 * Library services are accessed  via @ref app_lib_beacon_rx_t "lib_beacon_rx"
 * handle.
 */
#ifndef APP_LIB_BEACON_RX_H_
#define APP_LIB_BEACON_RX_H_

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "wms_app.h"

/** @brief Library symbolic name  */
#define APP_LIB_BEACON_RX_NAME 0x0cb93068

/** @brief Maximum supported library version */
#define APP_LIB_BEACON_RX_VERSION 0x200

/**
 * @brief  BLE Advertising channels to listen, used with @ref
 *         app_lib_beacon_rx_start "lib_beacon_rx->startScanner()" service.
 */
typedef enum
{
    /** Use advertising channel 37 */
    APP_LIB_BEACON_RX_CHANNEL_37  = 0x1,
    /** Use advertising channel 38 */
    APP_LIB_BEACON_RX_CHANNEL_38  = 0x2,
    /** Use advertising channel 39 */
    APP_LIB_BEACON_RX_CHANNEL_39  = 0x4,
    /** Use all advertising channels */
    APP_LIB_BEACON_RX_CHANNEL_ALL = 0x7
} app_lib_beacon_rx_channels_mask_e;

/**
 * @brief  BLE structure received from network. Used in callback function set
 *         with @ref app_lib_beacon_rx_set_data_received_cb_f
 *         "lib_beacon_rx->setBeaconReceivedCb()" service.
 */
typedef struct
{
    /** PDU type (BlueTooth Core_v5.0.pdf, p2567) */
    uint8_t     type;
    /** Number of bytes of data payload */
    uint8_t     length;
    /** RSSI from packet received */
    int8_t      rssi;
    /** Data payload */
    uint8_t     * payload;
} app_lib_beacon_rx_received_t;

/**
 * @brief   Function type for BLE Advertisement packet received callback
 * @param   packet
 *          Received packet
 *
 * Used with @ref app_lib_beacon_rx_set_data_received_cb_f
 *         "lib_beacon_rx->setBeaconReceivedCb()" service.
 */
typedef void (*app_lib_beacon_rx_data_received_cb_f)
    (const app_lib_beacon_rx_received_t * packet);

/**
 * @brief   Set a callback to be called when data packet received
 *
 * Example on use:
 *
 * @code
 *
 * static void BLEdataReceivedCb(const app_lib_beacon_rx_received_t * packet)
 * {
 *     ...
 * }
 *
 * void App_init(const app_global_functions_t * functions)
 * {
 *
 *     // Set callback for received BT Beacon.
 *     // This callback will be call in Interrupt Context !
 *     lib_beacon_rx->setBeaconReceivedCb(BLEdataReceivedCb);
 *     ...
 *
 *     // Start the stack
 *     lib_state->startStack();
 * }
 * @endcode
 *
 * @param   cb
 *          The function to be executed, or NULL to unset
 * @note    This callback is called from IRQ context so keep the processing time
 *          at minimum!
 */
typedef void
    (*app_lib_beacon_rx_set_data_received_cb_f)(app_lib_beacon_rx_data_received_cb_f cb);

/**
 * @brief   Start BLE advertisement scanner
 *
 * For example on how to use this function, check @ref app_lib_beacon_rx_started
 * "lib_beacon_rx->isScannerStarted()".
 *
 * @param   channel mask
 *          BlueTooth advertisement channel mask to use
 * @returns Result code, @ref APP_RES_OK if successful,
 *          @ref APP_RES_INVALID_STACK_STATE if BLE Scanner is already running,
 *          @ref APP_RES_INVALID_NULL_POINTER if no callback set
 *
 * @note    BLE advertisement scanner scanning in Low Energy Mode (see @ref
 *          app_lib_settings_set_node_role_f
 *          "lib_settings->setNodeRole()") is an experimental
 *          feature in WM FW v5.1.0 and shall not be used by Wirepas licensees.
 *          No support is provided on this feature.
 */
typedef app_res_e
    (*app_lib_beacon_rx_start)(app_lib_beacon_rx_channels_mask_e channel);

/**
 * @brief   Stop BLE advertisement scanner
 * @returns Result code, @ref APP_RES_OK if successful,
 *          @ref APP_RES_INVALID_STACK_STATE if BLE Scanner is not running
 */
typedef app_res_e
    (*app_lib_beacon_rx_stop)(void);

/**
 * @brief   Check if BLE advertisement scanner is running
 *
 * Example on use. When device receives a packet to destination @ref endpoint
 * "endpoint" 10, it checks whether BLE scanner is active. If not, it will
 * start it.
 *
 * @code
 *
 * #define SET_NUM_BTLE_EP  10
 *
 * static app_lib_data_receive_res_e dataReceivedCb(
 *     const app_lib_data_received_t * data)
 * {
 *     bool btle_started = lib_beacon_rx->isScannerStarted();
 *
 *     if (data->dest_endpoint != SET_NUM_BTLE_EP)
 *     {
 *         // Data was not for this application
 *         return APP_LIB_DATA_RECEIVE_RES_NOT_FOR_APP;
 *     }
 *
 *     if (!btle_started)
 *     {
 *         // Start BT Sanner now
 *         lib_beacon_rx->startScanner(APP_LIB_BEACON_RX_CHANNEL_ALL);
 *     }
 *
 *     // Data handled successfully
 *     return APP_LIB_DATA_RECEIVE_RES_HANDLED;
 * }
 *
 * void App_init(const app_global_functions_t * functions)
 * {
 *
 *     // Set callback for received unicast messages
 *     lib_data->setDataReceivedCb(dataReceivedCb);
 *
 *     // Start the stack
 *     lib_state->startStack();
 * }
 * @endcode
 *
 * @returns Result code, true if running,
 *          false if BLE advertisement scanner is not running
 */
typedef bool
    (*app_lib_beacon_rx_started)(void);


/**
 * @brief   List of library functions
 */
typedef struct
{
    app_lib_beacon_rx_set_data_received_cb_f      setBeaconReceivedCb;
    app_lib_beacon_rx_start                       startScanner;
    app_lib_beacon_rx_stop                        stopScanner;
    app_lib_beacon_rx_started                     isScannerStarted;
} app_lib_beacon_rx_t;

#endif /* APP_LIB_BEACON_RX_H_ */
