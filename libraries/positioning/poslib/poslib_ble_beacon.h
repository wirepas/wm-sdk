/**
 * @file  poslib_ble_beacon.h
 * @brief Header for the application Beacon source.
 * @copyright Wirepas Ltd 2020
 */
#ifndef POSLIB_BLE_BEACON_H
#define POSLIB_BLE_BEACON_H

/**
 * @brief   Initializes Beacon module.
 * @param   settings type of ble_beacon_settings_t
 */
void PosLibBle_init(ble_beacon_settings_t * settings);

/**
 * @brief   Sets ble beacons active when Wirepas Mesh Stack in offline.
 */
void PosLibBle_enableOffline();

/**
 * @brief   Sets Beacon configuration
 */
void PosLibBle_set();

/**
 * @brief   Stops sending Bluetooth Low Energy Beacons
 */
void PosLibBle_disable();

/**
 * @brief   Sets mode to ble module
 * @param   mode
 *          type poslib_ble_mode_e
 */
void PosLibBle_setMode(poslib_ble_mode_e mode);

#endif /* POSLIB_BLE_BEACON_H */
