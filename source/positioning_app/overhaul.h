/**
    @file overhaul.h
    @brief      Header file for overhaul.c
    @copyright  Wirepas Oy 2019
*/

#ifndef POS_OVERHAUL_H
#define POS_OVERHAUL_H

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "scheduler.h"

#define OVERHAUL_MAX_FIELD   25
#define TX_INTERVAL_MIN    1     // min 1 sec
#define TX_INTERVAL_MAX    86400  // max 24 hours

#define LEGACY_OVERHAUL_MSG_MEAS_RATE sizeof(uint16_t)  //don't change

/**
    @brief defines the available appconfig commands.
*/
typedef enum
{
    OVERHAUL_MSG_MEASUREMENT_RATE = 0x01,
    OVERHAUL_MSG_MODE = 0x02,
    OVERHAUL_MSG_OTAP = 0x04,
    OVERHAUL_MSG_BLEBEACON_OFFLINE_WAITTIME = 0x08,
    OVERHAUL_MSG_CLASS = 0x0A,
} overhaul_msg_e;

/**
    @brief overhauls unparsed element.
*/
typedef struct  __attribute__ ((packed))
{
    overhaul_msg_e type;
    uint8_t length;
    const uint8_t* ptr_payload; // 4 bytes
} overhaul_data_t; // sizeof = 6

/**
    @brief stores the fields to parse and act upon.
*/
typedef struct  __attribute__ ((packed))
{
    uint8_t num_fields;
    overhaul_data_t field[OVERHAUL_MAX_FIELD];

} overhaul_info_t;

/**
    @example Measurement_rate_command

    @brief

        Payload description:

            Type (1 byte) | Length (1 byte) | LE interval (2 bytes | 4 bytes)

            Type: 0x01
            Length: 0x02 or 0x04
            Interval:  word or double-word little-endian encoded

        Payload example:

            0x01 0x02 0x2C 0x01  : 300 sec update interval
            0x01 0x04 0x80 0x51 0x01 0x00 : 24 hours update interval
*/
typedef struct  __attribute__ ((packed))
{
    uint32_t interval; // change sleep/scan rate
} overhaul_msg_meas_rate_t;

/**
    @example Mode_command

    @brief

        Payload description:

            Type (1 byte) | Length (1 byte) | Mode (1 byte)

            Mode shall be selected according to the node role:
            Anchor: role headnode|sink  -> POS_APP_MODE_AUTOSCAN_ANCHOR
                                        -> POS_APP_MODE_OPPORTUNISTIC_ANCHOR
            Tag: role subnode   -> POS_APP_MODE_NRLS_TAG
                                -> POS_APP_MODE_AUTOSCAN_TAG
            Note that a role / mode combination other than the above will not be accepted

        Payload example:
            0x02 0x01 0xXX
*/
typedef struct  __attribute__ ((packed))
{
    positioning_mode_e value; // change between nrls or self scan

} overhaul_msg_mode_t;



/**
    @example Otap_command

    @brief

        Payload description:

            Type (1 byte) | Length (1 byte) | Scratchpad that should be in use (1 byte)

        Paylod example:

            0x04 0x02 0x00
*/
typedef struct  __attribute__ ((packed))
{
    app_lib_otap_seq_t target_sequence;
} overhaul_msg_otap_t;

/**
    @example blebeacon_offline_timeout_command

    @brief

        Payload description:

            Type (1 byte) | Length (1 byte) | LE waittime (2 bytes)

        Payload example:

            0x08 0x02 0x00 0x00  (Ble Beacon always off)

            0x08 0x02 0xFF 0xFF  (Ble Beacon forever on)

            0x08 0x02 0x3C 0x00  (Ble Beacon on when offline at a given period, eg 60s)
*/
typedef struct  __attribute__ ((packed))
{
    uint16_t waittime; // waittime before put BleBeacon on
} overhaul_msg_blebeacon_offline_waittime_t;



/**
    @example Class_command

    @brief

        Payload description:

            Type (1 byte) | Length (1 byte) | Scratchpad that should be in use (1 byte)

        Paylod example:

            0x04 0x02 0x00
*/
typedef struct  __attribute__ ((packed))
{
    positioning_class_e value; // class identifier
} overhaul_msg_class_t;




void Overhaul_init(void);

bool Overhaul_decode_config(const uint8_t* bytes,
                            uint32_t num_bytes,
                            uint8_t seq);

scheduler_state_e Overhaul_msg(void);

bool Overhaul_msg_measurement_rate(overhaul_msg_meas_rate_t* command,
                                   positioning_settings_t* app_settings);

bool Overhaul_msg_mode(overhaul_msg_mode_t* command,
                       positioning_settings_t* app_settings);


bool Overhaul_msg_otap(overhaul_msg_otap_t* command,
                       positioning_settings_t* app_settings);

bool Overhaul_msg_blebeacon_offline_waittime(
    overhaul_msg_blebeacon_offline_waittime_t* command,
    positioning_settings_t* app_settings);

bool Overhaul_msg_class(overhaul_msg_class_t* command,
                        positioning_settings_t* app_settings);

#endif /*POS_OVERHAUL_H*/
