/**
* @file       poslib_decode.h
* @brief      Header file for poslib_decode.c
* @copyright  Wirepas Ltd. 2020
*/

#ifndef _POSLIB_DECODE_H_
#define _POSLIB_DECODE_H_

#define DECODE_MAX_FIELD   25
/** min 1 sec */
#define TX_INTERVAL_MIN      1
/** max 24 hours */
#define TX_INTERVAL_MAX      86400

/** do not change */
#define LEGACY_DECODE_MSG_MEAS_RATE sizeof(uint16_t)

/**
* @brief defines the available appconfig commands.
*/
typedef enum
{
    POSLIB_DECODE_MSG_MEASUREMENT_RATE = 0x01,
    POSLIB_DECODE_MSG_MODE = 0x02,
    POSLIB_DECODE_MSG_OTAP = 0x04,
    POSLIB_DECODE_MSG_BLEBEACON_OFFLINE_WAITTIME = 0x08,
    POSLIB_DECODE_MSG_CLASS = 0x0A,
} poslib_decode_msg_e;

/**
* @brief overhauls unparsed element.
*/
typedef struct  __attribute__ ((packed))
{
    poslib_decode_msg_e type;
    uint8_t length;
    const uint8_t* ptr_payload; // 4 bytes
} poslib_decode_data_t; // sizeof = 6

/**
* @brief stores the fields to parse and act upon.
*/
typedef struct  __attribute__ ((packed))
{
    uint8_t num_fields;
    poslib_decode_data_t field[DECODE_MAX_FIELD];
} poslib_decode_info_t;

/**
* @example   Measurement_rate_command
*
* @brief
*
*            Payload description:
*
*            Type (1 byte) | Length (1 byte) | LE interval (2 bytes | 4 bytes)
*
*            Type: 0x01
*            Length: 0x02 or 0x04
*            Interval:  word or double-word little-endian encoded
*
*            Payload example:
*            0x01 0x02 0x2C 0x01  : 300 sec update interval
*            0x01 0x04 0x80 0x51 0x01 0x00 : 24 hours update interval
*/
typedef struct  __attribute__ ((packed))
{
    uint32_t interval; // change sleep/scan rate
} poslib_decode_msg_meas_rate_t;

/**
* @example   Mode_command
*
* @brief
*
*            Payload description:
*
*            Type (1 byte) | Length (1 byte) | Mode (1 byte)
*
*            Mode shall be selected according to the node role:
*            Anchor: role headnode|sink  -> POS_APP_MODE_AUTOSCAN_ANCHOR
*                                        -> POS_APP_MODE_OPPORTUNISTIC_ANCHOR
*            Tag: role subnode   -> POS_APP_MODE_NRLS_TAG
*                                -> POS_APP_MODE_AUTOSCAN_TAG
*            Note that a role / mode combination other than the above will not be accepted
*
*            Payload example:
*            0x02 0x01 0xXX
*/
typedef struct  __attribute__ ((packed))
{
    poslib_mode_e value; // change between nrls or self scan

} poslib_decode_msg_mode_t;

/**
* @example   Otap_command
*
* @brief
*
*            Payload description:
*            Type (1 byte) | Length (1 byte) | Scratchpad that should be in use (1 byte)
*
*            Paylod example:
*            0x04 0x02 0x00
*/
typedef struct  __attribute__ ((packed))
{
    app_lib_otap_seq_t target_sequence;
} poslib_decode_msg_otap_t;

/**
* @example   blebeacon_offline_timeout_command
*
* @brief
*
*            Payload description:
*            Type (1 byte) | Length (1 byte) | LE waittime (2 bytes)
*
*            Payload example:
*            0x08 0x02 0x00 0x00  (Ble Beacon always off)
*            0x08 0x02 0xFF 0xFF  (Ble Beacon forever on)
*            0x08 0x02 0x3C 0x00  (Ble Beacon on when offline at a given period, eg 60s)
*/
typedef struct  __attribute__ ((packed))
{
    uint16_t waittime; // waittime before put BleBeacon on
} poslib_decode_msg_blebeacon_offline_waittime_t;

/**
* @example   Class_command
*
* @brief
*
*            Payload description:
*            Type (1 byte) | Length (1 byte) | Scratchpad that should be in use (1 byte)
*
*            Paylod example:
*            0x04 0x02 0x00
*/
typedef struct  __attribute__ ((packed))
{
    poslib_class_e value; // class identifier
} poslib_decode_msg_class_t;

/**
 * @brief   Initializes overhaul functions.
 */
void PosLibDecode_init(void);

/**
 * @brief   Decodes received configuration.
 * @param   bytes type of const uint8_t *
 * @param   num_bytes type of uint32_t
 * @return  bool - true if received app_config is valid
 */
bool PosLibDecode_config(const uint8_t * bytes, uint32_t num_bytes);

/**
 * @brief   Acts on the commands provided in the configuration payload.
 */
void PosLibDecode_msg(void);
#endif
