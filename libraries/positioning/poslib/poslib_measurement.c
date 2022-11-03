/**
 * @file       poslib_measurements.c
 * @brief      contains the routines with WM stack interface like scan control.
 * @copyright  Wirepas Ltd.2020
 */

#define DEBUG_LOG_MODULE_NAME "POSLIB_MEAS"
#ifdef DEBUG_POSLIB_LOG_MAX_LEVEL
#define DEBUG_LOG_MAX_LEVEL DEBUG_POSLIB_LOG_MAX_LEVEL
#else
#define DEBUG_LOG_MAX_LEVEL LVL_NOLOG
#endif
#include "debug_log.h"
#include <string.h>
#include "app_scheduler.h"
#include "poslib.h"
#include "poslib_control.h"
#include "poslib_event.h"
#include "poslib_measurement.h"
#include "shared_neighbors.h"
#include "shared_data.h"
#include "stack_state.h"
#include "api.h"
#include "voltage.h"


/** Internal module structures */

typedef enum
{
    POSLIB_MEAS_BEACON_TYPE_CB = 0, //cluster-beacon
    POSLIB_MEAS_BEACON_TYPE_NB = 1, // network-beacon
    POSLIB_MEAS_BEACON_TYPE_MBCN = 2, //mini-beacon
    POSLIB_MEAS_BEACON_TYPE_FLT = 3, // mixed (filtered) beacon
    POSLIB_MEAS_BEACON_TYPE_UNKNOWN = 4,
} poslib_meas_beacon_type_e;

typedef struct
{
    app_addr_t address;
    int16_t  norm_rss; // normalized to 0dB TX power
    int8_t   txpower;
    poslib_meas_beacon_type_e  type;
    uint8_t samples;  
    app_lib_time_timestamp_hp_t last_update;
} poslib_meas_wm_beacon_t;

typedef struct
{
    poslib_meas_wm_beacon_t beacons[MAX_BEACONS];
    uint8_t num_beacons;
    uint8_t min_index;
    int16_t min_rss;
} poslib_meas_table_t;

/**
 * @brief buffer to help prepare the measurement payload.
 */
typedef struct
{
    uint8_t * dest;
    uint8_t max_len;
    uint8_t * cursor;
} poslib_meas_payload_buffer_t;

static poslib_meas_table_t m_meas_table;


/** Callbacks state variables */
static uint16_t m_beacon_cb_id = 0;
static bool m_beacon_cb_reg = false;

static bool m_opportunistic = false;

static shared_data_item_t m_mbcn_item;

// Maximum sample history in RSSI filter
#define MAX_FLT_SAMPLES 8

/** 0 if beacons are found, otherwise time in sec when no beacons */
static uint32_t m_time_when_no_beacons_s;

static bool m_scan_pending = false;
static bool m_init = false;

#ifdef CONF_VOLTAGE_REPORT
/** Voltage sampling variables */
static uint8_t m_samples = 0;
static uint32_t m_voltage_sampling_time_s = 0;
static uint16_t m_voltage_flt = 0;
#endif

/**
 * @brief   Inserts a beacon measurement to the table.
 *
 *          This method guarantees an unique insertion of a new beacon.
 *          If the beacon was observed previously, its value will be
 *          overwritten.
 *          When there is not enough space for a new beacon, the beacon
 *          will replace the entry with the lowest
 *
 * @param   beacon       The network/cluster beacon
 */
static void insert_beacon(const poslib_meas_wm_beacon_t * beacon);

// Forward declaration
static void deregister_callbacks(void);


#ifdef POSLIB_CLEANBEACON_USE
/**
 * @brief   Removes ageing beacons (unfinished - wipes table)
 *
 *          This function operates on the beacon table to remove entries
 *          older than a given amount of seconds.
 *
 * @param   older_than   Remove all beacons seen after this amount of seconds.
 * @param]  max_beacons  Perform a lookup on the top n members of the table.
 */

static void void clean_beacon(uint32_t older_than, uint8_t max_beacons);
#endif

/**
 * @brief   Callback for scan end
 *
 * @param   void
 */
