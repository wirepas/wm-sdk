/**  Copyright 2020 Wirepas Ltd. All Rights Reserved.
 *
 *   See file LICENSE.txt for full license details.
 *
 */
#ifndef _POSLIB_H_
#define _POSLIB_H_


#if (APP_LIB_STATE_VERSION > 0x205)
//#define DA_SUPPORT
#endif


/**
 * @brief Endpoints for measurements
 */
#define POS_DESTINATION_ENDPOINT 238
#define POS_SOURCE_ENDPOINT      238

/**
 * @brief Update rate min/max
 */
#define POSLIB_MIN_MEAS_RATE_S   8
#define POSLIB_MAX_MEAS_RATE_S   86400

/**
 * @brief Voltage sampling settings
 */
#define POSLIB_VOLTAGE_SAMPLING_MAX_S 60
#define POSLIB_VOLTAGE_FILTER_SAMPLES 30


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
    /**< Not configured */
    POS_RET_NOT_CONFIGURED = 6,
    /**< Error registering for event */
    POS_RET_EVENT_REG_ERROR = 7,
    /** Motion monitoring not enabled */
    POS_RET_MOTION_NOT_ENABLED = 8
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
    POSLIB_MODE_DA_TAG = 5,
    POSLIB_MODE_ENDS = 5
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
    POSLIB_BLE_OFF = 0,             // OFF
    POSLIB_BLE_ON = 1,              // always ON
    POSLIB_BLE_ON_WHEN_OFFLINE = 2  //activated when outside coverage
} poslib_ble_mode_e;

/**
 * @brief  defines the device's operational class.
 */
