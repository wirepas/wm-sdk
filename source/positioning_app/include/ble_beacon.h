/**
    @file  ble_beacon.h
    @brief Header for the application Beacon source.
    @copyright Wirepas Oy 2019
*/
#ifndef BLE_BEACON_H
#define BLE_BEACON_H

#include "api.h"

// Wait for this time in seconds before putting on Beacon
#define OFFLINE_TIMER_S      120 // default value

/**
    @brief  Beacon devices types
*/
typedef enum
{
    EDDYSTONE = 1,
    IBEACON,
    BEACON_ALL,
} ble_beacon_type_e;

/**
    @brief  defines the device's Beacon's setup.
*/
typedef enum
{
    BLE_BEACON_OFF = 0,         // Never used
    BLE_BEACON_ON,              // Always used
    BLE_BEACON_ON_WHEN_OFFLINE, // Used when offline
} ble_beacon_setup_e;


/**
    @brief  defines Beacon's monitoring.
*/
typedef enum
{
    MON_CHECK = 0,      // Check the monitoring
    MON_CONN_FAIL,      // Connection fail
    MON_UPD_APPCONF,    // Got the appconfig message
    MON_UPD_LATEST_MSG, // Got the ack for the latest message
} ble_beacon_monitoring_e;

/**
    @brief      Stops sending Bluetooth Low Energy Beacons
*/
void beacon_disable();

/**
    @brief      Initializes Beacon module.
    @param[in]  setup
                Beacon's configuring settings
*/
void Ble_Beacon_init(ble_beacon_setup_e setup);

/**
    @brief  Set Beacon configuration
*/
void Ble_Beacon_set_configuration();

/**
    @brief      Beacon monitoring
    @param[in]  state
                check or update tthe connection
*/
void Ble_Beacon_check_monitoring(ble_beacon_monitoring_e command);

#endif /* BLE_BEACON_H */
