/**
 * @file poslib_ble_beacon.c
 * @brief BLE beacons module.
 * @copyright Wirepas Ltd 2021
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
#include "poslib_ble_beacon.h"
#include "shared_beacon.h"
#include "api.h"

#define MAX_BLE_BEACON_TX_SIZE     38
#define BEACON_TX_POWER 0
#define USE_EDYSTONE_UID  //if not defined Edystone URL will be set
#define BLE_STACK_WAIT_MS 15000  

#define TIME_SEC_TO_MSEC(tm) ((tm) * 1000U)

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
    int8_t tx_power;
} ibeacon_frame_t;

#define AD_TYPE 0x42            // Non-connectable Beacon
static uint8_t m_addresses[6];  // Storage for Node Id and Network address
static uint8_t m_eddystone_idx;
static uint8_t m_ibeacon_idx;
static bool m_eddystone_enabled = false;
static bool m_ibeacon_enabled = false;
static bool m_ble_initialised = true;
static ble_beacon_settings_t m_ble_settings;
static bool m_activation_started = false;

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
static void set_eddystone_dataframe(uint8_t * pframe)
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
static void set_eddystone_dataframe(uint8_t * pframe, int8_t tx_power)
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
        .tx_power = tx_power, // set to TX power
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
static void set_ibeacon_dataframe(uint8_t * pframe, int8_t tx_power)
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
        .tx_power = tx_power
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

    LOG(LVL_DEBUG, "Ble Beacon updated for node :0x%x", node_address);
}

bool start_beacon(poslib_ble_type_e ble_type, 
                    poslib_ble_mode_config_t * ble_cfg,
                    uint8_t * ble_index)
{
    uint8_t buffer[MAX_BLE_BEACON_TX_SIZE] = {0};

    set_random_address(); 

    if (ble_type == POSLIB_EDDYSTONE)
    {
        set_eddystone_dataframe(buffer, ble_cfg->tx_power);
        if (m_eddystone_enabled)
        {
            Shared_Beacon_stopBeacon(m_eddystone_idx);
            m_eddystone_enabled = false;
        }
    }
    else if (ble_type == POSLIB_IBEACON)
    {
        set_ibeacon_dataframe(buffer, ble_cfg->tx_power);
        if (m_ibeacon_enabled)
        {
           Shared_Beacon_stopBeacon(m_ibeacon_idx);
           m_ibeacon_enabled = false; 
        }
    }
    else
    {
        LOG(LVL_ERROR, "Unknown beacon type: %u", ble_type);
        return false;
    }

    return (Shared_Beacon_startBeacon(ble_cfg->tx_interval_ms,
                                        &ble_cfg->tx_power,
                                        ble_cfg->channels,
                                        buffer,
                                        sizeof(buffer),
                                        ble_index) == SHARED_BEACON_RES_OK);
}

void start_beacons(ble_beacon_settings_t * settings)
{
    bool send_event = false;
  
    if (settings->type == POSLIB_BEACON_ALL || 
        settings->type == POSLIB_EDDYSTONE)
    {
        m_eddystone_enabled = start_beacon(POSLIB_EDDYSTONE, 
                            &settings->eddystone,
                            &m_eddystone_idx);

        if (m_eddystone_enabled)
        {
            send_event = true;
            LOG(LVL_DEBUG, "BLE Eddystone started");
        }
    }

    if (settings->type == POSLIB_BEACON_ALL || 
        settings->type == POSLIB_IBEACON)
    {
        m_ibeacon_enabled = start_beacon(POSLIB_IBEACON, 
                            &settings->ibeacon,
                            &m_ibeacon_idx);

        if (m_ibeacon_enabled)
        {
            send_event = true;
            LOG(LVL_DEBUG, "BLE iBeacon started");
        }
    }

    if (send_event)
    {
        PosLibEvent_add(POSLIB_CTRL_EVENT_BLE_START);    
    }
}

static uint32_t start_beacons_task(void)
{
    if(m_ble_initialised)
    {
        if (lib_state->getStackState() == APP_LIB_STATE_STARTED ||
            lib_sleep->getSleepState() == APP_LIB_SLEEP_STARTED)
        {
            start_beacons(&m_ble_settings);
            return APP_SCHEDULER_STOP_TASK;
        }
        else
        {
            LOG(LVL_INFO, "BLE start task - stack not started - wait");
            return BLE_STACK_WAIT_MS;
        }
    }
    LOG(LVL_INFO, "BLE start task - nothing to do");
    return APP_SCHEDULER_STOP_TASK;
}

/**
 * @brief   Stops beacon based global parameters.
 */
