/**  Copyright 2020 Wirepas Ltd. All Rights Reserved.
 *
 *   See file LICENSE.txt for full license details.
 *
 */
#ifndef _POSLIB_H_
#define _POSLIB_H_

/**
 * @brief Return codes of positioning functions
 */
typedef enum
{
    /**< Operation is a success. */
    POS_RET_OK = 0,
    /**< Positioning not in a valid state. */
    POS_RET_INVALID_STATE = 1,
    /**< Invalid parameters. */
    POS_RET_INVALID_PARAM = 2,
    /**< Invalid data. */
    POS_RET_INVALID_DATA = 3,
    /**< Positioning library error. */
    POS_RET_LIB_ERROR = 4,
    /**< Internal error (no more task, ...). */
    POS_RET_INTERNAL_ERROR = 5,
} poslib_ret_e;

/**
 * @brief  defines the device's positioning mode.
 */
typedef enum
{
    POSLIB_MODE_NRLS_TAG = 1,
    POSLIB_MODE_AUTOSCAN_TAG = 2,
    POSLIB_MODE_AUTOSCAN_ANCHOR = 3,
    POSLIB_MODE_OPPORTUNISTIC_ANCHOR = 4,
    POSLIB_MODE_ENDS = 7
} poslib_mode_e;

/**
 * @brief  defines the position motion mode.
 */
typedef enum
{
    POSLIB_MOTION_STATIC = 1,
    POSLIB_MOTION_DYNAMIC = 2,
} poslib_motion_mode_e;

/**
 * @brief  defines the state (started/stopped) of the BLE beacon sent by PosLib
 */
typedef enum
{
    POSLIB_BLE_STOP = 1,
    POSLIB_BLE_START = 2,
} poslib_ble_mode_e;

/**
 * @brief  defines the device's operational class.
 */
typedef enum
{
    POSLIB_CLASS_DEFAULT = 0xF9,
    POSLIB_CLASS_A = 0xFA,
    POSLIB_CLASS_B = 0xFB,
    POSLIB_CLASS_C = 0xFC,
    POSLIB_CLASS_D = 0xFD,
    POSLIB_CLASS_E = 0xFE,
    POSLIB_CLASS_F = 0xFF
} poslib_class_e;

/**
 * @brief  Beacon devices types
 */
typedef enum
{
    POSLIB_EDDYSTONE = 1,
    POSLIB_IBEACON,
    POSLIB_BEACON_ALL,
} poslib_ble_type_e;

/**
 * @brief  defines events which can be used with callback.
 */
typedef enum
{
    POSLIB_UPDATE_START = 1,
    POSLIB_UPDATE_END = 2,
    POSLIB_BLE_START_EVENT = 3,
    POSLIB_BLE_STOP_EVENT = 4,
    POSLIB_WM_SLEEP_REQUEST = 5,
    POSLIB_WM_WAKE_UP_REQUEST = 6,
    POSLIB_APPCONFIG_NEW_SETTINGS = 7,
    POSLIB_UPDATE_ALL = 8,
    POSLIB_BLE_ALL = 9,
    POSLIB_NRLS_ALL = 10,
} poslib_events_e;

/**
 * @brief  defines status returned in PosLib_status().
 */
typedef enum
{
    POSLIB_STARTED_UPDATE_ONGOING = 1,
    POSLIB_STARTED_IDLE = 2,
    POSLIB_STOPPED = 3,
} poslib_status_e;

/**
 * @brief  defines BLE mode mode configration.
 */
typedef struct
{
    int8_t ble_tx_power;
    uint32_t ble_tx_interval_s;
    app_lib_beacon_tx_channels_mask_e ble_channels;
} poslib_ble_mode_config_t;

/**
 * @brief position library ble settings.
 */
typedef struct __attribute((packed))
{
    poslib_ble_type_e ble_type;
    poslib_ble_mode_e ble_mode;
    poslib_ble_mode_config_t ble_eddystone;
    poslib_ble_mode_config_t ble_ibeacon;
    /* defined using PosLib_setConfig. Enables ble sending after WM offline period */
    uint32_t update_period_bleon_offline_s;
} ble_beacon_settings_t;

/**
 * @brief position library settings.
 */