static void scanend_cb(app_lib_stack_event_e event, void * param_p)
{
    app_lib_settings_role_t node_role;
    app_lib_state_neighbor_scan_info_t * scan_info;

    lib_settings->getNodeRole(&node_role);
    scan_info = (app_lib_state_neighbor_scan_info_t *) param_p;

    if ((scan_info->complete) || (node_role == APP_LIB_SETTINGS_ROLE_ADVERTISER))
    {
        if (!m_opportunistic)
        {
            deregister_callbacks();
        }
        if (m_meas_table.num_beacons == 0 && m_time_when_no_beacons_s == 0)
        {
            m_time_when_no_beacons_s =  lib_time->getTimestampS();
        }
        else if (m_meas_table.num_beacons != 0)
        {
            m_time_when_no_beacons_s = 0;
        }
    }

    PosLibEvent_add(POSLIB_CTRL_EVENT_SCAN_END);
    m_scan_pending = false;
}

/**
 * @brief   Callback for beacon RX
 *
 * @param   beacon input of type \ref app_lib_state_beacon_rx_t
 */
static void beacon_cb(const app_lib_state_beacon_rx_t * beacon)
{
    poslib_meas_wm_beacon_t bcn;

    bcn.address = beacon->address;
    bcn.norm_rss = beacon->rssi - beacon->txpower;
    bcn.txpower = beacon->txpower;
    bcn.last_update = lib_time->getTimestampHp();
    bcn.samples = 1;

    if (beacon->type == APP_LIB_STATE_BEACON_TYPE_NB)
    {
        bcn.type = POSLIB_MEAS_BEACON_TYPE_NB;
    }
    else if (beacon->type == APP_LIB_STATE_BEACON_TYPE_CB)
    {
        bcn.type = POSLIB_MEAS_BEACON_TYPE_CB;
    }
    else
    {
        bcn.type = POSLIB_MEAS_BEACON_TYPE_UNKNOWN;
    }
    
    if (beacon->is_da_support)
    {
        LOG(LVL_INFO, "DA router: %u", beacon->address);
    }

    insert_beacon(&bcn); 
}

static app_lib_data_receive_res_e mbcn_cb(const shared_data_item_t * item,
                                    const app_lib_data_received_t * data)
{
    poslib_meas_wm_beacon_t bcn;
    
    if (item->filter.src_endpoint != POSLIB_MBCN_SRC_EP ||
        item->filter.dest_endpoint != POSLIB_MBCN_DEST_EP)
    {
        LOG(LVL_ERROR, "Incorect EP for mini-beacon");
        return APP_LIB_DATA_RECEIVE_RES_HANDLED;
    }

    bcn.address = data->mac_src_address;
    bcn.norm_rss = data->rssi - data->tx_power;
    bcn.txpower = data->tx_power;
    bcn.last_update = lib_time->getTimestampHp();
    bcn.type = POSLIB_MEAS_BEACON_TYPE_MBCN;
    bcn.samples = 1;

    //FixMe: decode beacon data    
    insert_beacon(&bcn); 

    return APP_LIB_DATA_RECEIVE_RES_HANDLED;
}

void init_mbcn_item()
{
    memset(&m_mbcn_item, 0, sizeof(m_mbcn_item));
    m_mbcn_item.cb = mbcn_cb;
    m_mbcn_item.filter.mode = SHARED_DATA_NET_MODE_BROADCAST;
    m_mbcn_item.filter.src_endpoint = POSLIB_MBCN_SRC_EP;
    m_mbcn_item.filter.dest_endpoint = POSLIB_MBCN_DEST_EP;
    m_mbcn_item.filter.multicast_cb = NULL;
}

/**
 * @brief   Deregister callbacks (beacon cb & scan end) if registered
 *
 * @param   void
 */
static void register_callbacks(void)
{
   app_res_e res;
   res = Stack_State_addEventCb(scanend_cb,
                                1 << APP_LIB_STATE_STACK_EVENT_SCAN_STOPPED);
    if (res != APP_RES_OK)
    {
        LOG(LVL_ERROR, "Cannot register scan end cb. res %u", res);
    }

    if (!m_beacon_cb_reg)
    {
        Shared_Neighbors_addOnBeaconCb(beacon_cb, &m_beacon_cb_id);
        m_beacon_cb_reg = true;
    }

    if (m_mbcn_item.cb == NULL)
    {
        init_mbcn_item();
        res = Shared_Data_addDataReceivedCb(&m_mbcn_item);
        if (res != APP_RES_OK)
        {
            m_mbcn_item.cb = NULL;
        }
    }
}

/**
 * @brief   De register callbacks (beacon cb & scan end) if registered
 *
 * @param   void
 */
