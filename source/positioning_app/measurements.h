/**
    @file measurements.h
    @brief      Header for measurements.c
    @copyright  Wirepas Oy 2018
*/

#ifndef POS_MEASUREMENTS_H
#define POS_MEASUREMENTS_H

#include <stdbool.h>
#include <stdint.h>
#include <string.h>


/**
    @brief  defines the device's positioning mode.

    The type is matched against the WPE proto definition.
*/
typedef enum
{
// 0x00 - 0x6F : range reserved for Wirepas
    POS_APP_MEAS_RSS_SR = 0x00,
    POS_APP_MEAS_RSS_HR = 0x01,
    POS_APP_MEAS_TIME = 0x02,
    POS_APP_MEAS_SPACE = 0x03,
    POS_APP_MEAS_VOLTAGE = 0x04,
    POS_APP_MEAS_RSS_SR_4BYTE_ADDR = 0x05,

// 0x70 - 0xEF : range reserved for customer
// Please notify Wirepas if a custom record ID is used by
// your customised positoning app

// 0xF0 - 0xFF : range reserved for Wirepas
    POS_APP_MEAS_RSS_SR_ANCHOR = 0xF0,
    POS_APP_MEAS_RSS_SR_ANCHOR_4BYTE_ADDR = 0xF5
} positioning_measurements_e;

#define MAX_PAYLOAD  102  // 2.4 profile - limited by internal memory

#ifdef USE_3BYTE_NODE_ADDRESS
// only for 4.x stack
#define MAX_BEACONS  20  // limited by internal memory
#define NODE_ADDRESS_LENGTH 3 // node address length in bytes
#define DEFAULT_MEASUREMENT_TYPE_TAG POS_APP_MEAS_RSS_SR
#define DEFAULT_MEASUREMENT_TYPE_ANCHOR POS_APP_MEAS_RSS_SR_ANCHOR
#else
// 5.x stack version
#define MAX_BEACONS  16  // limited by internal memory
#define NODE_ADDRESS_LENGTH 4 // node address length in bytes
#define DEFAULT_MEASUREMENT_TYPE_TAG POS_APP_MEAS_RSS_SR_4BYTE_ADDR
#define DEFAULT_MEASUREMENT_TYPE_ANCHOR POS_APP_MEAS_RSS_SR_ANCHOR_4BYTE_ADDR
#endif

typedef uint16_t measurement_header_sequence_t;
typedef uint8_t measurement_header_type_t;
typedef uint8_t positioning_header_length_t;
typedef uint8_t measurement_payload_rss_t;

/**
    @brief defines payload address
*/
typedef struct
{
    uint8_t x[NODE_ADDRESS_LENGTH];
} measurement_payload_addr_t;

/**
    @brief defines the measurement voltage
*/
typedef uint16_t measurement_payload_voltage_t;

/**
    @brief stores the measurement data being sent out towards a sink.
*/
typedef struct
{
    measurement_payload_addr_t address;
    measurement_payload_rss_t value;
} measurement_rss_data_t;


/**
    @brief defines the application header
*/
typedef struct
{
    measurement_header_type_t  type;
    positioning_header_length_t  length;
} measurement_header_t;


/**
    @brief contains the WM beacon data obtained by the device.
*/
typedef struct
{
    app_addr_t address;
    int16_t  rss;
    int8_t   txpower;
    app_lib_time_timestamp_hp_t last_update;
} measurement_wm_beacon_t;

/**
    @brief buffer to help prepare the measurement payload.
*/
typedef struct
{
    uint8_t bytes[MAX_PAYLOAD];
    uint8_t len;
    uint8_t* ptr;
} measurement_payload_t;

/**
    @brief stores the beacon data.
*/
typedef struct
{
    measurement_wm_beacon_t beacons[MAX_BEACONS];
    uint8_t num_beacons;
    uint8_t min_index;
    int16_t min_rss;
} measurement_table_t;

void Measurements_init(void);
void Measurements_table_reset(void);
uint8_t Measurements_get_num_beacon(void);
void Measurements_clean_beacon(uint32_t older_than, uint8_t max_beacons);
void Measurements_insert_beacon(const app_lib_state_beacon_rx_t* beacon);
const uint8_t* Measurements_payload_init(void);
uint8_t Measurements_payload_length(void);

const uint8_t* Measurements_payload_add_rss(positioning_measurements_e
        meas_type);
void Measurements_payload_add_voltage(void);

#endif /*POS_MEASUREMENTS_H*/