typedef struct __attribute((packed))
{
    /* mode defined using PosLib_setConfig */
    poslib_mode_e node_mode;
    /* class defined using PosLib_setConfig */
    poslib_class_e node_class;
    /* defined using PosLib_setConfig. Used for normal PosLib update period */
    uint32_t update_period_static_s;
    /* defined using PosLib_setConfig. Used when api call PosLib_motion */
    uint32_t update_period_dynamic_s;
    /* defined using PosLib_setConfig. Used when autoscan mode and stack is offline */
    uint32_t update_period_autoscan_offline_s;
    /* endpoint definition for PosLib data */
    uint8_t source_endpoint;
    /* endpoint definition for PosLib data */
    uint8_t destination_endpoint;
    /* defined using PosLib_setConfig. Configuration of ble */
    ble_beacon_settings_t poslib_ble_settings;
} poslib_settings_t;

/** Helper structure to move data from PosLib events*/
typedef struct
{
    uint8_t                        event_id;
    app_lib_time_timestamp_hp_t    time_hp;
    uint32_t                       nrls_sleep_time_sec;
} poslib_events_info_t;

/**
 * @brief   Function used with PosLib_event_setCb
 * @param   struct for poslib_events_listen_info_f
 *          struct for poslib_events_info_t data.
 */
typedef void (*poslib_events_listen_info_f)(poslib_events_info_t * msg);

/**
 * @brief   This callback is used to receive Poslib generated events.
 * @param   Callback function type of poslib_events_listen_info_f.
 *
 */
typedef void
    (*poslib_events_callback_f)
    (poslib_events_listen_info_f callback);

/**
 * @brief   Sets PosLib configuration.
 * @param   poslib_settings_t type pointer
 * @return  See \ref poslib_ret_e
 */
poslib_ret_e PosLib_setConfig(poslib_settings_t * settings);

/**
 * @brief   Gets PosLib configuration.
 * @param   poslib_settings_t type pointer
 * @return  See \ref poslib_ret_e
 */
poslib_ret_e PosLib_getConfig(poslib_settings_t * settings);

/**
 * @brief   Start the positioning updates according to provided configuration.
 *          Calls PosLib initialization function before start.
 * @note    Following shared libraries needs to be initialized before use
 *          PosLib_start():
 *          Shared_Data_init();
 *          Shared_Appconfig_init();
 *          Shared_Neighbors_init();
 *          Shared_Shutdown_init();
 * @return  See \ref poslib_ret_e
 */
poslib_ret_e PosLib_start(void);

/**
 * @brief   Stops the positioning updates.
 * @return  See \ref poslib_ret_e
 */
poslib_ret_e PosLib_stop(void);

/**
 * @brief   Triggers one position update. If PosLib is started the scheduled
 *          operation will continue after the one-shot update with the
 *          respective schedule. If in stop mode then will stay in stop mode
 *          after the one-shot update.
 * @note    If called during NRLS sleep, sleep is stopped which triggers one
 *          position update. NRLS period continues with the respective schedule.
 * @return  See \ref poslib_ret_e
 */
poslib_ret_e PosLib_oneshot(void);

/**
 * @brief   Is used to indicate that the tag is static or dynamic. The update
 *          rate used will be adjusted according to the state:  when moving from
 *          static → dynamic:  an update will be triggered immediately if the
 *          last update is older than (current time - dynamic rate),
 *          otherwise the next update is re-scheduled as
 *          (last update + dynamic rate).
 * @param   mode type of poslib_motion_mode_e
 * @return  See \ref poslib_ret_e
 */
poslib_ret_e PosLib_motion(poslib_motion_mode_e mode);

/**
 * @brief   Returns the current status of PosLib.
 * @return  See \ref poslib_ret_e
 */
poslib_status_e PosLib_status(void);

/**
 * @brief   Sets a callback for the requested event(s). It shall be possible to
 *          subscribe to several events with the same callback. In this case
 *          the callback function will be called multiple times once for
 *          each event.
 * @note    No possible to set separate callback function for different events.
 * @param   requested_event type of poslib_events_e
 * @param   callback type of poslib_events_listen_info_f
 */
void PosLib_event_setCb(poslib_events_e requested_event,
                        poslib_events_listen_info_f callback);

/**
 * @brief   Clears a callback for the requested event.
 * @param   requested_event type of poslib_events_e
 */
void PosLib_event_clearCb(poslib_events_e requested_event);
#endif
