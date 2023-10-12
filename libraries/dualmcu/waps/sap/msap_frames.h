/* Copyright 2017 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

#ifndef MSAP_FRAMES_H_
#define MSAP_FRAMES_H_

#include <stdbool.h>
#include "multicast.h"

/**
 * \brief   App config max size.
 *          The define is needed for compiling
 *          time size reservations.
 */
#define APP_CONF_MAX    80

/**
 * \brief   Type for OTAP sequence number
 *
 *          Do not confuse this with actual firmware version numbers.
 */
typedef uint8_t otap_seq_t;

/**
 * \file    msap_frames.h
 *          Values related to the Management Service Access Point (MSAP)
 */

/** How many neighbors in single query */
#define MSAP_MAX_NBORS  8

/** Mask for MSAP_AUTOSTART */
#define MSAP_AUTOSTART  1

/**
 * \brief   MSAP nbor type
 * \note    Most reliable information is always from next hop and members
 *          Other entries might be very old
 */
typedef enum
{
    /** Neighbor is specifically a next hop cluster */
    NEIGHBOR_IS_NEXT_HOP,
    /** Neighbor is specifically a member of this node */
    NEIGHBOR_IS_MEMBER,
    /** Neighbor is synced or cluster from network scan */
    NEIGHBOR_IS_CLUSTER,
} msap_neighbor_type_e;

/** MSAP neighbor structure */
typedef struct __attribute__ ((__packed__))
{
    /** Neighbor ID */
    w_addr_t    addr;
    /** Link reliability for this node (for next hop) */
    uint8_t     link_rel;
    /** Normalized RSSI */
    int8_t      rssi_norm;
    /** TC 0 cost */
    uint8_t     cost_0;
    /** Cluster channel index */
    uint8_t     channel;
    /** Neighbor type \see msap_neighbor_type_e */
    uint8_t     type;
    /** Used power in dBm for transmissions (for next hop) */
    int8_t      tx_power;
    /** Received power in dBm (for next hop) */
    int8_t      rx_power;
    /** Time since last update time in seconds */
    uint16_t    last_update;
} msap_neighbor_entry_t;

/* MSAP attributes */

typedef enum
{
    /* Read only */
    MSAP_ATTR_STACK_STATUS = 1,
    MSAP_ATTR_PDU_BUFF_USAGE = 2,
    MSAP_ATTR_PDU_BUFF_CAPA = 3,
    MSAP_ATTR_NBOR_COUNT = 4,
    MSAP_ATTR_ROUTE_COUNT = 7,
    MSAP_ATTR_SYSTEM_TIME = 8,
    MSAP_ATTR_AC_LIMITS = 10,
    MSAP_ATTR_CURRENT_AC = 11,
    MSAP_ATTR_SCRATCHPAD_BLOCK_MAX = 12,
    MSAP_ATTR_SCRATCHPAD_NUM_BYTES = 14,
    /* Read / Write */
    MSAP_ATTR_RESERVED_1 = 5, /* Old MSAP_ATTR_ENERGY */
    MSAP_ATTR_AUTOSTART = 6,
    MSAP_ATTR_AC_RANGE = 9,
    MSAP_ATTR_MCAST_GROUPS = 13,
} msap_attr_e;

/* MSAP attributes lengths */
typedef enum
{
    MSAP_ATTR_STACK_STATUS_SIZE = 1,
    MSAP_ATTR_PDU_BUFF_USAGE_SIZE = 1,
    MSAP_ATTR_PDU_BUFF_CAP_SIZE = 1,
    MSAP_ATTR_NBOR_COUNT_SIZE = 1,
    MSAP_ATTR_RESERVED_1_SIZE = 1,
    MSAP_ATTR_AUTOSTART_SIZE = 1,
    MSAP_ATTR_ROUTE_COUNT_SIZE = 1,
    MSAP_ATTR_SYSTEM_TIME_SIZE = 4,
    MSAP_ATTR_AC_RANGE_SIZE = 4,
    MSAP_ATTR_AC_LIMITS_SIZE = 4,
    MSAP_ATTR_CURRENT_AC_SIZE = 2,
    MSAP_ATTR_SCRATCHPAD_BLOCK_MAX_SIZE = 1,
    MSAP_ATTR_MCAST_GROUPS_SIZE = \
        MULTICAST_ADDRESS_AMOUNT * sizeof(w_addr_t),
    MSAP_ATTR_SCRATCHPAD_NUM_BYTES_SIZE = 4,
} msap_attr_size_e;

