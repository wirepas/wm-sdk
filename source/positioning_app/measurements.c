/**
    @file measurements.c
    @brief      Handles the processing and dispatch of measurements.
    @copyright  Wirepas Oy 2018
*/

#include "app_settings.h"
#include "scheduler.h"

#include "measurements.h"

static measurement_payload_t m_payload; // buffer to help build the payload
static measurement_table_t m_meas_table; // stores nw/cb data

/**
    @brief      Clears the measurement table.
*/

void Measurements_table_reset(void)
{
    memset(&m_meas_table, '\0', sizeof(m_meas_table));
}

/**
    @brief      Clears the payload buffer.
*/
static void Measurements_payload_reset(void)
{
    memset(&m_payload, '\0', sizeof(m_payload));
    m_payload.ptr = (uint8_t*) &m_payload.bytes;

}

/**
    @brief      Copies a set of bytes to the payload buffer.

                This function keeps track of the payload length by moving
                the internal pointer further down the array.

    @param      from  The memory address where to copy bytes from
    @param[in]  len   The amount of bytes to copy
*/
static void Measurements_payload_copy(void* from, uint8_t len)
{

    memcpy(m_payload.ptr, from, len);
    m_payload.ptr += len;
    Measurements_payload_length();

}


/**
    @brief      Cleans up the beacon measurement table
*/
void Measurements_init(void)
{
    Measurements_table_reset();
    Measurements_payload_reset();
}


/**
    @brief      Initialises the payload buffer for the next message.

    @return     Pointer to the current edge of the payload
*/
const uint8_t* Measurements_payload_init(void)
{
    measurement_header_sequence_t seq_id = Scheduler_get_message_sequence_id() &
                                           0xFFFF;
    Measurements_payload_reset();
    Measurements_payload_copy(&(seq_id),
                              sizeof(measurement_header_sequence_t));

    return (const uint8_t*) &m_payload.bytes;
}


/**
    @brief   Removes ageing beacons (unfinished - wipes table)

             This function operates on the beacon table to remove entries
             older than a given amount of seconds.

    @param[in]  older_than   Remove all beacons seen after this amount of seconds.
    @param[in]  max_beacons  Perform a lookup on the top n members of the table.
*/
void Measurements_clean_beacon(uint32_t older_than, uint8_t max_beacons)
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
                           sizeof(measurement_wm_beacon_t));
                    head++;
                }
            }
        }
    }

}


/**
    @brief      Searchs through the measurement table for the entry with
                the smallest rssi
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


/**
    @brief      Inserts a beacon measurement to the table.

                This method guarantees an unique insertion of a new beacon.
                If the beacon was observed previously, its value will be
                overwritten.
                When there is not enough space for a new beacon, the beacon
                will replace the entry with the lowest

    @param[in]  beacon       The network/cluster beacon
    @param[in]  max_beacons  The maximum beacons
*/
void Measurements_insert_beacon(const app_lib_state_beacon_rx_t* beacon)
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

    }
}


/**
    @brief      Getter for number of beacons

    @return     Returns the number of beacons stored in the measurement table.
*/
uint8_t Measurements_get_num_beacon(void)
{
    return m_meas_table.num_beacons;
}

/**
    @brief      Retreives the size of the buffer.

    @return     The size of the useful payload.
*/
uint8_t Measurements_payload_length(void)
{
    m_payload.len = (uint8_t) (m_payload.ptr - (uint8_t*) & (m_payload.bytes));

    return m_payload.len;
}


/**
    @brief      Adds a RSS measurement to the payload buffer.

    @param[in]  meas_type  The measurement type.

    @return     Pointer to the current edge of the payload.
*/
const uint8_t* Measurements_payload_add_rss(positioning_measurements_e
        meas_type)
{
    uint8_t i = 0;
    uint8_t len = 0;
    int16_t rss = 0;
    uint8_t num_meas = m_meas_table.num_beacons;
    measurement_header_t header;

    if(num_meas > 0)
    {
        header.type = meas_type;
        header.length = num_meas * sizeof(measurement_rss_data_t);

        len = sizeof(measurement_header_t);
        Measurements_payload_copy((void*) &header, len);

        // copy data from internal tracking structure
        for (i = 0; i < num_meas; i++)
        {
            len = sizeof(measurement_payload_addr_t);
            Measurements_payload_copy((void*) & (m_meas_table.beacons[i].address), len);

            len = sizeof(measurement_payload_rss_t);
            // converts rss to a positive 0.5 dBm range
            // the range is saturated to the max representation of 127
            rss = m_meas_table.beacons[i].rss * -2;
            if(rss >= 0xFF)
            {
                rss = 0xFF;
            }
            Measurements_payload_copy((void*) & (rss), len);
        }
    }
    return (const uint8_t*) m_payload.ptr;
}

/**
    @brief      Adds the voltage measurement to the payload buffer.

    @param[in]  void

    @return     void
*/
void Measurements_payload_add_voltage(void)
{
    measurement_header_t header;
    measurement_payload_voltage_t voltage;
    uint8_t len = 0;

    header.type = POS_APP_MEAS_VOLTAGE;
    header.length = sizeof(measurement_payload_voltage_t);
    len = sizeof(measurement_header_t);
    Measurements_payload_copy((void*) &header, len);

    voltage = lib_hw->readSupplyVoltage();
    len = sizeof(measurement_payload_voltage_t);
    Measurements_payload_copy((void*) &voltage, len);
}
