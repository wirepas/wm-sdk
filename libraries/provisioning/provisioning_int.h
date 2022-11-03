/* Copyright 2019 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */
#ifndef _PROVISIONING_INT_H_
#define _PROVISIONING_INT_H_

#include <stdint.h>
#include "api.h"
#include "aessw.h"

/* Wirepas Provisioning Protocol endpoints :
 *  - Downlink: Source: 255 ; Destination: 246
 *  - Uplink: Source: 246 ; Destination: 255
 */
/** Provisioning downlink endpoint. */
#define PROV_DOWNLINK_EP    255
/** Provisioning uplink endpoint. */
#define PROV_UPLINK_EP   246

/** Maximum size of a packet payload. Needed for buffer size definitions. */
#define PROV_PDU_SIZE 102

/** Size of the counter used in AES CTR mode encryption. */
#define PROV_CTR_SIZE 2

/** Size of MIC in DATA packet if secured method is used. */
#define PROV_MIC_SIZE 5

/** Encryption key offset in Key buffer. */
#define ENC_KEY_OFFSET 16

/** Offset of the data in the provisioning DATA packet.
 *  pdu_prov_hdr(6) + key_index(1) + counter(2)
 */
#define PROV_DATA_OFFSET (sizeof(pdu_prov_hdr_t) + 1 + PROV_CTR_SIZE)

/** Minimum User Specific Id in Cbor provisioning buffer (range [128:255]). */
#define PROV_DATA_MIN_USER_ID 128

/** Maximum User Specific Id in Cbor provisioning buffer (range [128:255]). */
#define PROV_DATA_MAX_USER_ID 255

/** Network address for sending and receiving joining beacons */
#define JOINING_NETWORK_ADDRESS 0x89d3b8 // "JBTX"

/** Network channel for sending and receiving joining beacons */
#define JOINING_NETWORK_CHANNEL 4

/** Time to scan for joining beacons, in milliseconds.
 *  \ref JOINING_RX_TIMEOUT and \ref JOINING_TX_INTERVAL needs to
 *  be modified accordingly.
 */
#define JOINING_RX_TIMEOUT      600

/** Interval to send joining beacons, in milliseconds */
#define JOINING_TX_INTERVAL     500

/** A value for the joining type field., Allows to identify provisioning
 *  proxy nodes.
 */
#define JOINING_BEACON_TYPE 0x647650fb // "PROVIS"

/** \brief List of Wirepas Ids for CBOR encoded provisioning data. */
typedef enum
{
    PROV_DATA_ID_ENC_KEY = 0,
    PROV_DATA_ID_AUTH_KEY = 1,
    PROV_DATA_ID_NET_ADDR = 2,
    PROV_DATA_ID_NET_CHAN = 3,
    PROV_DATA_ID_NODE_ADDR = 4,
    PROV_DATA_ID_NODE_ROLE = 5
} provisioning_data_ids_e;

typedef enum
{
    PROV_PACKET_TYPE_START = 1,
    PROV_PACKET_TYPE_DATA = 2,
    PROV_PACKET_TYPE_DATA_ACK = 3,
    PROV_PACKET_TYPE_NACK = 4
} prov_packet_type_e;

typedef enum
{
    PROV_NACK_TYPE_NOT_AUTHORIZED = 0,
    PROV_NACK_TYPE_METHOD_NOT_SUPPORTED = 1,
    PROV_NACK_TYPE_INVALID_DATA = 2,
    PROV_NACK_TYPE_INVALID_KEY_IDX = 3
} prov_nack_type_e;

typedef struct __attribute__((__packed__))
{
    uint8_t  type;
    app_addr_t address;
    uint8_t  session_id;
}pdu_prov_hdr_t;

typedef struct __attribute__((__packed__))
{
    pdu_prov_hdr_t  pdu_header;
    uint8_t         method;
    uint8_t         iv[AES_128_KEY_BLOCK_SIZE];
    uint8_t         uid[PROV_PDU_SIZE -
                        sizeof(pdu_prov_hdr_t) - 1 - AES_128_KEY_BLOCK_SIZE];
}pdu_prov_start_t;