/* FUNC_WAPS_MSAP_STACK_START_REQUEST */

typedef struct __attribute__ ((__packed__))
{
    uint8_t     start_options;
} msap_start_req_t;

typedef struct __attribute__ ((__packed__))
{
    uint8_t queued_indications;
    uint8_t result;
} msap_state_ind_t;

/* FUNC_WAPS_MSAP_STACK_START_CONFIRMATION */
/* FUNC_WAPS_MSAP_STACK_STOP_CONFIRMATION  */

typedef struct __attribute__ ((__packed__))
{
    uint8_t             queued_indications;
    uint8_t             seq;
    uint16_t            interval;
    uint8_t             config[APP_CONF_MAX];
} msap_int_ind_t;

typedef struct __attribute__ ((__packed__))
{
    /** Base cost to set: 0...255. Value is hard-capped at 254 */
    uint8_t base_cost;
} msap_sink_cost_write_req_t;

typedef struct __attribute__ ((__packed__))
{
    /** Read result: \see msap_sink_cost_e */
    uint8_t result;
    /** Base cost read out: will read as 0...254 */
    uint8_t base_cost;
} msap_sink_cost_read_cnf_t;

typedef struct __attribute__ ((__packed__))
{
    /** Seq number of this config */
    uint8_t             seq;
    /** send interval */
    uint16_t            interval;
    /** App Config */
    uint8_t             config[APP_CONF_MAX];
} msap_int_write_req_t;

typedef struct __attribute__ ((__packed__))
{
    /** Read result */
    uint8_t             result;
    /** Other fields: \see msap_int_write_req_t */
    uint8_t             seq;
    uint16_t            interval;
    uint8_t             config[APP_CONF_MAX];
} msap_int_read_cnf_t;

typedef struct __attribute__ ((__packed__))
{
    uint8_t         queued;
} msap_ind_poll_cnf_t;

/** Result of MSAP-SCRATCHPAD_CLEAR request */
typedef enum
{
    /** Scratchpad cleared successfully */
    MSAP_SCRATCHPAD_CLEAR_SUCCESS = 0,
    /** Stack in invalid state */
    MSAP_SCRATCHPAD_CLEAR_INVALID_STATE = 1,
    /** Access denied due to feature lock bits */
    MSAP_SCRATCHPAD_CLEAR_ACCESS_DENIED = 2,
} msap_scratchpad_clear_e;

/** MSAP-SCRATCHPAD_START request frame */
typedef struct __attribute__ ((__packed__))
{
    /** Total number of bytes of data */
    uint32_t    num_bytes;
    /** Sequence number of the scratchpad */
    otap_seq_t  seq;
} msap_scratchpad_start_req_t;

/** Result of MSAP-SCRATCHPAD_START request */
typedef enum
{
    /** Scratchpad write started successfully */
    MSAP_SCRATCHPAD_START_SUCCESS = 0,
    /** Stack in invalid state */
    MSAP_SCRATCHPAD_START_INVALID_STATE = 1,
    /** Invalid \ref msap_scratchpad_start_req_t::num_bytes */
    MSAP_SCRATCHPAD_START_INVALID_NUM_BYTES = 2,
    /** Invalid \ref msap_scratchpad_start_req_t::seq (not used anymore) */
    MSAP_SCRATCHPAD_START_INVALID_SEQ = 3,
    /** Access denied due to feature lock bits */
    MSAP_SCRATCHPAD_START_ACCESS_DENIED = 4,
} msap_scratchpad_start_e;

/** Maximum number of bytes in a single scratchpad block */
#define MSAP_SCRATCHPAD_BLOCK_MAX_NUM_BYTES 112

/** MSAP-SCRATCHPAD_BLOCK request frame */
typedef struct __attribute__ ((__packed__))
{
    /** Byte offset from the beginning of scratchpad memory */
    uint32_t        start_addr;
    /** Number of bytes of data */
    uint8_t         num_bytes;
    /** Byte data */
    uint8_t         bytes[MSAP_SCRATCHPAD_BLOCK_MAX_NUM_BYTES];
} msap_scratchpad_block_req_t;

#define FRAME_MSAP_SCRATCHPAD_BLOCK_REQ_HEADER_SIZE  \
    (sizeof(msap_scratchpad_block_req_t) - MSAP_SCRATCHPAD_BLOCK_MAX_NUM_BYTES)

