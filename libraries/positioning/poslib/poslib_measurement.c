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
#include "shared_shutdown.h"
#include "shared_data.h"

static uint32_t m_scan_start_time;
/** buffer to help build the payload */
static poslib_meas_payload_t m_payload;
/** stores nw/cb data */
static poslib_meas_table_t m_meas_table;
/** Callback id for shared data cluster beacon */
static uint16_t m_networking_cb_id;
/** Filter id for shared data scan end */
static uint16_t m_networking_scan_id;
/** Scan duration which differs in case of DA or minibeacons */
static uint32_t m_scan_duration_us;
/** 0 if beacons are found, otherwise time in sec when no beacons */
static uint32_t m_time_when_no_beacons_s;

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
 * @param   max_beacons  The maximum beacons
 */
static void insert_beacon(const app_lib_state_beacon_rx_t * beacon);

/**
 * @brief   Wrapper for a new beacon reception callback.
 * @param   beacon  The network beacon
 */
static void cb_beacon_reception(const app_lib_state_beacon_rx_t * beacon);

/**
 * @brief   Wrapper for the network scan end callback
 */
static void scanend_cb(void);

/**
 * @brief   Removes ageing beacons (unfinished - wipes table)
 *
 *          This function operates on the beacon table to remove entries
 *          older than a given amount of seconds.
 *
 * @param   older_than   Remove all beacons seen after this amount of seconds.
 * @param]  max_beacons  Perform a lookup on the top n members of the table.
 */
#ifdef POSLIB_CLEANBEACON_USE
static void void clean_beacon(uint32_t older_than, uint8_t max_beacons);
#endif


static void scanend_cb(void)
{
    poslib_events_info_t msg;

    LOG(LVL_INFO, "");

    msg.event_id = POSLIB_UPDATE_END;
    PosLibCtrl_generateEvent(&msg);

    if (PosLibCtrl_getMode() == POSLIB_MODE_AUTOSCAN_TAG ||
        PosLibCtrl_getMode() == POSLIB_MODE_AUTOSCAN_ANCHOR)
    {
        PosLibMeas_removeCallbacks();
    }

    PosLibEvent_ScanEnd();
    PosLibCtrl_endScan();

    /** update time when no beacons received from scan, keep the oldest time */

    if (m_meas_table.num_beacons == 0 && m_time_when_no_beacons_s == 0)
    {
        m_time_when_no_beacons_s =  lib_time->getTimestampS();
    }
    else if (m_meas_table.num_beacons != 0)
    {
        m_time_when_no_beacons_s = 0;
    }
}

static void cb_beacon_reception(const app_lib_state_beacon_rx_t * beacon)
{
    LOG(LVL_DEBUG, "cb_beacon_reception-scanT: %u",
        lib_time->getTimeDiffUs(lib_time->getTimestampHp() ,
        m_scan_start_time));
    lib_system->enterCriticalSection();
    insert_beacon(beacon); // defines lookup size
    lib_system->exitCriticalSection();
}

/**
 * @brief   Initialization for shared libraries
 */
static void init_shared_libs(void)
{
    /** Make sure that shared items does not exits before new ones */
    Shared_Neighbors_removeBeaconCb(m_networking_cb_id);
    Shared_Neighbors_removeScanNborsCb(m_networking_scan_id);
    Shared_Neighbors_addScanNborsCb(scanend_cb, &m_networking_scan_id);
    Shared_Neighbors_addOnBeaconCb(cb_beacon_reception, &m_networking_cb_id);
}

void PosLibMeas_startScan(poslib_settings_t * settings)
{

    if (settings->node_mode == POSLIB_MODE_AUTOSCAN_TAG ||
        settings->node_mode == POSLIB_MODE_AUTOSCAN_ANCHOR ||
        posLibCtrl_getOneshotStatus())
    {
        poslib_events_info_t msg;
        msg.event_id = POSLIB_UPDATE_START;
        PosLibCtrl_generateEvent(&msg);
        lib_state->setScanDuration(m_scan_duration_us);
        lib_state->startScanNbors();
        m_scan_start_time = lib_time->getTimestampHp();
    }
}

void PosLibMeas_removeCallbacks(void)
{
    Shared_Neighbors_removeBeaconCb(m_networking_cb_id);
    Shared_Neighbors_removeScanNborsCb(m_networking_scan_id);
}

void PosLibMeas_resetTable(void)
{
    memset(&m_meas_table, '\0', sizeof(m_meas_table));
    LOG(LVL_DEBUG, "\"num_beacons\":%d,\"min_rss\":%d,\"min_index\":%d",
        m_meas_table.num_beacons,
        m_meas_table.min_rss, m_meas_table.min_index);
}

/**
 * @brief   Clears the payload buffer.
 */
static void meas_reset_payload(void)
{
    memset(&m_payload, '\0', sizeof(m_payload));
    m_payload.ptr = (uint8_t*) &m_payload.bytes;
    LOG(LVL_DEBUG, "\"bytes\":%d,\"ptr\":%d", &m_payload.bytes, m_payload.ptr);
}

/**
 * @brief   Copies a set of bytes to the payload buffer.
 *          This function keeps track of the payload length by moving
 *          the internal pointer further down the array.
 * @param   from  The memory address where to copy bytes from
 * @param   len   The amount of bytes to copy
 */
