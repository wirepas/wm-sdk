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
#include "poslib.h"
#include "poslib_control.h"
#include "api.h"

#define MAX_BLE_BEACON_TX_SIZE     38
#define BEACON_ADV_INTERVAL      1000
#define BEACON_TX_POWER          0x04
#define USE_EDYSTONE_UID  //if not defined Edystone URL will be set

/**
 * @brief Beacon's states
 */
typedef enum
{
    POSLIB_BEACON_STATE_OFF = 0,
    POSLIB_BEACON_STATE_ON = 1,
    POSLIB_BEACON_STATE_NOT_IMPLEMENTED = 10,
} beacon_state_e;

#define BEACON_SET_ON               1
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
static beacon_state_e m_beacon_state; // Beacon states are on or off
static ble_beacon_settings_t m_ble_beacon_settings;

static void beacon_enable();

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
 *             The whole Eddystone frame
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
 *              The whole IBeacon frame
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

}

/**
 * @brief   Make the Beacon
 * @param   index
 *             index
 * @param   beacon
 *             Beacon's type
 * @param   length
 *             Beacon's frame length
 * @param   enable
 *             start used or not the Beacon(s)
*/
static void makeBeacon(uint8_t index, const uint8_t * beacon, size_t length,
                       bool enable)
{
    lib_beacon_tx->setBeaconInterval(
        m_ble_beacon_settings.ble_eddystone.ble_tx_interval_s * 1000);
    lib_beacon_tx->setBeaconPower(index,
        &m_ble_beacon_settings.ble_eddystone.ble_tx_power);
    lib_beacon_tx->setBeaconChannels(index,
        m_ble_beacon_settings.ble_eddystone.ble_channels);
    lib_beacon_tx->setBeaconContents(index, beacon, length);
    beacon_enable();
}

/**
 * @brief   Handle the right Beacon(s) frame and set it
 * @param   beacon
 *             Beacon's type
 * @param   enable
 *             start or not the Beacon(s) immediately
 */
static void configure_beacon(uint8_t selection, bool enable)
{
    uint8_t buffer[MAX_BLE_BEACON_TX_SIZE] = {0};

    if ((selection == POSLIB_BEACON_ALL) || (selection == POSLIB_EDDYSTONE))
    {

#ifdef USE_EDYSTONE_UID
        set_eddystone_uid_dataframe(buffer);
#else
        set_eddystone_url_dataframe(buffer);
#endif
        makeBeacon(0, buffer, sizeof(buffer), enable);
    }

    if ((selection == POSLIB_BEACON_ALL) || (selection == POSLIB_IBEACON))
    {
        set_ibeacon_dataframe(buffer);
        makeBeacon(1, buffer, sizeof(buffer), enable);
    }
}

/**
 * @brief   Starts sending Bluetooth Low Energy Beacons
 */
static void beacon_enable()
{
    app_res_e rval = 0;
    poslib_events_info_t msg;

    rval = lib_beacon_tx->enableBeacons(true);
    if (rval == APP_RES_OK)
    {
        m_beacon_state = POSLIB_BEACON_STATE_ON;
        msg.event_id = POSLIB_BLE_START_EVENT;
        PosLibCtrl_generateEvent(&msg);
    }
    else
    {
        // Maybe stack is not running. Try later again. Do not change state
    }
}

void PosLibBle_disable()
{
    app_res_e rval = 0;
    poslib_events_info_t msg;

    rval = lib_beacon_tx->enableBeacons(false);

    if (rval == APP_RES_OK)
    {
        m_beacon_state = POSLIB_BEACON_STATE_OFF;
        msg.event_id = POSLIB_BLE_STOP_EVENT;
        PosLibCtrl_generateEvent(&msg);
    }
    else
    {
        // Maybe stack is not running. Try later again. Do not change state
    }
}

void PosLibBle_init(ble_beacon_settings_t * settings)
{
    PosLibBle_disable();
    m_beacon_state = POSLIB_BEACON_STATE_OFF;
    m_ble_beacon_settings.ble_type = settings->ble_type;
    m_ble_beacon_settings.ble_mode = settings->ble_mode;

    set_random_address();
    // set_interval_power_channel_settings();

    if (settings->ble_type == POSLIB_EDDYSTONE ||
        settings->ble_type == POSLIB_BEACON_ALL)
    {
        m_ble_beacon_settings.ble_eddystone.ble_tx_interval_s =
            settings->ble_eddystone.ble_tx_interval_s;
        m_ble_beacon_settings.ble_eddystone.ble_tx_power =
            settings->ble_eddystone.ble_tx_power;
        m_ble_beacon_settings.ble_eddystone.ble_channels =
            settings->ble_eddystone.ble_channels;
    }

    if (settings->ble_type == POSLIB_IBEACON ||
        settings->ble_type == POSLIB_BEACON_ALL)
    {
        m_ble_beacon_settings.ble_ibeacon.ble_tx_interval_s =
            settings->ble_ibeacon.ble_tx_interval_s;
        m_ble_beacon_settings.ble_ibeacon.ble_tx_power =
            settings->ble_ibeacon.ble_tx_power;
        m_ble_beacon_settings.ble_ibeacon.ble_channels =
            settings->ble_ibeacon.ble_channels;
    }
}

void PosLibBle_setMode(poslib_ble_mode_e mode)
{
    m_ble_beacon_settings.ble_mode = mode;

}

void PosLibBle_set()
{
    if (lib_state->getStackState() != APP_LIB_STATE_STARTED)
    {
        return;
    }

    if (m_ble_beacon_settings.ble_mode == BEACON_SET_ON)
    {
        configure_beacon(m_ble_beacon_settings.ble_type, true);
        m_beacon_state = POSLIB_BEACON_STATE_ON;
    }
    else
    {
        configure_beacon(m_ble_beacon_settings.ble_type, false);
        m_beacon_state = POSLIB_BEACON_STATE_OFF;
        PosLibBle_disable();
    }

}

void PosLibBle_enableOffline()
{
    if (m_beacon_state == POSLIB_BEACON_STATE_OFF &&
        lib_state->getStackState() == APP_LIB_STATE_STARTED)
    {
        configure_beacon(m_ble_beacon_settings.ble_type, true);
        m_beacon_state = POSLIB_BEACON_STATE_ON;
        beacon_enable();
    }
}