typedef enum
{
    POSLIB_CLASS_NODE_SPECIFIC = 0xF8,
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

#define POSLIB_FLAG_EVENT_SUBSCRIBERS_MAX 8

/**
 * @brief Defines the events triggered by PosLib.
 *  
 */
typedef enum
{
    /**< No event */
    POSLIB_FLAG_EVENT_NONE = 0,
    /**< Position measurement update started */
    POSLIB_FLAG_EVENT_UPDATE_START = 1,
    /**< Position measurement update ended */
    POSLIB_FLAG_EVENT_UPDATE_END = 2,
    /**< BLE beacon transmission started */
    POSLIB_FLAG_EVENT_BLE_START = 4,
    /**< BLE beacon transmission stopped */
    POSLIB_FLAG_EVENT_BLE_STOP = 8,
    /**< PosLib requested WM to enter offline */
    POSLIB_FLAG_EVENT_OFLINE = 16,
    /**< PosLib WM is in online, NRLS update starts */
    POSLIB_FLAG_EVENT_ONLINE = 32,
    /**< PosLib configuration changed */
    POSLIB_FLAG_EVENT_CONFIG_CHANGE = 64,
    /**< Led status on */
    POSLIB_FLAG_EVENT_LED_ON = 128,
    /**< Led status off */
    POSLIB_FLAG_EVENT_LED_OFF = 256,
    /**< Global flag for subscribing to all events */
    POSLIB_FLAG_EVENT_ALL = 511 /* ! Update when adding a new event */
} poslib_events_e;

/**
 * @brief  defines status returned in PosLib_status().
 */
typedef enum
{
    POSLIB_STOPPED = 0,
    POSLIB_IDLE = 1,
    POSLIB_UPDATE_START = 2,
} poslib_status_e;

/**
 * @brief  defines the mini-beacon payload records types
 */
typedef enum
{
    POSLIB_MBCN_INVALID_TYPE = 0, 
    // Wirepas defined - required types
    POSLIB_MBCN_TX_INTERVAL = 1,  
    POSLIB_MBCN_FEATURES = 2,
    // Wirepas defined - optional types
    POSLIB_MBCN_AREA_ID = 3,
    POSLIB_MBCN_FLOOR_ID = 4,
    POSLIB_MBCN_X = 5,
    POSLIB_MBCN_Y = 6,
    POSLIB_MBCN_Z = 7,
    // 8-15 reserved
    // Application specific types (use only when standard records not available)
    POSLIB_MBCN_CUSTOM_1 = 16,
    POSLIB_MBCN_CUSTOM_2 = 17,
    POSLIB_MBCN_CUSTOM_3 = 18,
    POSLIB_MBCN_CUSTOM_4 = 19,
    // 20-31 reserved.  !!! type cannot be > 31
    POSLIB_MBCN_MAX_TYPE = 31,
} poslib_mbcn_record_types_e;

/**< Maximum number of bytes included in one record */
#define POSLIB_MBCN_RECORD_MAX_SIZE 16

/**< Maximum optional records in mini-beacon payload*/
#define POSLIB_MBCN_RECORDS 8

#define POSLIB_MBCN_SRC_EP 238
#define POSLIB_MBCN_DEST_EP 238

typedef enum
{
    POSLIB_MBCN_TX_INTERVAL_DEFAULT = 1000,
    POSLIB_MBCN_TX_INTERVAL_500 = 500,
    POSLIB_MBCN_TX_INTERVAL_250 = 250,
} poslib_mbcn_tx_interval_e;

/**
 * @brief  defines mini-beacon record.
 */
typedef struct
{
    uint8_t type;   // according to poslib_mbcn_record_types_e
    uint8_t length;  // shall be <= POSLIB_MBCN_RECORD_MAX_SIZE, invalid record when 0
    uint8_t value[POSLIB_MBCN_RECORD_MAX_SIZE];
} poslib_mbcn_record_t;

/**
 * @brief  defines mini-beacon configuration.
 */
typedef struct
{
    bool enabled;
    uint16_t tx_interval_ms;
    poslib_mbcn_record_t records[POSLIB_MBCN_RECORDS];
} poslib_mbcn_config_t;


/**
 * @brief  defines mini-beacon data payload.
 */
typedef struct
{
    uint16_t seq;
    uint16_t tx_interval_ms;
    uint16_t features;
    poslib_mbcn_record_t records[POSLIB_MBCN_RECORDS];
} poslib_mbcn_data_t;


/**
 * @brief  defines BLE mode mode configuration.
 */
typedef struct
{
    int8_t tx_power;
    app_lib_beacon_tx_channels_mask_e channels;
    uint16_t tx_interval_ms;
} poslib_ble_mode_config_t;

/**
 * @brief position library ble settings.
 */
typedef struct
{
    poslib_ble_type_e type;
    poslib_ble_mode_e mode;
    poslib_ble_mode_config_t eddystone;
    poslib_ble_mode_config_t ibeacon;
    uint16_t activation_delay_s;  //BLE beacons activation delay when outside coverage
} ble_beacon_settings_t;

/**
 * @brief position library motion settings.
 */
typedef struct
{    
    bool enabled;
    uint16_t threshold_mg;
    uint16_t duration_ms;
} poslib_motion_mon_settings_t;

/**
 * @brief position library DA settings.
 */
typedef struct
{    
    bool routing_enabled;
    bool follow_network;
} poslib_da_settings_t;

/**
 * @brief position library settings.
 */
typedef struct
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
    uint32_t update_period_offline_s;
    /* defined using PosLib_setConfig. Configuration of ble */
    ble_beacon_settings_t ble;
    /* Motion settings*/
    poslib_motion_mon_settings_t motion;
    /*Mini-beacon settings*/
    poslib_mbcn_config_t mbcn;
    /* DA settings */
    poslib_da_settings_t da;
} poslib_settings_t;

/**
 * @brief positioning library auxiliary settings
 */
typedef struct
{
    /* duration for LED to be turned on */
    uint16_t led_on_duration_s;
    /* LED control command sequence */
    uint8_t led_cmd_seq;
} poslib_aux_settings_t;

/** Helper structure to move data from PosLib events*/
typedef struct
{
    uint16_t                       event_id;
    app_lib_time_timestamp_hp_t    time_hp;
    uint32_t                       nrls_sleep_time_sec;
} POSLIB_FLAG_EVENT_info_t;

/**
 * @brief   Function used with POSLIB_FLAG_EVENT_setCb
 * @param   struct for poslib_events_listen_info_f
 *          struct for poslib_events_info_t data.
 */
