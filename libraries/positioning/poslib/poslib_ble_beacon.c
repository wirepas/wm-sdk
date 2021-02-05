/**
 * @file poslib_ble_beacon.c
 * @brief Handles the Beacon configuration.
 * @copyright Wirepas Ltd 2020
 */

#include <string.h>
#define DEBUG_LOG_MODULE_NAME "POSLIB_BLE"
#ifdef DEBUG_POSLIB_LOG_MAX_LEVEL
#define DEBUG_LOG_MAX_LEVEL DEBUG_POSLIB_LOG_MAX_LEVEL
#else
#define DEBUG_LOG_MAX_LEVEL LVL_NOLOG
#endif
#include "debug_log.h"
#include "app_scheduler.h"
#include "poslib.h"
#include "poslib_control.h"
#include "poslib_measurement.h"
#include "poslib_ble_beacon.h"
#include "shared_beacon.h"
#include "api.h"

#define MAX_BLE_BEACON_TX_SIZE     38
#define BEACON_ADV_INTERVAL      1000
#define BEACON_TX_POWER          0x04
#define USE_EDYSTONE_UID  //if not defined Edystone URL will be set

/** 0 indicates that becons are found in PosLib scan */
#define POSLIB_MEAS_SCAN_BEACONS_FOUND       0

#define TIME_SEC_TO_MSEC(tm) ((tm) * 1000U)

/** Time interval in second when number of beacons is checked for ble offline */
#define CHECK_BLEON_OFFLINE_BEACONS_MS        TIME_SEC_TO_MSEC(10)

/** Offline timeout status*/
static poslib_bleoffline_status_e m_offline_status = POSLIB_BLE_OFFLINE_IDLE;

/**
 * @brief Beacon's states
 */
typedef enum
{
    POSLIB_BEACON_STATE_OFF = 0,
    POSLIB_BEACON_STATE_ON = 1,
    POSLIB_BEACON_STATE_NOT_IMPLEMENTED = 10,
} beacon_state_e;

/**
 * @brief Common beacon header
 */
typedef struct
{
    uint8_t ad_type;
    uint8_t nid[6];
    uint8_t ad_flags_len;
    uint8_t ad_flags_type;
    uint8_t ad_flags_data;
} beacon_common_frame_t;

/**
 * @brief Eddystone beacon URL header
 */
typedef struct
{
    uint8_t uuid_len;
    uint8_t uuid_type;
    uint8_t uuid[2];
    uint8_t service_len;
    uint8_t service_type;
    uint8_t service_uuid[2];
    uint8_t service_frame_type;
    uint8_t service_frame_oid;
    uint8_t url_prefix;
    uint8_t url_advert[7];
    uint8_t url_suffix;
} eddystone_url_frame_t;

/**
 * @brief Eddystone beacon UID header
 */
typedef struct
{
    uint8_t uuid_len;
    uint8_t uuid_type;
    uint8_t uuid[2];
    uint8_t service_len;
    uint8_t service_type;
    uint8_t service_uuid[2];
    uint8_t service_frame_type;
    uint8_t tx_power;    // ranging data in the spec
    uint8_t nid[10];    // 10 byte name space
    uint8_t bid[6];    // 6 byte instance id
    uint8_t rfu[2];
} eddystone_uid_frame_t;

/**
 * @brief IBeacon header
 */
typedef struct
{
    uint8_t service_len;
    uint8_t manufacturer_spec;
    uint8_t company_id[2];
    uint8_t subtype;
    uint8_t subtype_len;
    uint8_t uuid[16];
    uint8_t major[2];
    uint8_t minor[2];
    uint8_t tx_power;
} ibeacon_frame_t;

const uint8_t AD_TYPE = 0x42;         // Non-connectable Beacon
static uint8_t m_addresses[6];  // Storage for Node Id and Network address
static beacon_state_e m_beacon_state = POSLIB_BEACON_STATE_OFF; // Beacon states are on or off
static poslib_ble_type_e m_beacon_type;
static poslib_ble_mode_e m_beacon_mode;
static uint8_t m_beacontx_eddybeacon_index;
static uint8_t m_beacontx_ibeacon_index;
static bool m_enabled_eddystone = 0;
static bool m_enabled_ibeacon = 0;

