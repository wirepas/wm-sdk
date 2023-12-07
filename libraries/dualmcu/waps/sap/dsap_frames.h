/* Copyright 2017 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

#ifndef DSAP_FRAMES_H_
#define DSAP_FRAMES_H_

/** Result of transmitted packet */
typedef enum
{
    DSAP_TX_SUCCESS = 0,
    DSAP_TX_STACK_STOPPED = 1,
    DSAP_TX_INV_QOS_PARAM = 2,
    DSAP_TX_INV_OPTS_PARAM = 3,
    DSAP_TX_OUT_OF_MEMORY = 4,
    DSAP_TX_UNKNOWN_DST = 5, // Not used as of version 5.0
    DSAP_TX_INV_LEN = 6,
    DSAP_TX_IND_FULL = 7,
    DSAP_TX_INV_PDU_ID = 8,
    DSAP_TX_RESV_EP = 9,
    DSAP_TX_ACCESS_DENIED = 10,
} dsap_tx_result_e;

typedef enum
{
    DSAP_IND_SUCCESS = 0,
    DSAP_IND_TIMEOUT = 1,
} dsap_indication_e;

/** TX options of WAPS_DSAP_DATA.request */
typedef enum
{
    TX_OPTS_NO_IND_REQ = 0, /** No indication req when pkt sent */
    TX_OPTS_IND_REQ = 1,    /** Indication req */
    TX_OPTS_UNACK_CSMA_CA = 2,    /** Unack CSMA-CA packet */
    TX_OPTS_HOPLIMIT_MASK = 0x3c, /** Hop limit mask */
} dsap_tx_options_e;

// Offset for hop limit
#define TX_OPTS_HOPLIMIT_OFFSET 2

/** QOS classes of WAPS_DSAP_DATA.request */
typedef enum
{
    DSAP_QOS_NORMAL = 0,    /**< Normal priority packet */
    DSAP_QOS_HIGH = 1,      /**< High priority packet */
    DSAP_QOS_UNACKED = 2    /**< Unacknowledged packet */
} dsap_qos_e;

/** Maximum size of transmittable data APDU via WAPS/DSAP interface */
#define APDU_MAX_SIZE   102

/* WAPS-DSAP-DATA_TX-REQUEST */

typedef struct __attribute__ ((__packed__))
{
    pduid_t     apdu_id;
    ep_t        src_endpoint;
    w_addr_t    dst_addr;
    ep_t        dst_endpoint;
    uint8_t     qos;
    uint8_t     tx_opts;
    uint8_t     apdu_len;
    uint8_t     apdu[APDU_MAX_SIZE];
} dsap_data_tx_req_t;

#define FRAME_DSAP_DATA_TX_REQ_HEADER_SIZE  \
    (sizeof(dsap_data_tx_req_t) - APDU_MAX_SIZE)

/* WAPS-DSAP-DATA_TX_TT-REQUEST */

typedef struct __attribute__ ((__packed__))
{
    pduid_t     apdu_id;
    ep_t        src_endpoint;
    w_addr_t    dst_addr;
    ep_t        dst_endpoint;
    uint8_t     qos;
    uint8_t     tx_opts;
    uint32_t    reserved;
    uint8_t     apdu_len;
    uint8_t     apdu[APDU_MAX_SIZE];
} dsap_data_tx_tt_req_t;

#define FRAME_DSAP_DATA_TX_TT_REQ_HEADER_SIZE  \
    (sizeof(dsap_data_tx_tt_req_t) - APDU_MAX_SIZE)

typedef struct __attribute__ ((__packed__))
{
    pduid_t     apdu_id;
    ep_t        src_endpoint;
    w_addr_t    dst_addr;
    ep_t        dst_endpoint;
    uint8_t     qos;
    uint8_t     tx_opts;
    uint32_t    reserved;
    uint16_t    full_packet_id : 12;
    // Fragment offset + flag
    uint16_t    fragment_offset_flag;
    uint8_t     apdu_len;
    uint8_t     apdu[APDU_MAX_SIZE];
} dsap_data_tx_frag_req_t;

