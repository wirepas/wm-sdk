/**
    @file ble_beacon.c
    @brief Handles the Beacon configuration.
    @copyright Wirepas Oy 2019
*/
#include <string.h> // memcpy

#include "app_settings.h"
#include "ble_beacon.h"
#include "scheduler.h"
#include "api.h"

#define MAX_BLE_BEACON_TX_SIZE     38
#define BEACON_ADV_INTERVAL      1000
#define BEACON_TX_POWER          0x04
#define USE_EDYSTONE_UID  //if not defined Edystone URL will be set


/**
    @brief Bluetooth LE Beacon interval, power and channel settings.
*/
typedef struct
{
    uint32_t interval_ms;
    int8_t tx_power;
    app_lib_beacon_tx_channels_mask_e adv_channels;
} ble_beacon_settings_t;

/**
    @brief Beacon's states
*/
typedef enum
{
    BEACON_STATE_OFF = 0,
    BEACON_STATE_ON,
    BEACON_STATE_NOT_IMPLEMENTED = 10,
} beacon_state_e;

/**
    @brief Common beacon header
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
    @brief Eddystone beacon URL header
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
    @brief Eddystone beacon UID header
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
    @brief IBeacon header
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
static bool m_configuration;          // Beacon configured
static bool m_monitoring;             // Offline monitoring
static bool m_offline_timer_on;       // Beacon offline timer started
static uint32_t m_offlineTimerStartTime = 0; // Offline counter

/**
    @brief      Set a common beacon frame
    @param[out] pframe
                a common part of the frame
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
    @brief      Set the Eddystone URL beacon frame
    @param[out] pframe
                The whole Eddystone frame
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
    @brief      Set the Eddystone UID beacon frame
    @param[out] pframe
                The whole Eddystone frame
*/
static void set_eddystone_uid_dataframe(uint8_t * pframe)
{
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

    positioning_settings_t * app_settings = Pos_get_settings();

    // Set network address as part of the NID
    // the values are set as big-endian according to the Eddystone specification
    eddystone_frame.nid[7] = (uint8_t)((app_settings->network_address >>  16) & 0xff);
    eddystone_frame.nid[8] = (uint8_t)((app_settings->network_address >>  8) & 0xff);
    eddystone_frame.nid[9] = (uint8_t)((app_settings->network_address) & 0xff);

    // .bid[0], .bid[1] are set to 0x00
    eddystone_frame.bid[2] = (uint8_t)((app_settings->node_address >>  24) & 0xff);
    eddystone_frame.bid[3] = (uint8_t)((app_settings->node_address >>  16) & 0xff);
    eddystone_frame.bid[4] = (uint8_t)((app_settings->node_address >>  8) & 0xff);
    eddystone_frame.bid[5] = (uint8_t)((app_settings->node_address) & 0xff);

    set_common_dataframe(buffer);
    memcpy(pframe, &buffer, sizeof(beacon_common_frame_t));
    i += sizeof(beacon_common_frame_t);
    memcpy(pframe + i, &eddystone_frame, sizeof(eddystone_uid_frame_t));
}
#endif

/**
    @brief      Set IBeacon frame
    @param[out] pframe
                The whole IBeacon frame
*/
static void set_ibeacon_dataframe(uint8_t * pframe)
{
    uint8_t i = 0;
    uint8_t buffer[sizeof(beacon_common_frame_t)+1] = {'\0'};
    positioning_settings_t * app_settings = Pos_get_settings();

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
    ibeacon_frame.uuid[13] = (uint8_t)((app_settings->network_address >>  16) & 0xff);
    ibeacon_frame.uuid[14] = (uint8_t)((app_settings->network_address >>  8) & 0xff);
    ibeacon_frame.uuid[15] = (uint8_t)((app_settings->network_address) & 0xff);

    memcpy(pframe + i, &ibeacon_frame, sizeof(ibeacon_frame_t));
    i += sizeof(ibeacon_frame);
}

/**
    @brief  Modify random Bluetooth LE beacon address with device's addresses
*/
static void set_random_address()
{
    positioning_settings_t * app_settings = Pos_get_settings();

// Address sent in the beacon is defined as "static device address"
// as defined by the BLE standard

    m_addresses[0] = (uint8_t)((app_settings->node_address) & 0xff);
    m_addresses[1] = (uint8_t)((app_settings->node_address >>  8) & 0xff);
    m_addresses[2] = (uint8_t)((app_settings->node_address >>  16) & 0xff);
    m_addresses[3] = (uint8_t)((app_settings->node_address >>  24) & 0xff);
    m_addresses[4] = 0x00;
    m_addresses[5] = 0xC0;

}

/**
    @brief  Set default settings for all Beacons. Beacon(s) advertising
            interval and channels as well as transmission power.

*/
static void set_interval_power_channel_settings()
{
    m_ble_beacon_settings.interval_ms = BEACON_ADV_INTERVAL;
    m_ble_beacon_settings.tx_power = BEACON_TX_POWER;
    m_ble_beacon_settings.adv_channels = APP_LIB_BEACON_TX_CHANNELS_ALL;
}

/**
    @brief      Make the Beacon
    @param[in]  index
                index
    @param[in]  beacon
                Beacon's type
    @param[in]  length
                Beacon's frame length
    @param[in]  enable
                start used or not the Beacon(s)
*/
static void makeBeacon(uint8_t index, const uint8_t * beacon, size_t length,
                       bool enable)
{
    lib_beacon_tx->setBeaconInterval(m_ble_beacon_settings.interval_ms);
    lib_beacon_tx->setBeaconPower(index, &m_ble_beacon_settings.tx_power);
    lib_beacon_tx->setBeaconChannels(index, m_ble_beacon_settings.adv_channels);
    lib_beacon_tx->setBeaconContents(index, beacon, length);
    lib_beacon_tx->enableBeacons(enable);
}