/**
 * @brief      Set a common beacon frame
 * @param[out] pframe
 *             a common part of the frame
 */
static void set_common_dataframe(uint8_t * pframe)
{
    beacon_common_frame_t common_frame =
    {
        .ad_type = AD_TYPE,
        .nid[0] = m_addresses[0],
        .nid[1] = m_addresses[1],
        .nid[2] = m_addresses[2],
        .nid[3] = m_addresses[3],
        .nid[4] = m_addresses[4],
        .nid[5] = m_addresses[5],
        .ad_flags_len = 0x02,
        .ad_flags_type = 0x01,
        .ad_flags_data = 0x04,      /* Bluetooth LE Beacon only */
    };
    memcpy(pframe, &common_frame, sizeof(beacon_common_frame_t));
}

#ifndef USE_EDYSTONE_UID
/**
 * @brief   Set the Eddystone URL beacon frame
 * @param   pframe
 *              The whole Eddystone frame
 */
static void set_eddystone_url_dataframe(uint8_t * pframe)
{
    uint8_t i = 0;
    uint8_t buffer[sizeof(beacon_common_frame_t)+1] = {'\0'};

    set_common_dataframe(buffer);
    memcpy(pframe, &buffer, sizeof(beacon_common_frame_t));
    i += sizeof(beacon_common_frame_t);

    eddystone_url_frame_t eddystone_frame =
    {
        .uuid_len = 0x03,
        .uuid_type = 0x03,
        .uuid[0] = 0xaa,            /* 16-bit Eddystone UUID */
        .uuid[1] = 0xfe,
        .service_len = 0x0e,        /* Service data length */
        .service_type = 0x16,       /* Service data type */
        .service_uuid[0] = 0xaa,    /* 16-bit Eddystone UUID */
        .service_uuid[1] = 0xfe,
        .service_frame_type = 0x10, /* URL */
        .service_frame_oid = 0xf7,
        .url_prefix = 0x00,
        .url_advert = {'w','i','r','e','p','a','s'},
        .url_suffix = 0x07          /* URL scheme suffix .com/ */
    };
    memcpy(pframe + i, &eddystone_frame, sizeof(eddystone_url_frame_t));
}

#else
/**
 * @brief   Set the Eddystone UID beacon frame
 * @paramp  frame
 *          The whole Eddystone frame
 */
static void set_eddystone_uid_dataframe(uint8_t * pframe)
{
    app_lib_settings_net_addr_t network_address;
    app_addr_t node_address;
    uint8_t i = 0;
    uint8_t buffer[sizeof(beacon_common_frame_t)+1] = {'\0'};


    eddystone_uid_frame_t eddystone_frame =
    {
        .uuid_len = 0x03,
        .uuid_type = 0x03,
        .uuid[0] = 0xaa,            /* 16-bit Eddystone UUID */
        .uuid[1] = 0xfe,
        .service_len = 0x17,        /* Service data length */
        .service_type = 0x16,       /* Service data type */
        .service_uuid[0] = 0xaa,    /* 16-bit Eddystone UUID */
        .service_uuid[1] = 0xfe,
        .service_frame_type = 0x00, /* UID */
        .tx_power = BEACON_TX_POWER, // set to TX power
        .nid = {'w','i','r','e','p','a','s', 0x00, 0x00, 0x00},
        .bid = {0x00},
        .rfu = {0x00}
    };

    lib_settings->getNetworkAddress(&network_address);
    lib_settings->getNodeAddress(&node_address);

    // Set network address as part of the NID
    // the values are set as big-endian according to the Eddystone specification
    eddystone_frame.nid[7] = (uint8_t)((network_address >>  16) & 0xff);
    eddystone_frame.nid[8] = (uint8_t)((network_address >>  8) & 0xff);
    eddystone_frame.nid[9] = (uint8_t)((network_address) & 0xff);

    // .bid[0], .bid[1] are set to 0x00
    eddystone_frame.bid[2] = (uint8_t)((node_address >>  24) & 0xff);
    eddystone_frame.bid[3] = (uint8_t)((node_address >>  16) & 0xff);
    eddystone_frame.bid[4] = (uint8_t)((node_address >>  8) & 0xff);
    eddystone_frame.bid[5] = (uint8_t)((node_address) & 0xff);

    set_common_dataframe(buffer);
    memcpy(pframe, &buffer, sizeof(beacon_common_frame_t));
    i += sizeof(beacon_common_frame_t);
    memcpy(pframe + i, &eddystone_frame, sizeof(eddystone_uid_frame_t));
}
#endif