#define FRAME_DSAP_DATA_TX_FRAG_REQ_HEADER_SIZE  \
    (sizeof(dsap_data_tx_frag_req_t) - APDU_MAX_SIZE)
/* WAPS-DSAP-DATA_RX-INDICATION */

// info field masks and offsets

// Qos: Lowest 2 bits
#define RX_IND_INFO_QOS_MASK 0x3
#define RX_IND_INFO_QOS_OFFSET 0
// Hop count, 6 next LSB bits
#define RX_IND_INFO_HOPCOUNT_MASK 0xfc
#define RX_IND_INFO_HOPCOUNT_OFFSET 2
#define RX_IND_INFO_MAX_HOPCOUNT \
    (RX_IND_INFO_HOPCOUNT_MASK >> RX_IND_INFO_HOPCOUNT_OFFSET)

// fragment_offset_flag field access

// Fragment offset: Lowest 12 bits
#define DSAP_FRAG_LENGTH_MASK 0x0fff

// Last fragment: Highest bit
#define DSAP_FRAG_LAST_FLAG_MASK 0x8000

typedef struct __attribute__ ((__packed__))
{
    uint8_t     queued_indications;
    w_addr_t    src_addr;
    ep_t        src_endpoint;
    w_addr_t    dst_addr;
    ep_t        dst_endpoint;
    // Qos + hop count
    uint8_t     info;
    uint32_t    delay;
    uint8_t     apdu_len;
    uint8_t     apdu[APDU_MAX_SIZE];
} dsap_data_rx_ind_t;

#define FRAME_DSAP_DATA_RX_IND_HEADER_SIZE  \
    (sizeof(dsap_data_rx_ind_t) - APDU_MAX_SIZE)


/* WAPS-DSAP-DATA_RX_FRAG-INDICATION */

typedef struct __attribute__ ((__packed__))
{
    uint8_t     queued_indications;
    w_addr_t    src_addr;
    ep_t        src_endpoint;
    w_addr_t    dst_addr;
    ep_t        dst_endpoint;
    // Qos + hop count
    uint8_t     info;
    uint32_t    delay;
    uint16_t    full_packet_id;
    // Fragment offset + flag
    uint16_t    fragment_offset_flag;
    uint8_t     apdu_len;
    uint8_t     apdu[APDU_MAX_SIZE];
} dsap_data_rx_frag_ind_t;

#define FRAME_DSAP_DATA_RX_FRAG_IND_HEADER_SIZE  \
    (sizeof(dsap_data_rx_frag_ind_t) - APDU_MAX_SIZE)

/* WAPS-DSAP-DATA_TX-INDICATION */

typedef struct __attribute__ ((__packed__))
{
    uint8_t     queued_indications;
    pduid_t     apdu_id;
    ep_t        src_endpoint;
    w_addr_t    dst_addr;
    ep_t        dst_endpoint;
    uint32_t    queue_delay;
    uint8_t     result;
} dsap_data_tx_ind_t;

/* WAPS-DSAP-DATA_TX-CONFIRMATION */

typedef struct __attribute__ ((__packed__))
{
    pduid_t     apdu_id;
    uint8_t     result;
    uint8_t     buff_cap;
} dsap_data_tx_cnf_t;

typedef union
{
    dsap_data_tx_req_t          data_tx_req;
    dsap_data_tx_tt_req_t       data_tx_tt_req;
    dsap_data_tx_frag_req_t     data_tx_frag_req;
    dsap_data_tx_ind_t          data_tx_ind;
    dsap_data_tx_cnf_t          data_tx_cnf;
    dsap_data_rx_ind_t          data_rx_ind;
    dsap_data_rx_frag_ind_t     data_rx_frag_ind;
} frame_dsap;


#endif /* DSAP_FRAMES_H_ */
