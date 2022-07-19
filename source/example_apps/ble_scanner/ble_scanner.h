/* Copyright 2021 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/**
 * @file ble_scanner.h
 *
 * Bleutooth Low Energy beacons scanner library.
 */

#ifndef _BLE_SCANNER_H_
#define _BLE_SCANNER_H_

/** BLE advertising PDU has a maximum length of 37 bytes */
#define BLE_SCANNER_MAX_BEACON_SIZE 37

typedef enum {
    BLE_SCANNER_RES_SUCCESS = 0, /**< Operation is a success. */
    BLE_SCANNER_RES_UNINTIALLIZED = 1,
    BLE_SCANNER_RES_WRONG_STATE = 2,
    BLE_SCANNER_RES_INVALID_PARAM = 3,
    BLE_SCANNER_RES_INTERNAL_ERROR = 4,
} Ble_scanner_res_e;

/** 
 * BLE beacon received data Data
 */
typedef struct {
    uint8_t type;
    int8_t rssi;
    uint8_t length;
    uint8_t data[BLE_SCANNER_MAX_BEACON_SIZE];
} Ble_scanner_beacon_t;

typedef enum {
    BLE_SCANNER_SCANNING_TYPE_ALWAYS = 0,
    BLE_SCANNER_SCANNING_TYPE_PERIODIC = 1
} Ble_scanner_scanning_type_e;

typedef enum {
    /** Use any advertising channel */
    BLE_SCANNER_CHANNEL_ANY = 0,
    /** Use advertising channel 37 */
    BLE_SCANNER_CHANNEL_37  = 1,
    /** Use advertising channel 38 */
    BLE_SCANNER_CHANNEL_38  = 2,
    /** Use advertising channel 39 */
    BLE_SCANNER_CHANNEL_39  = 3,
    /** Use all advertising channels */
} Ble_scanner_channel_e;

/**
 * \brief   Callback to filter ble beacons received
 * \param   packet
 *          packet received on radio level
 * \return  True to keep it, False to discard
 * \note    Called from Radio IRQ context so execution must be short
 */
typedef bool (*ble_scanner_filter_cb)(const app_lib_beacon_rx_received_t * packet);

/**
 * \brief   Callback to be notified when ble beacons are available
 *          matching the filter
 * \param   beacons
 *          Beacons to handle
 * \param   beacons_available
 *          Number of beacons to handle
 * \return  Number of beacons handled from the table (from the start)
 *          Unhandled beacons will be offered again to app later (immediatelly)
 * \note    Called from task context so execution can be longer (up to 500us)
 */
typedef size_t (*ble_scanner_beacon_received_cb)(Ble_scanner_beacon_t beacons[],
                                                  size_t beacons_available);

/**
 * @brief   Structure to hold a scanning config
 */
typedef struct
{
    Ble_scanner_scanning_type_e type; //< Type of scanning
    uint16_t period_s; //< Scanning period in s, only needed if type is periodic
    uint16_t scan_length_s; //< Lenght of a scan in s, only needed if type is periodic, must be < period_s
    Ble_scanner_channel_e channel; //< Channel(s) to use for scanning
} Ble_scanner_scanning_config_t;

/**
 * \brief   Initialize the ble scanner module
 * \param   beacon_filter_cb
 *          Filter to be used to select the beacons to keep. It is called from
 *          ISR and execution must be really short.
 * \param   beacon_received_cb
 *          Callback called for every beacon that match the filter. It is called
 *          asynchronously and execution time can be up to 5OOus
 * \return  Result code of the operation
 */
Ble_scanner_res_e Ble_scanner_init(ble_scanner_filter_cb beacon_filter_cb,
                                   ble_scanner_beacon_received_cb beacon_received_cb);


/**
 * \brief   Start the beacon scanner with the specified config
 * \param   config
 *          The config to use for ble scanning
 * \return  Result code of the operation
 */
Ble_scanner_res_e Ble_scanner_start(Ble_scanner_scanning_config_t * config);

/**
 * \brief   Stop the beacon scanner
 * \return  Result code of the operation
 */
Ble_scanner_res_e Ble_scanner_stop();

#endif //_BLE_SCANNER_H_