/** Result of MSAP-SCRATCHPAD_BLOCK request */
typedef enum
{
    /** Block request was successful */
    MSAP_SCRATCHPAD_BLOCK_SUCCESS = 0,
    /** Block request was successful and all data received OK */
    MSAP_SCRATCHPAD_BLOCK_COMPLETED_OK = 1,
    /** Block request was successful, all data received but error in data */
    MSAP_SCRATCHPAD_BLOCK_COMPLETED_ERROR = 2,
    /** Stack in invalid state */
    MSAP_SCRATCHPAD_BLOCK_INVALID_STATE = 3,
    /** No block writes have been started */
    MSAP_SCRATCHPAD_BLOCK_NOT_ONGOING = 4,
    /** Invalid \ref msap_scratchpad_block_req_t::start_addr */
    MSAP_SCRATCHPAD_BLOCK_INVALID_START_ADDR = 5,
    /** Invalid \ref msap_scratchpad_block_req_t::num_bytes */
    MSAP_SCRATCHPAD_BLOCK_INVALID_NUM_BYTES = 6,
    /** Data does not appear to be a valid scratchpad file */
    MSAP_SCRATCHPAD_BLOCK_INVALID_DATA = 7,
} msap_scratchpad_block_e;

/** MSAP-SCRATCHPAD_STATUS confirmation frame */
typedef struct __attribute__ ((__packed__))
{
    /** Information about stored scratchpad */
    uint32_t    num_bytes;
    uint16_t    crc;
    uint8_t     seq;
    uint8_t     type;
    uint8_t     status;
    /**
     * Information about processed scratchpad, i.e. the scratchpad
     * that produced the currently running firmware
     */
    uint32_t    processed_num_bytes;
    uint16_t    processed_crc;
    uint8_t     processed_seq;
    uint32_t    area_id;
    /** Version information of currently running firmware */
    uint8_t     major_version;
    uint8_t     minor_version;
    uint8_t     maint_version;
    uint8_t     devel_version;
} msap_scratchpad_status_cnf_t;

/** Result of MSAP-SCRATCHPAD_BOOTABLE request */
typedef enum
{
    MSAP_SCRATCHPAD_BOOTABLE_SUCCESS = 0,
    MSAP_SCRATCHPAD_BOOTABLE_INVALID_STATE = 1,
    MSAP_SCRATCHPAD_BOOTABLE_NO_SCRATCHPAD = 2,
    MSAP_SCRATCHPAD_BOOTABLE_ACCESS_DENIED = 3,
} msap_scratchpad_bootable_result_e;

/** MSAP-REMOTE_STATUS request frame */
typedef struct __attribute__ ((__packed__))
{
    /** Unicast or broadcast targeting */
    w_addr_t        target;
} msap_remote_status_req_t;

/** Result of MSAP-REMOTE_STATUS request
    NB: ONLY one return code possible as feature is not
    implemented anymore and must be used through Remote API
*/
typedef enum
{
    MSAP_REMOTE_STATUS_ACCESS_DENIED = 4,
} msap_remote_status_e;

/** MSAP-REMOTE_STATUS indication frame */
typedef struct __attribute__ ((__packed__))
{
    uint8_t     queued_indications;
    w_addr_t    source;
    /** Information about stored scratchpad */
    uint32_t    num_bytes;
    uint16_t    crc;
    uint8_t     seq;
    uint8_t     type;
    uint8_t     status;
    /**
     * Information about processed scratchpad, i.e. the scratchpad
     * that produced the currently running firmware
     */
    uint32_t    processed_num_bytes;
    uint16_t    processed_crc;
    uint8_t     processed_seq;
    uint32_t    area_id;
    /** Version information of currently running firmware */
    uint8_t     major_version;
    uint8_t     minor_version;
    uint8_t     maint_version;
    uint8_t     devel_version;
    uint16_t    update_req_timeout;
} msap_remote_status_ind_t;

/** Result of MSAP-SCAN_NBORS request */
typedef enum
{
    MSAP_SCAN_NBORS_SUCCESS = 0,
    MSAP_SCAN_NBORS_INVALID_STATE = 1,
    MSAP_SCAN_NBORS_ACCESS_DENIED = 2,
} msap_scan_nbors_status_e;