static void deregister_callbacks(void)
{
    Stack_State_removeEventCb(scanend_cb);

    if (m_beacon_cb_reg)
    {
        Shared_Neighbors_removeBeaconCb(m_beacon_cb_id);
        m_beacon_cb_reg = false;
    }

    if (m_mbcn_item.cb != NULL)
    {
        Shared_Data_removeDataReceivedCb(&m_mbcn_item);
        m_mbcn_item.cb = NULL;
    }
}

static void init_module(void)
{
    memset(&m_meas_table, 0, sizeof(m_meas_table));
}

static void clear_measurement_table()
{
    m_meas_table.num_beacons = 0;
    memset(&m_meas_table.beacons, 0, sizeof(m_meas_table.beacons));
}

/**
 * @brief   Enable/dissable opportunistic "scan"
 *
 * @param   bool true/false enable/disable control 
 */
bool PosLibMeas_opportunisticScan(bool enable)
{
    
    if (!m_init)
    {
        init_module();
        m_init = true;
    }

    if (enable)
    {
        register_callbacks();
        m_opportunistic = true;
        clear_measurement_table();
    }
    else
    {
        deregister_callbacks();
        m_opportunistic = false;
    }
    return true;
}

static uint32_t stopScanNbors()
{
    lib_state->stopScanNbors();
    return APP_SCHEDULER_STOP_TASK;
}

bool PosLibMeas_startScan(poslib_scan_ctrl_t * scan_ctrl)
{
    if (!m_init)
    {
        init_module();
        m_init = true;
    }

    if (m_scan_pending)
    {
        LOG(LVL_ERROR, "Scan already ongoing");
        return false;
    }

    if (scan_ctrl->mode != SCAN_MODE_STANDARD)
    {
        LOG(LVL_ERROR, "Unknown scan mode: %u", scan_ctrl->mode);
        return false;
    }
 
    clear_measurement_table();
    register_callbacks(); 

    if (lib_state->startScanNbors() != APP_RES_OK)
    {
        LOG(LVL_ERROR, "Cannot start scan. mode: %u, duration: %u us",
            scan_ctrl->mode, scan_ctrl->max_duration_ms);
        return false;
    }
    
    m_scan_pending = true;

    // Schedule task to stop the scan after the requested time (!= 0)
    if (scan_ctrl->max_duration_ms > 0)
    {
        if (App_Scheduler_addTask_execTime(stopScanNbors, scan_ctrl->max_duration_ms, 100) != APP_SCHEDULER_RES_OK)
        {
            LOG(LVL_ERROR, "Cannot add task to stop scan after %d ms",
                scan_ctrl->max_duration_ms);
            // Scan will go till the end
        }
    }
    
    LOG(LVL_DEBUG, "Scan requested. mode: %u, duration: %u us", 
        scan_ctrl->mode, scan_ctrl->max_duration_ms);

    PosLibEvent_add(POSLIB_CTRL_EVENT_SCAN_STARTED);
    return true;
}

void PosLibMeas_stop(void)
{
    deregister_callbacks();
    m_opportunistic = false;
    m_scan_pending = false;
}

#ifdef POSLIB_CLEANBEACON_USE
static void clean_beacon(uint32_t older_than, uint8_t max_beacons)
{
    uint8_t i = 0;
    uint8_t head = 0;

    app_lib_time_timestamp_hp_t now = lib_time->getTimestampHp();

    for (i = 0; i < max_beacons && i < MAX_BEACONS
            && m_meas_table.num_beacons > 0; i++)
    {
        if (m_meas_table.beacons[i].address != 0)
        {
            if ((lib_time->getTimeDiffUs(now,
                m_meas_table.beacons[i].last_update) / 1e6) >= older_than)

            {
                m_meas_table.num_beacons--;
            }
            else
            {
                //moves beacon up the table
                if(i != head)
                {
                    memcpy(&m_meas_table.beacons[head],
                           &m_meas_table.beacons[i],
                           sizeof(poslib_meas_wm_beacon_t));
                    head++;
                      LOG(LVL_DEBUG, " head :%d, tail :%d",head, i);
                }
            }
        }
    }

    LOG(LVL_DEBUG, " now :%d, num_beacons :%d",
        now, m_meas_table.num_beacons);
}
#endif

/**
 * @brief   Searchs through the measurement table for the entry with
 *          the smallest rssi
 */