/**
 * @brief   Set IBeacon frame
 * @param   pframe
 *          The whole IBeacon frame
 */
static void set_ibeacon_dataframe(uint8_t * pframe)
{
    uint8_t i = 0;
    uint8_t buffer[sizeof(beacon_common_frame_t)+1] = {'\0'};

    app_lib_settings_net_addr_t network_address;

    lib_settings->getNetworkAddress(&network_address);

    set_common_dataframe(buffer);
    memcpy(pframe, &buffer, sizeof(beacon_common_frame_t));
    i += sizeof(beacon_common_frame_t);

    ibeacon_frame_t ibeacon_frame =
    {
        .service_len = 0x1a,
        .manufacturer_spec = 0xff,
        .company_id[0] = 0x4C,      // Used 'Unknown' name
        .company_id[1] = 0x00,
        .subtype = 0x02,            // Beacons
        .subtype_len = 0x15,
        .uuid = {'w','i','r','e','p','a','s',' ', 'm','e','s', 'h',
        0x00,0x00,0x00,0x00},
        .major[0] = 0x00,           // Not used for now, user's definable
        .major[1] = 0x00,
        .minor[0] = 0x00,           // Not used for now, user's definable
        .minor[1] = 0x00,
        .tx_power = BEACON_TX_POWER
    };

    // Set network address as part of the UUID
    ibeacon_frame.uuid[13] = (uint8_t)(network_address >>  16) & 0xff;
    ibeacon_frame.uuid[14] = (uint8_t)(network_address >>  8) & 0xff;
    ibeacon_frame.uuid[15] = (uint8_t)(network_address) & 0xff;

    memcpy(pframe + i, &ibeacon_frame, sizeof(ibeacon_frame_t));
    i += sizeof(ibeacon_frame);
}

/**
 * @brief   Modify random Bluetooth LE beacon address with device's addresses
 */
static void set_random_address()
{
    app_addr_t node_address;

    lib_settings->getNodeAddress(&node_address);

// Address sent in the beacon is defined as "static device address"
// as defined by the BLE standard

    m_addresses[0] = (uint8_t)(node_address) & 0xff;
    m_addresses[1] = (uint8_t)(node_address >>  8) & 0xff;
    m_addresses[2] = (uint8_t)(node_address >>  16) & 0xff;
    m_addresses[3] = (uint8_t)(node_address >>  24) & 0xff;
    m_addresses[4] = 0x00;
    m_addresses[5] = 0xC0;

    LOG(LVL_DEBUG, "\"Ble Beacon updated for node\":0x%x", node_address);
}

/**
 * @brief   Stops beacon based global parameters.
 */
static void stopBeacon(void)
{
    shared_beacon_res_e sh_res = SHARED_BEACON_RES_OK;
    poslib_events_info_t msg;

    if (m_enabled_eddystone)
    {
        sh_res |= Shared_Beacon_stopBeacon(m_beacontx_eddybeacon_index);
        m_beacontx_eddybeacon_index = 0;
        m_enabled_eddystone = false;
    }

    if (m_enabled_ibeacon)
    {
        sh_res |= Shared_Beacon_stopBeacon(m_beacontx_ibeacon_index);
        m_beacontx_ibeacon_index = 0;
        m_enabled_ibeacon = false;
    }

    if (sh_res == SHARED_BEACON_RES_OK)
    {
        LOG(LVL_DEBUG, "\"Beacon is disabled\"");
        m_beacon_state = POSLIB_BEACON_STATE_OFF;
        msg.event_id = POSLIB_BLE_STOP_EVENT;
        PosLibCtrl_generateEvent(&msg);
    }
    else
    {
        LOG(LVL_ERROR, "\"Beacon disabling failed\"");
    }
}

bool PosLibBle_checkActive(void)
{
    if (m_enabled_eddystone || m_enabled_ibeacon)
    {
        return true;
    }
    else
    {
        return false;
    }
}

