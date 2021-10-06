/**
 * @file    posapp_settings.h
 * @brief   Header for the application settings source.
 * @copyright Wirepas Ltd 2021
 */
#ifndef POSAPP_SETTINGS_H
#define POSAPP_SETTINGS_H

#include <stdint.h>
#include <stdbool.h>

#include "api.h"
#include "node_configuration.h"
#include "poslib.h"
#ifdef CONF_USE_PERSISTENT_MEMORY
#include "app_persistent.h"
#endif

/** Magic id definition in PosLib persistent data 
*   !!! DO NOT change
*/
#define POSLIB_RECORD_MAGIC                 0x2020

#define POSAPP_TAG_DEFAULT_ROLE POSLIB_MODE_NRLS_TAG
#define POSAPP_ANCHOR_DEFAULT_ROLE POSLIB_MODE_OPPORTUNISTIC_ANCHOR

/** Node settings structure 
*   !!! DO NOT change the structure size
*   the rfu bytes can be used for additional field
*/
typedef struct __attribute__ ((packed))
{
    /** Node stack configuration */
    uint32_t address;
    uint32_t network_address;
    uint8_t network_channel;
    uint8_t role;
    uint8_t rfu[6]; //reserved for further usage
} node_persistent_settings_t;

typedef struct __attribute__ ((packed))
{
    /** Mini-beacon configuration */
    uint8_t type;
    uint8_t length;
    uint8_t value[POSLIB_MBCN_RECORD_MAX_SIZE];
} poslib_mbcn_record_persistent_t;

/** PosLib persistent storage struct poslib_persistent_settings_t version
* !!! Must be incremented every time a new field is introduced
*/
#define POSLIB_PERSISTENT_VERSION 2

/** PosLib persistent settings 
 *  !!! DO NOT remove any field previously defined
 *  New fields shall be added at the end of the structure
 */
typedef struct __attribute__ ((packed))
{
    uint16_t poslib_record_magic;
    uint8_t poslib_settings_version;
    // PosLib persistent version: 1
    uint8_t node_mode;
    uint8_t node_class;
    uint32_t update_period_static_s;
    uint32_t update_period_dynamic_s;
    uint32_t update_period_offline_s;
    uint8_t ble_type;
    uint8_t ble_mode;
    uint16_t ble_activation_delay_s;
    int8_t ble_eddystone_tx_power;
    uint8_t ble_eddystone_channels;
    uint16_t ble_eddystone_tx_interval_ms;
    int8_t ble_ibeacon_tx_power;
    uint8_t ble_ibeacon_channels;
    uint16_t ble_ibeacon_tx_interval_ms;
    bool motion_enabled;
    uint16_t motion_threshold_mg;
    uint16_t motion_duration_ms;
    // PosLib persistent version: 2 (including fields above)
    bool mbcn_enabled;
    uint16_t mbcn_tx_interval_ms;
    poslib_mbcn_record_persistent_t mbcn_records[POSLIB_MBCN_RECORDS];
    bool da_routing_enabled;
    bool da_follow_network;
} poslib_persistent_settings_t;


/**
 *  This is the combined structure which will be written to flash
 *  The size of this structure cannot be larger than available 
 *  flash space
 */
typedef struct __attribute__ ((packed))
{
    node_persistent_settings_t node;
    poslib_persistent_settings_t poslib;
} posapp_persistent_settings_t;


/**
 * @brief   Configure the node network parameters.
 *          Node settings are either the default or persistent 
 *          storage (when available)
 * @param   void
 * 
 * @return  bool
 *          true: node configured,  false: error 
 */
bool PosApp_Settings_configureNode(void);

/**
 * @brief   Provides the PosLib specific settings
 * 
 * @param   settings 
 *          Pointer to a poslib_settings_t structure where settings 
 *          will be written
 * @return  void
 */
void PosApp_Settings_get(poslib_settings_t * settings);

/**
 * @brief   Stores the PosLib specific settings tp persistent
 * 
 * @param   settings 
 *          Pointer to a poslib_settings_t structure containing the settings 
 *          will be written
 * @return  bool
 *          true: settings saved, false: settings not saved 
 *          due to: error | persistent not supported | no change
 */
bool PosApp_Settings_store(poslib_settings_t * settings);

#endif