/* Copyright 2017 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

#ifndef CSAP_FRAMES_H_
#define CSAP_FRAMES_H_

// Logical radio channel, value: RADIO_MIN_CHANNEL...
typedef uint8_t radio_channel_t;

/**
 * Network address width, acceptable value range 2-4.
 */
#define RADIO_ADDRESS_WIDTH 3u

typedef enum
{
    ATTR_RESET_OK                       = 0,
    ATTR_RESET_INVALID_STATE            = 1,
} attribute_reset_result_e;

/** CSAP attributes */
typedef enum
{
    /* Read / Write */
    CSAP_ATTR_NODE_ID = 1,
    CSAP_ATTR_NETWORK_ADDR = 2,
    CSAP_ATTR_NETWORK_CHANNEL = 3,
    CSAP_ATTR_NODE_ROLE = 4,
    CSAP_ATTR_CIPHER_KEY = 13,
    CSAP_ATTR_AUTHENTICATION_KEY = 14,
    CSAP_ATTR_OFFLINE_SCAN = 20,
    CSAP_ATTR_RESERVED_3 = 21,
    CSAP_ATTR_FEATURE_LOCK_BITS = 22,
    CSAP_ATTR_FEATURE_LOCK_KEY = 23,
    CSAP_ATTR_RESERVED_2 = 24,
    CSAP_ATTR_RESERVED_CHANNELS = 25,
    /* Read only */
    CSAP_ATTR_APP_MAXT_TRANS_UNIT = 5,
    CSAP_ATTR_PDU_BUFF_SIZE = 6,
    CSAP_ATTR_SCRATCHPAD_SEQ = 7,
    CSAP_ATTR_WAPS_VERSION = 8,
    CSAP_ATTR_FIRMWARE_MAJOR = 9,
    CSAP_ATTR_FIRMWARE_MINOR = 10,
    CSAP_ATTR_FIRMWARE_MAINTENANCE = 11,
    CSAP_ATTR_FIRMWARE_DEVELOPMENT = 12,
    CSAP_ATTR_CHANNEL_LIMITS = 15,
    CSAP_ATTR_APPCFG_MAX_SIZE = 16,
    CSAP_ATTR_HWMAGIC = 17,
    CSAP_ATTR_STACK_PROFILE = 18,
    CSAP_ATTR_RESERVED_1 = 19,
} csap_attr_e;

/** CSAP attributes lengths */
typedef enum
{
    CSAP_ATTR_NODE_ID_SIZE = sizeof(w_addr_t),
    CSAP_ATTR_NETWORK_CHANNEL_SIZE = sizeof(radio_channel_t),
    CSAP_ATTR_NETWORK_ADDRESS_SIZE = RADIO_ADDRESS_WIDTH,
    CSAP_ATTR_NODE_ROLE_SIZE = 1,
    CSAP_ATTR_APDU_SIZE_SIZE = 1,
    CSAP_ATTR_PDU_BUFF_SIZE_SIZE = 1,
    CSAP_ATTR_SCRATCHPAD_SEQ_SIZE = sizeof(otap_seq_t),
    CSAP_ATTR_WAPS_VERSION_SIZE = 2,
    CSAP_ATTR_FIRMWARE_MAJOR_SIZE = 2,
    CSAP_ATTR_FIRMWARE_MINOR_SIZE = 2,
    CSAP_ATTR_FIRMWARE_MAINTENANCE_SIZE = 2,
    CSAP_ATTR_FIRMWARE_DEVELOPMENT_SIZE = 2,
    CSAP_ATTR_CIPHER_KEY_SIZE = 16,
    CSAP_ATTR_AUTHENTICATION_KEY_SIZE = 16,
    CSAP_ATTR_CHANNEL_LIMIT_SIZE = 2,
    CSAP_ATTR_APPCFG_MAX_SIZE_SIZE = 1,
    CSAP_ATTR_HWMAGIC_SIZE = 2,
    CSAP_ATTR_STACK_PROFILE_SIZE = 2,
    CSAP_ATTR_OFFLINE_SCAN_SIZE = 2,
    CSAP_ATTR_RESERVED_3_SIZE = 0,
    CSAP_ATTR_FEATURE_LOCK_BITS_SIZE = 4,
    CSAP_ATTR_FEATURE_LOCK_KEY_SIZE = 16,
    CSAP_ATTR_RESERVED_CHANNELS_SIZE = 0,   /* Variable size */
    CSAP_ATTR_RESERVED_1_SIZE = 0,
    CSAP_ATTR_RESERVED_2_SIZE = 0,
} csap_attr_size_e;


typedef struct __attribute__ ((__packed__))
{
    /**
     * The purpose of the key is to make it a bit harder to
     * accidentally reset the stored values.
     */
    uint32_t    reset_key;
} csap_reset_req_t;

typedef union
{
    csap_reset_req_t    reset_req;
} frame_csap;

typedef enum
{
    // Expand to include the "DoIt" key
    CSAP_RESET_OK               = ATTR_RESET_OK,
    CSAP_RESET_INVALID_STATE    = ATTR_RESET_INVALID_STATE,
    CSAP_RESET_INVALID_KEY      = 2,
    CSAP_RESET_ACCESS_DENIED    = 3
} csap_reset_e;

#endif /* CSAP_FRAMES_H_ */