static void stop_beacons(void)
{
    shared_beacon_res_e sh_res = SHARED_BEACON_RES_OK;
    App_Scheduler_cancelTask(start_beacons_task);

    if (m_eddystone_enabled)
    {
        sh_res |= Shared_Beacon_stopBeacon(m_eddystone_idx);
        m_eddystone_enabled = false;
         LOG(LVL_INFO, "Eddystone beacon stopped");
    }

    if (m_ibeacon_enabled)
    {
        sh_res |= Shared_Beacon_stopBeacon(m_ibeacon_idx);
        m_ibeacon_enabled = false;
        LOG(LVL_INFO, "iBeacon stopped");
    }

    if (sh_res == SHARED_BEACON_RES_OK)
    {
       PosLibEvent_add(POSLIB_CTRL_EVENT_BLE_STOP); 
    }
    else
    {
        LOG(LVL_ERROR, "Beacon disabling failed");
    }
}

poslib_ret_e PosLibBle_start(ble_beacon_settings_t * settings)
{
    
    if (m_ble_initialised && 
        memcmp(&m_ble_settings, settings, sizeof(ble_beacon_settings_t)) == 0)
    {
        // Same settings nothing to do
        return POS_RET_OK;   
    }
    
    m_ble_initialised = true;
    memcpy(&m_ble_settings, settings, sizeof(ble_beacon_settings_t));

    switch (settings->mode)
    {
        case POSLIB_BLE_OFF:
        {
            stop_beacons();
            break;
        }

        case POSLIB_BLE_ON:
        {   
            App_Scheduler_cancelTask(start_beacons_task);
            App_Scheduler_addTask_execTime(start_beacons_task, BLE_STACK_WAIT_MS, 500);
            break; 
        }

        case POSLIB_BLE_ON_WHEN_OFFLINE:
        {
            //Nothing to for now; will be activated when offline
            break;
        }

        default:
        {
            LOG(LVL_DEBUG, "BLE mode: %u unknown", settings->mode);
            m_ble_initialised = false;
            return POS_RET_INVALID_PARAM;
        }
    }
    
    return POS_RET_OK;
}

void PosLibBle_stop()
{
    stop_beacons();
    m_ble_initialised = false;
}
    
static void setConnectionStatus(bool wm_coverage)
{
    if (!m_ble_initialised ||
        m_ble_settings.mode != POSLIB_BLE_ON_WHEN_OFFLINE)
    {
        // Nothing to do
        return;
    }

    if (wm_coverage)
    {
        /* Stop beacons when under WM coverage */
        LOG(LVL_DEBUG, "BLE beacons stopped!");
        stop_beacons();
        m_activation_started = false;
    }
    else if (!m_activation_started)
    {
        /*Start beacons if outside coverage for > activation_delay_s */
        m_activation_started = true;
        App_Scheduler_cancelTask(start_beacons_task);
        App_Scheduler_addTask_execTime(start_beacons_task,
            TIME_SEC_TO_MSEC(m_ble_settings.activation_delay_s), 500);

        LOG(LVL_DEBUG, "BLE beacons to be activated in %u sec", 
            m_ble_settings.activation_delay_s);
    }
}

bool PosLibBle_getEddystoneStatus()
{
    return m_eddystone_enabled;
}

bool PosLibBle_getiBeaconStatus()
{
    return m_ibeacon_enabled;
}

void PosLibBle_processEvent(poslib_internal_event_t * event)
{
    switch(event->type)
    {
        case POSLIB_CTRL_EVENT_OUTSIDE_WM:
        {
           setConnectionStatus(false);
           break; 
        }
        case POSLIB_CTRL_EVENT_UNDER_WM:
        {
           setConnectionStatus(true);
           break; 
        }
        default:
        {
            break;
        }
    }
}