/**
    @brief      Handle the right Beacon(s) frame and set it
    @param[in]  beacon
                Beacon's type
    @param[in]  enable
                start or not the Beacon(s) immediately
*/
static void configure_beacon(uint8_t selection, bool enable)
{
    uint8_t buffer[MAX_BLE_BEACON_TX_SIZE] = {0};

    if ((selection == BEACON_ALL) || (selection == EDDYSTONE))
    {

#ifdef USE_EDYSTONE_UID
        set_eddystone_uid_dataframe(buffer);
#else
        set_eddystone_url_dataframe(buffer);
#endif
        makeBeacon(0, buffer, sizeof(buffer), enable);
    }

    if ((selection == BEACON_ALL) || (selection == IBEACON))
    {
        set_ibeacon_dataframe(buffer);
        makeBeacon(1, buffer, sizeof(buffer), enable);
    }
}

/**
    @brief      Starts sending Bluetooth Low Energy Beacons
*/
static void beacon_enable()
{
    app_res_e rval = 0;

    rval = lib_beacon_tx->enableBeacons(true);
    if (rval == APP_RES_OK)
    {
        m_beacon_state = BEACON_STATE_ON;
        m_offlineTimerStartTime = 0;
        m_offline_timer_on = false;
    }
    else
    {
        // Maybe stack is not running. Try later again. Do not change state
    }
}

/**
    @brief      Check if it is time to turn on beacon(s)
*/
static void check_offline_timeout()
{
    uint32_t now = lib_time->getTimestampS();
    positioning_settings_t * app_settings = Pos_get_settings();

    if ((now - m_offlineTimerStartTime) >= app_settings->ble_beacon_offline_waittime)
    {
        beacon_enable();
    }
}

/**
    @brief      Start offline counter
*/
static void start_offline_timer()
{
    m_offline_timer_on = true;
    m_offlineTimerStartTime = lib_time->getTimestampS();
}

/**
    @brief      Set the Beacon to the correct state after connection update
    @param[in]  connection
                true after got application config message or
                true after received ack of the most recently sent message
                false after didn't get beacons received
*/
static void monitoring_update(bool connection)
{

    // Connection ok
    if (connection)
    {
        if (m_beacon_state == BEACON_STATE_ON)
        {
            beacon_disable();
        }
        else
        {
            m_offline_timer_on = false;
        }
    }
    // Connection fail
    else
    {
        if (m_beacon_state == BEACON_STATE_OFF)
        {
            // Timer is counting
            if (m_offline_timer_on)
            {
                check_offline_timeout();
            }
            else
            {
                start_offline_timer();
            }
        }
    }
}

/**
    @brief  Check the application connection and set the Beacon state
*/
static void monitoring_check()
{
    app_res_e rval = 0;
    size_t count = 0;
    bool appconfig_reception;

    rval = lib_state->getRouteCount(&count);
    appconfig_reception = Scheduler_get_appconfig_reception();


    if (m_beacon_state == BEACON_STATE_OFF)
    {
        if (m_offline_timer_on)
        {
            check_offline_timeout();
        }
        else if ((!m_offline_timer_on) &&
                 (count == 0) &&
                 (rval == APP_RES_OK) &&
                 (!appconfig_reception))
        {
            start_offline_timer();
        }
        else
        {
            // nothing to do
        }
    }
    else
    {
        // Check if the connection is restored
        if (count != 0 && rval == APP_RES_OK)
        {
            beacon_disable();
        }
    }
}

void beacon_disable()
{
    app_res_e rval = 0;

    rval = lib_beacon_tx->enableBeacons(false);
    if (rval == APP_RES_OK)
    {
        m_beacon_state = BEACON_STATE_OFF;
        m_offline_timer_on = false;
        m_offlineTimerStartTime = 0;
    }
    else
    {
        // Maybe stack is not running. Try later again. Do not change state
    }
}

void Ble_Beacon_init(ble_beacon_setup_e setup)
{
    m_configuration = false;
    m_monitoring = true;
    m_offline_timer_on = false;

    // Beacon is never used
    if (setup == BLE_BEACON_OFF)
    {
        beacon_disable();
        m_configuration = true; // configuration done
        m_monitoring = false;   // nothing to do anymore
        return;
    }

    m_beacon_state = BEACON_STATE_OFF;
    set_random_address();
    set_interval_power_channel_settings();
}

void Ble_Beacon_set_configuration()
{

    if (lib_state->getStackState() != APP_LIB_STATE_STARTED)
    {
        return;
    }

    positioning_settings_t * app_settings = Pos_get_settings();

    switch (app_settings->ble_beacon_setup)
    {
        case BLE_BEACON_ON:
            configure_beacon(app_settings->ble_beacon_selection, true);
            // Beacon is always on so for separate monitoring is not required
            m_monitoring = false;
            m_beacon_state = BEACON_STATE_ON;
            break;

        case  BLE_BEACON_ON_WHEN_OFFLINE:
            configure_beacon(app_settings->ble_beacon_selection, false);
            m_monitoring = true;
            m_beacon_state = BEACON_STATE_OFF;
            break;
        default:
            // The same than Beacon is not used
            break;
    }
    m_configuration = true;    // configuration done
}

void Ble_Beacon_check_monitoring(ble_beacon_monitoring_e command)
{
    if ((!m_monitoring) || (!m_configuration))
    {
        return;
    }

    switch (command)
    {
        case MON_CHECK:
            monitoring_check();
            break;

        case MON_CONN_FAIL:
            monitoring_update(false);
            break;

        case MON_UPD_APPCONF:
        case MON_UPD_LATEST_MSG:
            monitoring_update(true);
            break;

        default:
            break;
    }
}