static void meas_copy_payload(void * from, uint8_t len)
{
    memcpy(m_payload.ptr, from, len);
    m_payload.ptr += len;
    PosLibMeas_getPayloadLen();
}

void PosLibMeas_initMeas(void)
{
    PosLibMeas_resetTable();
    meas_reset_payload();
    init_shared_libs();
}

const uint8_t * PosLibMeas_initPayload(void)
{
    poslib_meas_header_sequence_t seq_id = PosLibCtrl_incrementSeqId() & 0xFFFF;
    meas_reset_payload();
    meas_copy_payload(&(seq_id), sizeof(poslib_meas_header_sequence_t));
    LOG(LVL_DEBUG, "\"initializing_payload\":%d", seq_id);
    return (const uint8_t*) &m_payload.bytes;
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
                      LOG(LVL_DEBUG, "\"head\":%d,\"tail\":%d",head, i);
                }
            }
        }
    }

    LOG(LVL_DEBUG, "\"now\":%d,\"num_beacons\":%d",
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
        if (m_meas_table.beacons[i].rss < m_meas_table.min_rss)
        {
            m_meas_table.min_index = i;
            m_meas_table.min_rss = m_meas_table.beacons[i].rss;
        }
    }
}

static void insert_beacon(const app_lib_state_beacon_rx_t * beacon)
{
    uint8_t i = 0;
    uint8_t insert_idx = MAX_BEACONS;
    bool match = false;

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
    if(! match)
    {
        if(m_meas_table.num_beacons == MAX_BEACONS) // no space
        {
            update_min();

            if(beacon->rssi > m_meas_table.min_rss)
            {
                insert_idx = m_meas_table.min_index;
            }
        }
        else
        {
            insert_idx = m_meas_table.num_beacons;
            m_meas_table.num_beacons++; // table size has to be increased
        }
    }

    // update the table
    if(insert_idx < MAX_BEACONS)
    {
        m_meas_table.beacons[insert_idx].address = beacon->address;
        m_meas_table.beacons[insert_idx].rss = beacon->rssi;
        m_meas_table.beacons[insert_idx].txpower = beacon->txpower;
        m_meas_table.beacons[insert_idx].last_update = lib_time->getTimestampHp();
        m_meas_table.beacons[insert_idx].beacon_type = beacon->type;

        LOG(LVL_DEBUG, "\"idx\":%d,\"address\":%d,\"rss\":%d,\"txpower\":%d,\"type\":%d",
            insert_idx,
            beacon->address,
            beacon->rssi,
            beacon->txpower,
            beacon->type);
    }
    else
    {
        LOG(LVL_DEBUG, "\"msg\":\"ignoring beacon\",\"address\":%d,\"rss\":%d",
            beacon->address,
            beacon->rssi);
    }
}

uint8_t PosLibMeas_getNumofBeacons(void)
{
    return m_meas_table.num_beacons;
}

uint8_t PosLibMeas_getPayloadLen(void)
{
    m_payload.len = (uint8_t) (m_payload.ptr - (uint8_t*) & (m_payload.bytes));
    LOG(LVL_DEBUG, "\"payload_len\":%d", m_payload.len);
    return m_payload.len;
}

const uint8_t * PosLibMeas_addPayloadRss(poslib_measurements_e meas_type)
{
    uint8_t i = 0;
    uint8_t len = 0;
    int16_t rss = 0;
    uint8_t num_meas = m_meas_table.num_beacons;
    poslib_meas_header_t header;

    if(num_meas > 0)
    {
        header.type = meas_type;
        header.length = num_meas * sizeof(poslib_meas_rss_data_t);

        len = sizeof(poslib_meas_header_t);
        meas_copy_payload((void*) &header, len);

        // copy data from internal tracking structure
        for (i = 0; i < num_meas; i++)
        {
            len = sizeof(poslib_meas_payload_addr_t);
            meas_copy_payload((void*) &
                                     (m_meas_table.beacons[i].address), len);

            len = sizeof(poslib_meas_payload_rss_t);
            // converts rss to a positive 0.5 dBm range
            // the range is saturated to the max representation of 127
            rss = m_meas_table.beacons[i].rss * -2;
            if(rss >= 0xFF)
            {
                rss = 0xFF;
            }
            meas_copy_payload((void*) & (rss), len);
        }
    }

    LOG(LVL_INFO, "\"num_rss_measurements\":%d,\"payload_length\":%d",
        num_meas, m_payload.len);
    return (const uint8_t*) m_payload.ptr;
}

void PosLibMeas_addVoltageToPayload(void)
{
    poslib_meas_header_t header;
    poslib_meas_payload_voltage_t voltage;
    uint8_t len = 0;

    header.type = POSLIB_MEAS_VOLTAGE;
    header.length = sizeof(poslib_meas_payload_voltage_t);
    len = sizeof(poslib_meas_header_t);
    meas_copy_payload((void*) &header, len);

    voltage = lib_hw->readSupplyVoltage();
    len = sizeof(poslib_meas_payload_voltage_t);
    meas_copy_payload((void*) &voltage, len);
    LOG(LVL_INFO, "\"voltage [mV]\" : %d, \"packet_id\" : %d",
        voltage, PosLibCtrl_getSeqId());
}

uint32_t PosLibMeas_getTimeNoBeacons(void)
{
    return m_time_when_no_beacons_s;
}
