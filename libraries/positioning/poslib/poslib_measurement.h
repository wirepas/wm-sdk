/**
* @file         poslib_measurement.h
* @brief        Header file for the poslib measurement.c
* @copyright    Wirepas Ltd. 2020
*/

#ifndef _POSLIB_MEASUREMENT_H_
#define _POSLIB_MEASUREMENT_H_

/**
* @brief  defines the device's positioning mode.
*         The type is matched against the WPE proto definition.
*/

typedef enum
{
    /** 0x00 - 0x6F : range reserved for Wirepas */
    POSLIB_MEAS_RSS_SR = 0x00,
    POSLIB_MEAS_RSS_HR = 0x01,
    POSLIB_MEAS_TIME = 0x02,
    POSLIB_MEAS_SPACE = 0x03,
    POSLIB_MEAS_VOLTAGE = 0x04,
    POSLIB_MEAS_RSS_SR_4BYTE_ADDR = 0x05,
    /** 0x70 - 0xEF : range reserved for customer
     *  Please notify Wirepas if a custom record ID is used by
     * your customised positoning app
     * 0xF0 - 0xFF : range reserved for Wirepas */
    POSLIB_MEAS_RSS_SR_ANCHOR = 0xF0,
    POSLIB_MEAS_RSS_SR_ANCHOR_4BYTE_ADDR = 0xF5
} poslib_measurements_e;

/** 2.4 profile - limited by internal memory */
#define MAX_PAYLOAD         102

#ifdef USE_3BYTE_NODE_ADDRESS
/** 4.x stack version */
/** limited by internal memory */
#define MAX_BEACONS         20
/** node address length in bytes */
#define NODE_ADDRESS_LENGTH 3
#define DEFAULT_MEASUREMENT_TYPE_TAG POSLIB_MEAS_RSS_SR
#define DEFAULT_MEASUREMENT_TYPE_ANCHOR POSLIB_MEAS_RSS_SR_ANCHOR
#else
/** 5.x stack version */
/** limited by internal memory */
#define MAX_BEACONS         16
/** node address length in bytes */
#define NODE_ADDRESS_LENGTH 4
#define DEFAULT_MEASUREMENT_TYPE_TAG POSLIB_MEAS_RSS_SR_4BYTE_ADDR
#define DEFAULT_MEASUREMENT_TYPE_ANCHOR POSLIB_MEAS_RSS_SR_ANCHOR_4BYTE_ADDR
#endif

typedef uint16_t poslib_meas_header_sequence_t;
typedef uint8_t poslib_meas_header_type_t;
typedef uint8_t poslib_header_length_t;
typedef uint8_t poslib_meas_payload_rss_t;
typedef struct { uint8_t x[NODE_ADDRESS_LENGTH]; } poslib_payload_addr_t;

/**
    @brief defines payload address
*/
typedef struct
{
    uint8_t x[NODE_ADDRESS_LENGTH];
} poslib_meas_payload_addr_t;

/**
    @brief defines the measurement voltage
*/
typedef uint16_t poslib_meas_payload_voltage_t;

/**
    @brief stores the measurement data being sent out towards a sink.
*/
typedef struct
{
    poslib_payload_addr_t address;
    poslib_meas_payload_rss_t value;
} poslib_meas_rss_data_t;


/**
    @brief defines the application header
*/
typedef struct
{
    poslib_meas_header_type_t  type;
    poslib_header_length_t  length;
} poslib_meas_header_t;

/**
 * @brief contains the WM beacon data obtained by the device.
*/
typedef struct
{
    app_addr_t address;
    int16_t  rss;
    int8_t   txpower;
    uint8_t  beacon_type;  // new in rel 5.0
    app_lib_time_timestamp_hp_t last_update;
} poslib_meas_wm_beacon_t;

/**
 * @brief buffer to help prepare the measurement payload.
 */
typedef struct
{
    uint8_t bytes[MAX_PAYLOAD];
    uint8_t len;
    uint8_t* ptr;
} poslib_meas_payload_t;

/**
 * @brief stores the beacon data.
 */
typedef struct
{
    poslib_meas_wm_beacon_t beacons[MAX_BEACONS];
    uint8_t num_beacons;
    uint8_t min_index;
    int16_t min_rss;
} poslib_meas_table_t;

/**
 * @brief   Starts new scan
 *
 * @param   Positioning_startScan  The poslib settings defined
 */
void PosLibMeas_startScan(poslib_settings_t * settings);

/**
 * @brief   Cleans up the beacon measurement table
 */
void PosLibMeas_initMeas(void);

/**
 * @brief   Clears the measurement table.
 */
void PosLibMeas_resetTable(void);

/**
 * @brief   Getter for number of beacons.
 * @return  Returns the number of beacons stored in the measurement table.
 */
uint8_t PosLibMeas_getNumofBeacons(void);

/**
 * @brief   Initialises the payload buffer for the next message.
 * @return  Pointer to the current edge of the payload
 */
const uint8_t * PosLibMeas_initPayload(void);

/**
 * @brief   Retreives the size of the buffer.
 * @return  The size of the useful payload.
 */
uint8_t PosLibMeas_getPayloadLen(void);

/**
 * @brief   Adds a RSS measurement to the payload buffer.
 * @param   meas_type  The measurement type.
 * @return  Pointer to the current edge of the payload.
 */
const uint8_t * PosLibMeas_addPayloadRss(poslib_measurements_e meas_type);

/**
 * @brief   Adds the voltage measurement to the payload buffer.
 */
void PosLibMeas_addVoltageToPayload(void);

/**
 * @brief   Removed shared libraries used callbacks.
 */
void PosLibMeas_removeCallbacks(void);
#endif
