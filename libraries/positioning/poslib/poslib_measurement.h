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
    POSLIB_MEAS_NODE_INFO = 0x06,
    POSLIB_MEAS_DA = 0x07,
    /** 0x70 - 0xEF : range reserved for customer
     *  Please notify Wirepas if a custom record ID is used by
     * your customised positoning app
     * 0xF0 - 0xFF : range reserved for Wirepas */
    POSLIB_MEAS_RSS_SR_ANCHOR = 0xF0,
    POSLIB_MEAS_RSS_SR_ANCHOR_4BYTE_ADDR = 0xF5
} poslib_measurements_e;

/** 2.4 profile - limited by internal memory */
#define MAX_PAYLOAD         102

/** limited by maximum payload size */
#define MAX_BEACONS         14
/** node address length in bytes */
#define NODE_ADDRESS_LENGTH 4
#define DEFAULT_MEASUREMENT_TYPE_TAG POSLIB_MEAS_RSS_SR_4BYTE_ADDR
#define DEFAULT_MEASUREMENT_TYPE_ANCHOR POSLIB_MEAS_RSS_SR_ANCHOR_4BYTE_ADDR


/** Definitions of positioning measurement message. !Note all structure definitions shall be packed */
#define PACKED_STRUCT struct __attribute__((__packed__)) 

typedef PACKED_STRUCT
{
    uint16_t sequence;
} poslib_meas_message_header_t;

/**
    @brief RSS measurement structure
*/
typedef PACKED_STRUCT
{
    uint32_t address;
    uint8_t norm_rss;
} poslib_meas_rss_data_t;

/**
    @brief Measurement record header
*/
typedef PACKED_STRUCT
{
    uint8_t type;
    uint8_t length;
} poslib_meas_record_header_t;

typedef PACKED_STRUCT
{
    poslib_meas_record_header_t header;
    uint32_t node_addr;
} poslib_meas_record_da_t;

/**
    @brief Voltage record (header + payload)
*/
typedef PACKED_STRUCT
{
    poslib_meas_record_header_t header;
    uint16_t voltage;
} poslib_meas_record_voltage_t;

/** Defines the feature flags version, to be incremented when new flags are added */
#define  POSLIB_NODE_INFO_FEATURES_VERSION 1
/**
    @brief Node active features ( @note this is a bit )
*/
typedef enum 
{
    /**< Features flags version mask */
    POSLIB_NODE_INFO_MASK_VERSION = 0x0000000F,
    /**< Motion enabled - valid from ver 0 */
    POSLIB_NODE_INFO_FLAG_MOTION_EN = 0x00000010,
    /**< node was static since last measurement - valid from ver 0 */
    POSLIB_NODE_INFO_FLAG_IS_STATIC = 0x000000020,
    /**< Eddystone BLE beacon active - valid from ver 0 */
    POSLIB_NODE_INFO_FLAG_EDDYSTONE_ON = 0x00000040,
    /**< iBeacon BLE beacon active - valid from ver 0 */
    POSLIB_NODE_INFO_FLAG_IBEACON_ON = 0x00000080,
    /**< Mini-beacon active - valid from ver 1 */
    POSLIB_NODE_INFO_FLAG_MBCN_ON = 0x00000100,
} poslib_node_info_features_e;

/**
    @brief Node info record
*/
typedef PACKED_STRUCT
{
    /**< next expected update */
    uint32_t update_s;
    /**< node active features */   
    uint32_t features;
    /**< node positioning mode */      
    uint8_t node_mode;
    /**< node positioning class */      
    uint8_t node_class;
} poslib_meas_record_node_info_t;

typedef enum
{
    SCAN_MODE_STANDARD = 1,
} poslib_scan_mode_e;

typedef struct poslib_measurement
{
   poslib_scan_mode_e mode;
   uint32_t max_duration_ms; //maximum scan duration [ms], (if 0 then scan default time is used)
} poslib_scan_ctrl_t;


/**
 * @brief   Starts new scan
 *
 * @param   Positioning_startScan  The poslib settings defined
 */
bool PosLibMeas_startScan(poslib_scan_ctrl_t * scan_ctrl);

bool PosLibMeas_opportunisticScan(bool enable);

/**
 * @brief   Get the number of beacons available. @note This shall be called after scan end.
 * @return  Returns the number of beacons stored in the measurement table.
 */
uint8_t PosLibMeas_getBeaconNum(void);

/**
 * @brief   Stops the measurement module. Callbacks are removed.
 */
void PosLibMeas_stop(void);

bool PosLibMeas_getPayload(uint8_t * bytes, uint8_t max_len, uint8_t sequence,
                                poslib_measurements_e meas_type, bool add_voltage, 
                                poslib_meas_record_node_info_t * node_info,
                                uint8_t * bytes_len, uint8_t * num_meas);

/**
 * @brief Clears measurement table.
 * @param void
 * @return void
 */
void PosLibMeas_clearMeas(void);
#endif
