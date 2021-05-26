/**
 * @file  poslib_ble_beacon.h
 * @brief Header for the BLE beacons module.
 * @copyright Wirepas Ltd 2021
 */
#ifndef POSLIB_BLE_BEACON_H
#define POSLIB_BLE_BEACON_H

#include "poslib_event.h"
/**
 * @brief   Starts the BLE module and sets the beacons according to 
 *          the provided configuration
 *          Note that if requeste mode is POSLIB_BLE_ON_WHEN_OFFLINE the beacons
 *          will only be started when the node is outside the WM coverage
 * @param   settings
 *          Pointer to ble_beacon_settings_t structure
 * @return  poslib_ret_e
 *          POS_RET_OK: start successful
 */
poslib_ret_e PosLibBle_start(ble_beacon_settings_t * settings);

/**
 * @brief   Stops BLE beacons, the module can only be restarted by 
 *          calling PosLibBle_start  
 * @param   void
 * @return  void
 */
void PosLibBle_stop();

/**
 * @brief   BLE module event processing function  
 *       
 * @param   event the event to process ( pointer to \ref poslib_internal_event_t )
 *          
 * @return  void
 */
 void PosLibBle_processEvent(poslib_internal_event_t * event);

/**
 * @brief   Get the status of BLE Eddystone  beacon
 *       
 * @return  true/false :  started/stoped
 */
 bool PosLibBle_getEddystoneStatus();

/**
 * @brief   Get the status of BLE iBeacon  beacon
 *       
 * @return  true/false :  started/stoped
 */
 bool PosLibBle_getiBeaconStatus();

#endif /* POSLIB_BLE_BEACON_H */