static void update_min(void)
{
    uint8_t i = 0;
    m_meas_table.min_rss = 0; // forces a minimum refresh

    // update minimum
    for (i = 0; i < m_meas_table.num_beacons; i++)
    {
        // updates minimum location
        if (m_meas_table.beacons[i].norm_rss < m_meas_table.min_rss)
        {
            m_meas_table.min_index = i;
            m_meas_table.min_rss = m_meas_table.beacons[i].norm_rss;
        }
    }
}

static void insert_beacon(const poslib_meas_wm_beacon_t * beacon)
{
    uint8_t i = 0;
    uint8_t insert_idx = MAX_BEACONS;
    bool match = false;
    poslib_meas_wm_beacon_t * bcn = NULL;

    // searches for itself
    for (i = 0; i < m_meas_table.num_beacons; i++)
    {
        // if there is an entry present, use that index
        if (m_meas_table.beacons[i].address == beacon->address)
        {
            insert_idx = i;
            match = true;
            break;
        }
    }

    // if there is no entry in the table for the given address, then simply
    // append the beacon, otherwise replace the entry with the lowest minimum
    if (!match)
    {
        if(m_meas_table.num_beacons == MAX_BEACONS) // no space
        {
            update_min();

            if(beacon->norm_rss > m_meas_table.min_rss)
            {
                insert_idx = m_meas_table.min_index;
            }
        }
        else
        {
            insert_idx = m_meas_table.num_beacons;
            m_meas_table.num_beacons++; 
        }
    }

    // update the table
    if (insert_idx < MAX_BEACONS)
    {
        bcn = &m_meas_table.beacons[insert_idx];
        if (bcn->samples < MAX_FLT_SAMPLES)
        {
           bcn->samples++; 
        }
        bcn->address = beacon->address;
        bcn->txpower = beacon->txpower;
        bcn->last_update = lib_time->getTimestampHp();
        
        if (bcn->samples > 1)
        {
            bcn->type = POSLIB_MEAS_BEACON_TYPE_FLT;
            bcn->norm_rss += (beacon->norm_rss - bcn->norm_rss) / bcn->samples;
        }
        else
        {
            bcn->type = beacon->type;
            bcn->norm_rss = beacon->norm_rss;
        }

        LOG(LVL_DEBUG, "idx:%d,address:%d,rss:%d,txpower:%d,type:%d",
            insert_idx,
            beacon->address,
            beacon->norm_rss,
            beacon->txpower,
            beacon->type);
    }
    // else
    // {
    //     LOG(LVL_DEBUG, "Ignoring beacon address :%u, rss :%d",
    //         beacon->address,
    //         beacon->rssi);
    // }
}

uint8_t PosLibMeas_getBeaconNum(void)
{
    return m_meas_table.num_beacons;
}

uint8_t get_payload_len(poslib_meas_payload_buffer_t * buf)
{
    return ((uint8_t*) buf->cursor - (uint8_t*) buf->dest);
}

static bool meas_copy_payload(poslib_meas_payload_buffer_t * buf, void * from, uint8_t len)
{
    uint8_t used = get_payload_len(buf); 

    if (len > (buf->max_len - used))
    {
        return false;
    }
    memcpy(buf->cursor, from, len);
    buf->cursor += len;
    return true;
}

static bool add_node_info_record(poslib_meas_payload_buffer_t * buf, poslib_meas_record_node_info_t * node_info)
{
    poslib_meas_record_header_t header;
    bool ret = true;
    uint8_t * prev_cursor = buf->cursor;

    // add header
    header.type = POSLIB_MEAS_NODE_INFO;
    header.length = sizeof(poslib_meas_record_node_info_t);
    ret &= meas_copy_payload(buf, (void*) &header, sizeof(header));

    // add payload
    ret &= meas_copy_payload(buf, (void*) node_info, sizeof(poslib_meas_record_node_info_t));
    
    if (ret)
    {    
        LOG(LVL_DEBUG, "Node info  added");
        return true;
    }
    else
    {
        LOG(LVL_ERROR, "Not enough space to add node info");
        buf->cursor = prev_cursor; //revert cursor
        return false;
    }
}