/** Result of MSAP-NRLS_STATUS request */
typedef enum
{
    MSAP_NRLS_SLEEP_ACTIVE        = 1,
    MSAP_NRLS_SLEEP_NOT_STARTED   = 2
} msap_nrls_status_e;

/** MSAP-ON_SCANNED_NBORS indication frame */
typedef struct __attribute__ ((__packed__))
{
    uint8_t     queued_indications;
    uint8_t     scan_ready;
} msap_on_scanned_nbors_ind_t;

/** MSAP-GET_NBORS */
typedef struct __attribute__ ((__packed__))
{
    /** Result of query, number of nbors is piggybacked here */
    uint8_t                 result;
    /** The neighbors themselves, result amount are filled, rest is garbage */
    msap_neighbor_entry_t   neighbors[MSAP_MAX_NBORS];
} msap_neighbors_cnf_t;

/** Result of MSAP-SLEEP start/stop request */
typedef enum
{
    MSAP_SLEEP_SUCCESS = 0,
    MSAP_SLEEP_INVALID_STATE = 1,
    MSAP_SLEEP_INVALID_ROLE = 2,
    MSAP_SLEEP_INVALID_VALUE = 5,
    MSAP_SLEEP_ACCESS_DENIED = 6,
} msap_sleep_update_e;

/** MSAP-SLEEP request frame */
typedef struct __attribute__ ((__packed__))
{
    /** timeout value for stack sleep period */
    uint32_t    seconds;
    /** configuration for DREQ NRLS handling */
    uint32_t    app_config_nrls;
} msap_sleep_start_req_t;

/** MSAP-SLEEP state request rsp frame */
typedef struct __attribute__ ((__packed__))
{
    /** stack sleep state and remaining sleep time in seconds */
    uint8_t     sleep_started;
    uint32_t    sleep_seconds;

} msap_sleep_state_rsp_t;

/** MSAP-SLEEP state request rsp frame */
typedef struct __attribute__ ((__packed__))
{
    /** Previous NRLS time to enter onto stack sleep from request */
    uint32_t    gotoNRSL_seconds;

} msap_sleep_latest_gotosleep_rsp_t;

/** MSAP-GET_INSTALL_QUALITY */
typedef struct
{
    /** Result of query, always OK */
    uint8_t result;
    /** Quality */
    uint8_t quality;
    /** Error codes */
    uint8_t error_codes;
} msap_install_quality_cnf_t;

/** Action of MSAP-SCRATCHPAD_TARGET(READ/WRITE)  */
typedef enum
{
    /** No otap in the sink tree */
    MSAP_SCRATCHPAD_ACTION_NO_OTAP = 0,
    /** Only propagate the target scratchpad but do not process it */
    MSAP_SCRATCHPAD_ACTION_PROPAGATE_ONLY = 1,
    /** Propagate the target scratchpad and process it immediately */
    MSAP_SCRATCHPAD_ACTION_PROPAGATE_AND_PROCESS = 2,
    /** Same as previous except that the processing should happen after
     *  the given delay. Delay starts when node receive the information */
    MSAP_SCRATCHPAD_ACTION_PROPAGATE_AND_PROCESS_WITH_DELAY = 3,
    /** Exchange and processing of scratchpad is managed the old way
     * (seq comparison) */
    MSAP_SCRATCHPAD_ACTION_LEGACY = 5,
} msap_scratchpad_action_e;

/** MSAP_SCRATCHPAD_TARGET_WRITE_REQ request */
typedef struct __attribute__ ((__packed__))
{
    /** Target sequence for the scratchpad to handle */
    uint8_t  target_sequence;
    /** Target CRC for the scrtachpad to handle*/
    uint16_t target_crc;
    /** Target action for the scratchpad */
    uint8_t  action;
    /** Param for the action (specific per action) */
    uint8_t  param;
} msap_scratchpad_target_write_req_t;

/** Result of MSAP-SCRATCHPAD_WRITE_TARGET request */
typedef enum
{
    /** Scratchpad write started successfully */
    MSAP_SCRATCHPAD_TARGET_SUCCESS = 0,
    /** Stack in invalid state */
    MSAP_SCRATCHPAD_TARGET_INVALID_ROLE = 1,
    /** One of the parameter provided is wrong */
    MSAP_SCRATCHPAD_TARGET_INVALID_VALUE = 2,
    /** Access denied due to feature lock bits */
    MSAP_SCRATCHPAD_TARGET_ACCESS_DENIED = 3,
} msap_scratchpad_target_write_res_e;