/**
 * @brief   Make the Beacon
 * @param   index
 *          index
 * @param   beacon
 *          Beacon's type
 * @param   length
 *          Beacon's frame length
 */
static void makeBeacon(poslib_ble_type_e beacon_type,
                       const uint8_t * beacon,
                       size_t length)
{
    shared_beacon_res_e sh_res = SHARED_BEACON_INVALID_PARAM;
    poslib_events_info_t msg;
    uint8_t shared_beacontx_index;
    poslib_settings_t * poslib_settings = Poslib_get_settings();
    poslib_ble_mode_config_t * ble_config;

    if (beacon_type == POSLIB_EDDYSTONE)
    {
        ble_config = &poslib_settings->poslib_ble_settings.ble_eddystone;

    }
    else if (beacon_type == POSLIB_IBEACON)
    {
        ble_config = &poslib_settings->poslib_ble_settings.ble_ibeacon;
    }

    if (beacon_type == POSLIB_EDDYSTONE || beacon_type == POSLIB_IBEACON)
    {
        /** shared_beacontx library is used to becon tx activation */
        sh_res = Shared_Beacon_startBeacon(TIME_SEC_TO_MSEC(ble_config->ble_tx_interval_s),
                                           &ble_config->ble_tx_power,
                                           ble_config->ble_channels,
                                           beacon,
                                           length,
                                           &shared_beacontx_index);

        if (sh_res == SHARED_BEACON_RES_OK)
        {
            if (beacon_type == POSLIB_EDDYSTONE)
            {
                m_beacontx_eddybeacon_index = shared_beacontx_index;
                m_enabled_eddystone = true;
            }
            else
            {
                m_beacontx_ibeacon_index = shared_beacontx_index;
                m_enabled_ibeacon = true;
            }

            LOG(LVL_DEBUG, "\"Beacon is enabled\"");
            m_beacon_state = POSLIB_BEACON_STATE_ON;
            msg.event_id = POSLIB_BLE_START_EVENT;
            PosLibCtrl_generateEvent(&msg);

        }
        else
        {
            // Maybe stack is not running. Try later again. Do not change state
            LOG(LVL_WARNING, "\"Beacon enabling failed\"");
        }
    }
}

/**
 * @brief   Handle the right Beacon(s) frame and set it
 * @param   beacon
 *          Beacon's type
 */
static void configure_beacon(poslib_ble_type_e selection)
{
    uint8_t buffer[MAX_BLE_BEACON_TX_SIZE] = {0};

    if ((selection == POSLIB_BEACON_ALL) || (selection == POSLIB_EDDYSTONE))
    {

#ifdef USE_EDYSTONE_UID
        set_eddystone_uid_dataframe(buffer);
#else
        set_eddystone_url_dataframe(buffer);
#endif
        makeBeacon(POSLIB_EDDYSTONE, buffer, sizeof(buffer));
    }

    if ((selection == POSLIB_BEACON_ALL) || (selection == POSLIB_IBEACON))
    {
        set_ibeacon_dataframe(buffer);
        makeBeacon(POSLIB_IBEACON, buffer, sizeof(buffer));
    }
    LOG(LVL_DEBUG, "\"beacon_type\":0x%x", selection);
}

void PosLibBle_init(ble_beacon_settings_t * settings)
{
    m_beacon_state = POSLIB_BEACON_STATE_OFF;
    m_beacon_type = settings->ble_type;
    m_beacon_mode = settings->ble_mode;

    set_random_address();
}

poslib_ret_e PosLibBle_set(poslib_ble_mode_e mode)
{
    poslib_ret_e res = POS_RET_INVALID_PARAM;

    if (lib_state->getStackState() != APP_LIB_STATE_STARTED ||
       (m_beacon_state == POSLIB_BEACON_STATE_ON && mode == POSLIB_BLE_START))
    {
        return POS_RET_INVALID_STATE;
    }

    m_beacon_mode = mode;

    if (m_beacon_mode == POSLIB_BLE_START)
    {
        configure_beacon(m_beacon_type);
        res = POS_RET_OK;
    }
    else if (m_beacon_mode == POSLIB_BLE_STOP)
    {
        stopBeacon();
        res = POS_RET_OK;
    }

    LOG(LVL_DEBUG, "\"beacon_state\":%d", m_beacon_state);
    return res;
}