static uint8_t add_rss_record(poslib_meas_payload_buffer_t * buf, poslib_measurements_e meas_type)
{
    int16_t norm_rss = 0;
    uint8_t num_meas = m_meas_table.num_beacons;
    poslib_meas_record_header_t header;
    poslib_meas_rss_data_t data;
    bool ret = true;
    uint8_t * prev_cursor = buf->cursor;

    if(num_meas == 0)
    {
        LOG(LVL_WARNING, "No measurements available");
        return num_meas;
    }
    else
    {
        // add header
        header.type = meas_type;
        header.length = num_meas * sizeof(poslib_meas_rss_data_t);
        ret &= meas_copy_payload(buf, (void*) &header, sizeof(header));

        // add RSS
        for (uint8_t i = 0; i < num_meas; i++)
        {
            data.address = m_meas_table.beacons[i].address;
            norm_rss = m_meas_table.beacons[i].norm_rss * -2;
            data.norm_rss = (norm_rss >= 0xFF) ? 0xFF : norm_rss;
            ret &= meas_copy_payload(buf, (void*) & (data), sizeof(data));
            if(!ret)
            {
                break;
            }
        }
    }

    if (ret)
    {    
        LOG(LVL_DEBUG, "Measurements added %u", num_meas);
        return num_meas;
    }
    else
    {
        LOG(LVL_ERROR, "Not enough space for all measurements");
        buf->cursor = prev_cursor; //revert cursor
        return 0;
    }
}


static uint16_t get_voltage(void)
#ifdef CONF_VOLTAGE_REPORT
{
    uint32_t now_s = lib_time->getTimestampS();
    uint32_t delta_s = now_s - m_voltage_sampling_time_s;
    uint16_t voltage = 0;

    if (delta_s >= POSLIB_VOLTAGE_SAMPLING_MAX_S || m_samples == 0)
    {
        int16_t delta_v;
        voltage = Mcu_voltageGet();
        m_voltage_sampling_time_s = now_s;

        if (m_samples >= POSLIB_VOLTAGE_FILTER_SAMPLES)
        {
            delta_v = (int16_t)(voltage - m_voltage_flt) / POSLIB_VOLTAGE_FILTER_SAMPLES;
        }
        else 
        {
            m_samples++;
            delta_v = (int16_t)(voltage - m_voltage_flt) / m_samples;
        }
        
        m_voltage_flt += delta_v;
    }
    
    LOG(LVL_DEBUG, "Voltage flt: %d sample: %u", m_voltage_flt, voltage);
    return m_voltage_flt;
}
#else
{
    return 0;
}
#endif

static bool add_voltage_record(poslib_meas_payload_buffer_t * buf)
{
    poslib_meas_record_voltage_t record;
    bool ret = false;
    uint8_t * prev_cursor = buf->cursor;

    record.header.type = POSLIB_MEAS_VOLTAGE;
    record.header.length = sizeof(poslib_meas_record_voltage_t) 
                            - sizeof(poslib_meas_record_header_t);

    record.voltage =  get_voltage();
    ret = meas_copy_payload(buf, (void*) &record, sizeof(record));

    if (!ret)
    {
        LOG(LVL_ERROR, "Not enough space to add voltage");
        buf->cursor = prev_cursor; //revert cursor
    }

    return ret;
}

bool PosLibMeas_getPayload(uint8_t * bytes, uint8_t max_len, uint8_t sequence,
                                poslib_measurements_e meas_type, bool add_voltage, 
                                poslib_meas_record_node_info_t * node_info, 
                                uint8_t * num_bytes, uint8_t * num_meas)
{
    poslib_meas_payload_buffer_t buf;
    buf.dest = bytes;
    buf.cursor = buf.dest;
    buf.max_len = max_len;
    bool ret = true;
    poslib_meas_message_header_t msg_header;

    *num_meas = 0;
    *num_bytes = 0;

    // add message header
    msg_header.sequence = sequence;
    ret &= meas_copy_payload(&buf, (void*) &msg_header, sizeof(msg_header));

    // add RSS measurements
    if (ret)
    {
        *num_meas = add_rss_record(&buf, meas_type);
        ret = (*num_meas == 0) ? false : true;
    }
    // add voltage
    if (ret && add_voltage)
    {
        ret &= add_voltage_record(&buf);
    }

    //add node info
    if (ret && node_info != NULL)
    {
        ret &= add_node_info_record(&buf, node_info);  
    }

    *num_bytes = get_payload_len(&buf);

    return ret;
}

void PosLibMeas_clearMeas(void)
{
    memset(&m_meas_table, 0, sizeof(m_meas_table));
}