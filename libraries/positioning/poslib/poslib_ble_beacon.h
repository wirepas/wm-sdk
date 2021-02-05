/**
 * @file  poslib_ble_beacon.h
 * @brief Header for the application Beacon source.
 * @copyright Wirepas Ltd 2020
 */
#ifndef POSLIB_BLE_BEACON_H
#define POSLIB_BLE_BEACON_H

/**
 * @brief  defines states for ble activation when no beacons detected.
 */
typedef enum
{
    /** State when beacons loss is not detected */
    POSLIB_BLE_OFFLINE_IDLE = 1,
    /** State when beacons loss is detected and task for ble activation started */
    POSLIB_BLE_OFFLINE_DETECTED = 2,
    /** State when beacons loss detected, route loss detected and ble activated */
    POSLIB_BLE_OFFLINE_ACTIVATED = 3,
} poslib_bleoffline_status_e;

/**
 * @brief   Initializes Beacon module.
 * @param   settings type of ble_beacon_settings_t
 */
void PosLibBle_init(ble_beacon_settings_t * settings);

/**
 * @brief   Starts periodic check if beacons are received after scan if
 *          poslib_ble_settings.update_period_bleon_offline_s is set > 0.
 *          If stack is set to sleeping periodic check is stopped.
 * @return  time interval to run task again
 *          CHECK_BLEON_OFFLINE_BEACONS_MS
 */
uint32_t PosLibBle_check_offline(void);

/**
 * @brief   Sets and starts Beacon configuration or stops activated beacons.
 * @param   mode
 *          type poslib_ble_mode_e
 * @return  POS_RET_OK, POS_RET_INVALID_PARAM or POS_RET_INVALID_STATE.
 */
poslib_ret_e PosLibBle_set(poslib_ble_mode_e mode);

/**
 * @brief   Check if active beacons actovated from PosLib
 * @return  true if active ble connections from Poslib
 */
bool PosLibBle_checkActive(void);

#endif /* POSLIB_BLE_BEACON_H */
