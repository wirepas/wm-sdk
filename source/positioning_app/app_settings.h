/**
    @file  app_settings.h
    @brief Header for the application settings source.
    @copyright Wirepas Oy 2019
*/
#ifndef POS_SETTINGS_H
#define POS_SETTINGS_H

#include <stdint.h>
#include <stdbool.h>

#include "api.h"
#include "node_configuration.h"
#include "ble_beacon.h"
#if defined (USE_PERSISTENT_MEMORY)
#include "persistent.h"

#define MAX_READ_SIZE  10
#endif


// Endpoint for measurements
#define DESTINATION_ENDPOINT 238
#define SOURCE_ENDPOINT      238

/** Access cycle **/
#define ACCESS_CYCLE_S       2

#define TIMEOUT_CONNECTION   ((uint8_t) ACCESS_CYCLE_S * 15) // network connection timeout
#define TIMEOUT_ACK   ((uint8_t) ACCESS_CYCLE_S * 15)  // data packet ACK timeout


/**
    @brief  defines the device's positioning mode.
*/
typedef enum
{
    POS_APP_MODE_NRLS_TAG = 1,
    // NRLS does not function for ANCHOR
    POS_APP_MODE_AUTOSCAN_TAG = 2,
    POS_APP_MODE_AUTOSCAN_ANCHOR = 3,
    POS_APP_MODE_OPPORTUNISTIC_ANCHOR = 4,
} positioning_mode_e;

// These are the default mode the node will switch in case of a configuration conflict
// Such a conflict can occur if the node role is changed through remote API e.g. a subnode
// is set to be headnode. In such case the anchor will have the tag mode which is incorrect

#define DEFAULT_ANCHOR_MODE POS_APP_MODE_OPPORTUNISTIC_ANCHOR
#define DEFAULT_TAG_MODE POS_APP_MODE_AUTOSCAN_TAG

/**
    @brief  defines the device's operational class.
*/
typedef enum
{
    POS_APP_CLASS_DEFAULT = 0xF9,
    POS_APP_CLASS_A = 0xFA,
    POS_APP_CLASS_B = 0xFB,
    POS_APP_CLASS_C = 0xFC,
    POS_APP_CLASS_D = 0xFD,
    POS_APP_CLASS_E = 0xFE,
    POS_APP_CLASS_F = 0xFF,
} positioning_class_e;


/**
    @brief application settings to avoid overdefinitions.
*/
typedef struct
{
    uint8_t destination_endpoint;
    uint8_t source_endpoint;

    uint8_t max_beacons;

    ble_beacon_setup_e ble_beacon_setup;
    uint8_t ble_beacon_selection;
    uint16_t ble_beacon_offline_waittime;

    positioning_class_e node_class;
    positioning_mode_e node_mode;
    app_addr_t node_address;
    app_lib_data_qos_e payload_qos;

    app_lib_settings_net_addr_t network_address;
    app_lib_settings_net_channel_t network_channel;
    app_lib_settings_role_t node_role;
    app_lib_settings_base_role_e node_role_base;
    app_lib_settings_flag_role_e node_role_flag;
    app_lib_state_scan_nbors_type_e scan_filter;

    uint8_t access_cycle;

    uint8_t timeout_ack;
    uint8_t timeout_connection;

     uint32_t scan_period;
} positioning_settings_t;

#if defined (USE_PERSISTENT_MEMORY)
typedef struct
{
    uint32_t node_address;
    uint32_t network_address;
    uint8_t network_channel;
    uint8_t node_role;
} persistent_settings_t;
#endif

positioning_settings_t* Pos_get_settings(void);

void Pos_settings_init(bool on_boot);
void App_Settings_configureNode(bool on_boot);
void App_Settings_initBleBeacon();
bool is_node_mode_anchor(positioning_mode_e node_mode);
bool is_node_mode_tag(positioning_mode_e node_mode);

#endif /*POS_SETTINGS*/