/**
 * @brief   Check if there is router to sink available.
 *          Starts ble if not route. Stops ble if activated and route is found.
 */
static void start_ble_stackoffline(void)
{
    app_res_e rval = 0;
    size_t count = 0;
    /** Read PosLib settings */
    poslib_settings_t * pos_settings = Poslib_get_settings();

    rval = lib_state->getRouteCount(&count);

    if (rval == APP_RES_OK &&
        lib_sleep->getSleepState() == APP_LIB_SLEEP_STOPPED &&
        lib_state->getStackState() == APP_LIB_STATE_STARTED &&
        pos_settings->poslib_ble_settings.update_period_bleon_offline_s != 0)
    {
        if (count == 0)
        {
            if (m_offline_status == POSLIB_BLE_OFFLINE_DETECTED)
            {
                LOG(LVL_DEBUG, "no route - ble is started");
                PosLibBle_set(POSLIB_BLE_START);
                m_offline_status = POSLIB_BLE_OFFLINE_ACTIVATED;
            }
            else if (m_offline_status == POSLIB_BLE_OFFLINE_ACTIVATED)
            {
                LOG(LVL_DEBUG, "no route - ble offline is activated");
            }
        }
        else if (count != 0)
        {
            if (m_offline_status == POSLIB_BLE_OFFLINE_DETECTED)
            {
                LOG(LVL_DEBUG, "route found - ble is not started");
                m_offline_status = POSLIB_BLE_OFFLINE_IDLE;
            }
            else if (m_offline_status == POSLIB_BLE_OFFLINE_ACTIVATED)
            {
                LOG(LVL_DEBUG, "route found - ble is disabled");
                PosLibBle_set(POSLIB_BLE_STOP);
                m_offline_status = POSLIB_BLE_OFFLINE_IDLE;
            }
        }
    }
}

/**
 * @brief   Check if no beacons are found.
 *          Starts task for route check and possible ble activation after that.
 *          Stops activated ble tx if ble tx is activated and beacons are found.
 * @param   pos_settings
 *          PosLib setting
 */
static void activate_bleoffline(poslib_settings_t * pos_settings)
{
    uint32_t time_no_beacons_s = PosLibMeas_getTimeNoBeacons();

    if (time_no_beacons_s != POSLIB_MEAS_SCAN_BEACONS_FOUND)
    {
        LOG(LVL_DEBUG, "time_diff_when_no_beacons: %u",
        (lib_time->getTimestampS() - time_no_beacons_s));
    }

    if (time_no_beacons_s > pos_settings->poslib_ble_settings.update_period_bleon_offline_s &&
        m_beacon_state == POSLIB_BEACON_STATE_OFF)
    {
        LOG(LVL_DEBUG, "NO neigh beacons - start_ble_stackoffline");
        start_ble_stackoffline();
        m_offline_status = POSLIB_BLE_OFFLINE_DETECTED;
    }
    else if (time_no_beacons_s == POSLIB_MEAS_SCAN_BEACONS_FOUND &&
             m_offline_status != POSLIB_BLE_OFFLINE_IDLE &&
             m_beacon_state == POSLIB_BEACON_STATE_ON)
    {
        LOG(LVL_DEBUG, "beacons found");
        start_ble_stackoffline();
    }
}

uint32_t PosLibBle_check_offline(void)
{
    poslib_settings_t * pos_settings = Poslib_get_settings();

    /** Needs to have feature active > 0, stack not sleeping and stack started */
    if (pos_settings->poslib_ble_settings.update_period_bleon_offline_s != 0 &&
        lib_sleep->getSleepState() == APP_LIB_SLEEP_STOPPED &&
        lib_state->getStackState() == APP_LIB_STATE_STARTED)
    {
        activate_bleoffline(pos_settings);
    }

    if (lib_sleep->getSleepState() == APP_LIB_SLEEP_STARTED)
    {
        return APP_SCHEDULER_STOP_TASK;
    }

    if (pos_settings->poslib_ble_settings.update_period_bleon_offline_s != 0)
    {
        return CHECK_BLEON_OFFLINE_BEACONS_MS;
    }
    else
    {
        return APP_SCHEDULER_STOP_TASK;
    }
}