/** MSAP_SCRATCHPAD_TARGET_READ_REQ request */
typedef struct __attribute__ ((__packed__))
{
    /** Read result */
    uint8_t result;
    /** fields: \see msap_scratchpad_target_write_req_t */
    uint8_t  target_sequence;
    uint16_t target_crc;
    uint8_t  action;
    uint8_t  param;
} msap_scratchpad_target_read_cnf_t;

/** Maximum number of bytes in a single scratchpad block */
#define MSAP_SCRATCHPAD_BLOCK_READ_MAX_NUM_BYTES (MSAP_SCRATCHPAD_BLOCK_MAX_NUM_BYTES)

/** MSAP-SCRATCHPAD_BLOCK_READ request frame */
typedef struct __attribute__ ((__packed__))
{
    /** Byte offset from the beginning of scratchpad memory */
    uint32_t        start_addr;
    /** Number of bytes of data */
    uint8_t         num_bytes;
} msap_scratchpad_block_read_req_t;

/** Result of MSAP-SCRATCHPAD_BLOCK_READ request */
typedef enum
{
    /** Read request was successful */
    MSAP_SCRATCHPAD_BLOCK_READ_SUCCESS = 0,
    /** Stack in invalid state */
    MSAP_SCRATCHPAD_BLOCK_READ_INVALID_STATE = 1,
    /** Invalid \ref msap_scratchpad_block_read_req_t::start_addr */
    MSAP_SCRATCHPAD_BLOCK_READ_INVALID_START_ADDR = 2,
    /** Invalid \ref msap_scratchpad_block_read_req_t::num_bytes */
    MSAP_SCRATCHPAD_BLOCK_READ_INVALID_NUM_BYTES = 3,
    /** No stored scratchpad */
    MSAP_SCRATCHPAD_BLOCK_READ_NO_SCRATCHPAD = 4,
    /** Access denied due to feature lock bits */
    MSAP_SCRATCHPAD_BLOCK_READ_ACCESS_DENIED = 5,
} msap_scratchpad_block_read_e;

/** MSAP-SCRATCHPAD_BLOCK_READ confirmation frame */
typedef struct __attribute__ ((__packed__))
{
    /** Read result: \see msap_scratchpad_block_read_e */
    uint8_t result;
    /** Byte data */
    uint8_t bytes[MSAP_SCRATCHPAD_BLOCK_READ_MAX_NUM_BYTES];
} msap_scratchpad_block_read_cnf_t;

#define FRAME_MSAP_SCRATCHPAD_BLOCK_READ_CNF_HEADER_SIZE  \
    (sizeof(msap_scratchpad_block_read_cnf_t) - MSAP_SCRATCHPAD_BLOCK_READ_MAX_NUM_BYTES)

typedef union
{
    msap_start_req_t                    start_req;
    msap_state_ind_t                    state_ind;
    msap_int_write_req_t                int_write_req;
    msap_int_read_cnf_t                 int_read_cnf;
    msap_sink_cost_write_req_t          cost_write_req;
    msap_sink_cost_read_cnf_t           cost_read_cnf;
    msap_neighbors_cnf_t                nbor_cnf;
    msap_int_ind_t                      int_ind;
    msap_ind_poll_cnf_t                 ind_poll_cnf;
    msap_scratchpad_start_req_t         scratchpad_start_req;
    msap_scratchpad_block_req_t         scratchpad_block_req;
    msap_scratchpad_status_cnf_t        scratchpad_status_cnf;
    msap_remote_status_req_t            remote_status_req;
    msap_remote_status_ind_t            remote_status_ind;
    msap_on_scanned_nbors_ind_t         on_scanned_nbors;
    msap_sleep_start_req_t              sleep_start_req;
    msap_sleep_state_rsp_t              sleep_state_rsp;
    msap_sleep_latest_gotosleep_rsp_t   sleep_gotosleep_rsp_t;
    msap_install_quality_cnf_t          inst_qual_cnf;
    msap_scratchpad_target_write_req_t  scratchpad_target_write_req;
    msap_scratchpad_target_read_cnf_t   scratchpad_target_read_cnf;
    msap_scratchpad_block_read_req_t    scratchpad_block_read_req;
    msap_scratchpad_block_read_cnf_t    scratchpad_block_read_cnf;
} frame_msap;

#endif /* MSAP_FRAMES_H_ */