typedef struct __attribute__((__packed__))
{
    pdu_prov_hdr_t  pdu_header;
    uint8_t         key_index;
    uint16_t        counter;
    uint8_t         data[PROV_PDU_SIZE - PROV_DATA_OFFSET - PROV_MIC_SIZE];
    /* Mic is added at the end of the data packet.
     * uint8_t         mic[PROV_MIC_SIZE];
     */
}pdu_prov_data_t;

typedef struct __attribute__((__packed__))
{
    pdu_prov_hdr_t  pdu_header;
}pdu_prov_data_ack_t;

typedef struct __attribute__((__packed__))
{
    pdu_prov_hdr_t  pdu_header;
    uint8_t         nack_type;
}pdu_prov_nack_t;

typedef union __attribute__((__packed__))
{
    union __attribute__((__packed__))
    {
        pdu_prov_hdr_t  pdu_header;
        uint8_t         pld[PROV_PDU_SIZE - sizeof(pdu_prov_hdr_t)];
    };
    pdu_prov_start_t start;
    pdu_prov_data_t data;
    pdu_prov_data_ack_t data_ack;
    pdu_prov_nack_t nack;
} pdu_prov_t;

/**
 * \brief This structure holds the provisioning data parameters.
 */
typedef struct
{
    /** End provisioning callback. */
    provisioning_end_cb_f end_cb;
    /** Data provisioning callback. */
    provisioning_user_data_cb_f user_data_cb;
    /** buffer containing provisioning data. */
    uint8_t * buffer;
    /** length of provisioning buffer. */
    uint8_t length;
} provisioning_data_conf_t;

/**
 * \brief   The end joining callback. This function is called at the end of
 *          the joining process.
 * \param   result
 *          Result of the joining process.
 */
typedef void (*provisioning_joining_end_cb_f)(provisioning_res_e result);

/**
 * \brief This structure holds the joining node parameters.
 */
typedef struct
{
    /** Callback used to select which beacon to connect to. */
    provisioning_joining_beacon_cb_f joining_cb;
    /** End joining callback. */
    provisioning_joining_end_cb_f end_cb;
    /** How many retries are allowed to connect to a network. */
    uint8_t nb_retry;
} provisioning_joining_conf_t;

/**
 * \brief   Decode (and apply if valid) the received provisioning data.
 * \param   conf
 *          Configuration for the provisioning data decoder.
 * \param   dry_run
 *          If true, only check data validity and don't apply it.
 * \return  Result code, \ref PROV_RET_OK if config is valid.
 *          See \ref provisioning_ret_e for other return codes.
 */
provisioning_ret_e Provisioning_Data_decode(provisioning_data_conf_t * conf,
                                            bool dry_run);

/**
 * \brief   Initialize the Joining module.
 * \note    If Joining is used, App_scheduler MUST BE initialized in App_Init
 *          of the application. Also \ref startJoiningBeaconRx function
 *          offered by Joining library and \ref setRouteCb function offered by
 *          State library MUST NOT be used outside of this module.
 * \note    Joining module needs 2 tasks from App_scheduler.
 * \param   conf
 *          Configuration for the Joining module.
 * \return  Result code, \ref PROV_RET_OK if ok. See \ref provisioning_ret_e
 *          for other return codes.
 */
provisioning_ret_e Provisioning_Joining_init(
                                        provisioning_joining_conf_t * conf);

/**
 * \brief   Start the joining process (listen to joining beacon and select
 *          a network to join).
 * \return  Result code, \ref PROV_RET_OK if joining has started.
 *          See \ref provisioning_ret_e for other return codes.
 */
provisioning_ret_e Provisioning_Joining_start(void);

/**
 * \brief   Stops the joining process.
 * \note    All taken callback and tasks are freed.
 * \note    Stop is not immediate, run_state_machine function must be
 *          scheduled first (typ. 100ms).
 * \return  Result code, \ref PROV_RET_OK if joining has stopped.
 *          See \ref provisioning_ret_e for other return codes.
 */
provisioning_ret_e Provisioning_Joining_stop(void);

#endif //_PROVISIONING_INT_H_