typedef void (*poslib_events_listen_info_f)(POSLIB_FLAG_EVENT_info_t * msg);

/**
 * @brief   This callback is used to receive Poslib generated events.
 * @param   Callback function type of poslib_events_listen_info_f.
 *
 */
typedef void (*poslib_events_callback_f) (poslib_events_listen_info_f callback);

/**
 * @brief   Sets PosLib configuration.
 * @param   poslib_settings_t type pointer
 * @return  See \ref poslib_ret_e
 */
poslib_ret_e PosLib_setConfig(poslib_settings_t * settings);

/**
 * @brief   Gets PosLib configuration.
 * @param   poslib_settings_t pointer where settings will be copied
 * @return  See \ref poslib_ret_e
 */
poslib_ret_e PosLib_getConfig(poslib_settings_t * settings);

/**
 * @brief   Start the positioning updates according to provided configuration.
 *          Calls PosLib initialization function before start.
 *          
 * @return  See \ref poslib_ret_e
 */
poslib_ret_e PosLib_startPeriodic(void);

/**
 * @brief   Stops the positioning updates.
 * @return  See \ref poslib_ret_e
 */
poslib_ret_e PosLib_stopPeriodic(void);

/**
 * @brief   Triggers one position update. If PosLib is started the scheduled
 *          operation will continue after the oneshot update. 
 *          If in stop state then after the oneshot update is completed it will return in stop state.
 * @note    If called during NRLS sleep, sleep is stopped which triggers one
 *          position update. NRLS period continues with the respective schedule.
 * @return  See \ref poslib_ret_e
 */
poslib_ret_e PosLib_startOneshot(void);

/**
 * @brief   Used to indicate that the tag is static or dynamic. The update
 *          rate used will be adjusted according to the state.  
 * @param   mode type of poslib_motion_mode_e
 * @return  See \ref poslib_ret_e
 */
poslib_ret_e PosLib_motion(poslib_motion_mode_e mode);

/**
 * @brief   Returns the current status of PosLib.
 * @return  See \ref poslib_status_e
 */
poslib_status_e PosLib_status(void);

/**
 * @brief   Register an event subscriber
 * @param   event Events of interest (type of poslib_events_e)
 * @param   cb Callback to be called (type poslib_events_listen_info_f)
 * @param   id Returned subscriber ID
 * @return  POS_RET_OK is succesful / POS_RET_EVENT_ERROR if failed
 */
poslib_ret_e PosLib_eventRegister(poslib_events_e event,
                            poslib_events_listen_info_f cb, 
                            uint8_t * id);

/**
 * @brief   De-register the event subscriber with the provided id
 * @param   id subscriber ID
 */
void PosLib_eventDeregister(uint8_t id);

/**
 * @brief   Decodes the mini-beacon payload
 * @param[in]   buf pointer to MBCN payload
 * @param[in]   length length of the MBCN payload
 * @param[out]  mbcn decoded minibeacon content   
 * @return  true: decoding sucessful, false: decode failed
 */
bool PosLib_decodeMbcn(uint8_t * buf, uint8_t length, poslib_mbcn_data_t * mbcn);

/**
 * @brief   Send data. The packet to send is represented as a
 *          @ref app_lib_data_to_send_t struct. In node has Advertiser role
 *          and dest_address is APP_ADDR_ANYSINK then the function will replace
 *          the destination with the address of the best found LL router supporting DA
 * @param   data
 *          Data to send
 * @param   sent_cb
 *          Callback function to be called when a packet has gone through local
 *          processing and has finally been sent or discarded. If NULL is
 *          passed, the callback is disabled. This callback replaces
 *          tracking_id and APP_LIB_DATA_SEND_FLAG_TRACK flag of data
 *          structure.
 * @note    tracking_id is managed by the library. When function return the
 *          data structure is modified to reflect id used to send the packet.
 * @return  Result code, @ref APP_LIB_DATA_SEND_RES_SUCCESS means that data
 *          was accepted for sending. See @ref app_res_e for
 *          other result codes.
 */
app_lib_data_send_res_e PosLib_sendData(app_lib_data_to_send_t * data,
                                            app_lib_data_data_sent_cb_f sent_cb);
#endif