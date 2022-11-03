/* Copyright 2019 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/**
 * \file wms_joining.h
 *
 * Application library for sending and receiving joining beacons
 *
 * Library services are accessed  via @ref app_lib_joining_t "lib_joining"
 * handle.
 *
 */
#ifndef APP_LIB_JOINING_H_
#define APP_LIB_JOINING_H_

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "wms_app.h"
#include "wms_settings.h"  // For app_lib_settings_net_addr_t and
                           // app_lib_settings_net_channel_t
#include "wms_time.h"     // For app_lib_time_timestamp_coarse_t

/** \brief Library symbolic name  */
#define APP_LIB_JOINING_NAME    0x000a2336 //!< "JOIN"

/** \brief Maximum supported library version */
#define APP_LIB_JOINING_VERSION 0x200

/** \brief Minimum joining beacon interval, in milliseconds
 *
 * Used when defining @ref app_lib_joining_beacon_tx_param_t.interval "interval"
 * of joining beacons in @ref app_lib_joining_start_joining_beacon_tx_f
 * "lib_joining->startJoiningBeaconTx()" service.
 */
#define APP_LIB_JOINING_MIN_INTERVAL    100

/** \brief Maximum joining beacon interval, in milliseconds
 *
 * Used when defining @ref app_lib_joining_beacon_tx_param_t.interval "interval"
 * of joining beacons in @ref app_lib_joining_start_joining_beacon_tx_f
 * "lib_joining->startJoiningBeaconTx()" service.
 */
#define APP_LIB_JOINING_MAX_INTERVAL    8000

/** \brief Minimum joining beacon interval, in milliseconds
 *
 * Used when defining @ref app_lib_joining_beacon_rx_param_t.timeout "timeout"
 * of joining beacon reception in @ref app_lib_joining_start_joining_beacon_rx_f
 * "lib_joining->startJoiningBeaconRx()" service.
 */
#define APP_LIB_JOINING_MIN_TIMEOUT     100

/** \brief Maximum joining beacon interval, in milliseconds
 *
 * Used when defining @ref app_lib_joining_beacon_rx_param_t.timeout "timeout"
 * of joining beacon reception in @ref app_lib_joining_start_joining_beacon_rx_f
 * "lib_joining->startJoiningBeaconRx()" service.
 */
#define APP_LIB_JOINING_MAX_TIMEOUT     10000

/** \brief Maximum number of payload bytes in a joining beacon
 *
 * Used when defining @ref app_lib_joining_beacon_tx_param_t.payload_num_bytes
 * "payload bytes" of joining beacons in @ref
 * app_lib_joining_start_joining_beacon_tx_f
 * "lib_joining->startJoiningBeaconTx()" service.
 */
#define APP_LIB_JOINING_MAX_PAYLOAD_NUM_BYTES   64

/**
 * \brief   Status values for beacon reception callback
 *
 * Used in @ref app_lib_joining_beacon_rx_cb_f "beacon reception callback" set
 * with service @ref app_lib_joining_start_joining_beacon_rx_f
 * "lib_joining->startJoiningBeaconRx()".
 */
typedef enum
{
    /** Reception timeout elapsed or requested number of beacons received */
    APP_LIB_JOINING_RX_STATUS_DONE = 0,
    /** Beacon reception stopped by calling @ref app_lib_joining_stop_joining_beacon_rx_f
     *  "lib_joining->stopJoiningBeaconRx()" */
    APP_LIB_JOINING_RX_STATUS_STOPPED = 1,
    /**
     * A higher priority radio event (e.g. network beacon scan)
     * interrupted the joining beacon reception
     */
    APP_LIB_JOINING_RX_STATUS_INTERRUPTED = 2,
    /** No space in buffer for any more joining beacons */
    APP_LIB_JOINING_RX_STATUS_OUT_OF_SPACE = 3
} app_lib_joining_rx_status_e;

/**
 * \brief   Parameters for sending joining beacons
 *
 * Used in service @ref app_lib_joining_start_joining_beacon_tx_f
 * "lib_joining->startJoiningBeaconTx()".
 */
typedef struct
{
    /** Interval of joining beacons, in milliseconds */
    uint32_t interval;
    /** Network address (radio address) to use for sending joining beacons */
    app_lib_settings_net_addr_t addr;
    /** Radio channel to use for sending joining beacons */
    app_lib_settings_net_channel_t channel;
    /** Transmission power to use for sending joining beacons, in dBm */
    int8_t tx_power;
    /** A 32-bit type field for early filtering of received joining beacons */
    uint32_t type;
    /** Optional payload, ignored if payload_num_bytes == 0 */
    const void * payload;
    /** Number of payload bytes */
    size_t payload_num_bytes;
} app_lib_joining_beacon_tx_param_t;

/**
 * \brief   Received joining beacon
 *
 * Used in @ref app_lib_joining_beacon_rx_cb_f "beacon reception callback" set
 * with service @ref app_lib_joining_start_joining_beacon_rx_f
 * "lib_joining->startJoiningBeaconRx()".
 */
struct app_lib_joining_received_beacon_s
{
    /** Pointer to next joining beacon or NULL */
    struct app_lib_joining_received_beacon_s * next;
    /**
     * Size of this joining beacon structure in bytes,
     * including the next and num_bytes fields
     */
    size_t num_bytes;
    /** A 32-bit protocol type identifier of received joining beacon */
    uint32_t type;
    /** Optional payload, NULL if payload_num_bytes == 0 */
    uint8_t * payload;
    /** Number of payload bytes */
    size_t payload_num_bytes;
    /** Time when beacon received, as coarse timestamp */
    app_lib_time_timestamp_coarse_t time;
    /** Network address of found network */
    app_lib_settings_net_addr_t addr;
    /** Network channel of found network */
    app_lib_settings_net_channel_t channel;
    /** Signal strength of received joining beacon, in dBm */
    int8_t rssi;
    /** Transmission power of joining beacon, in dBm */
    int8_t tx_power;
};

typedef struct app_lib_joining_received_beacon_s
    app_lib_joining_received_beacon_t;

/**
 * \brief   Function type for joining beacon RX callback
 * \param   status
 *          Status of joining beacon RX process
 * \param   beacons
 *          Pointer to first received beacon (read-only) or NULL
 * \param   num_beacons
 *          Number of received joining beacons
 */
typedef void (*app_lib_joining_beacon_rx_cb_f)(
    app_lib_joining_rx_status_e status,
    const app_lib_joining_received_beacon_t * beacons,
    size_t num_beacons);

/**
 * \brief   Parameters for receiving joining beacons
 *
 * Used in @ref app_lib_joining_start_joining_beacon_rx_f
 * "lib_joining->startJoiningBeaconRx()" service.
 */
typedef struct
{
    /** Function to call after joining beacon reception ends for any reason */
    app_lib_joining_beacon_rx_cb_f cb;
    /** Maximum time for the callback function to execute in microseconds */
    uint32_t max_exec_time_us;
    /** Network address to use for receiving joining beacons */
    app_lib_settings_net_addr_t addr;
    /** Network channel to use for receiving joining beacons */
    app_lib_settings_net_channel_t channel;
    /** Joining beacon reception duration in milliseconds */
    uint32_t timeout;
    /** Maximum number of joining beacons to receive */
    size_t max_num_beacons;
    /** A buffer for collecting joining beacons */
    void * buffer;
    /** Size of beacon buffer in bytes */
    size_t num_bytes;
} app_lib_joining_beacon_rx_param_t;

/**
 * \brief   Start transmitting joining beacons
 * \param   param
 *          Joining beacon transmission parameters
 * \return  Result code, \ref APP_RES_OK if successful,
 *          \ref APP_RES_INVALID_STACK_STATE if stack is not running,
 *          \ref APP_RES_RESOURCE_UNAVAILABLE if joining beacon reception
 *          is ongoing, \ref APP_RES_INVALID_VALUE if addr, channel or
 *          interval is invalid, \ref APP_RES_INVALID_NULL_POINTER
 *          if payload pointer is NULL and \p payload_num_bytes > 0
 * \note    Power is rounded to the closest possible value and the value
 *          pointed by tx_power_p is modified to the used value
 * \note    This function can be called even if already transmitting
 *          joining beacons. The transmissions continue with the new
 *          parameters
 */
typedef app_res_e (*app_lib_joining_start_joining_beacon_tx_f)(
    const app_lib_joining_beacon_tx_param_t * param);

/**
 * \brief   Stop transmitting joining beacons
 * \return  Result code, always \ref APP_RES_OK
 */
typedef app_res_e (*app_lib_joining_stop_joining_beacon_tx_f)(void);

/**
 * \brief   Start receiving joining beacons
 * \param   param
 *          Joining beacon reception parameters
 * \return  Result code, \ref APP_RES_OK if successful,
 *          \ref APP_RES_INVALID_STACK_STATE if stack is not running,
 *          \ref APP_RES_INVALID_CONFIGURATION if the node is a sink,
 *          \ref APP_RES_INVALID_NULL_POINTER if param, cb or beacons is NULL,
 *          \ref APP_RES_INVALID_VALUE if max_exec_time_us, addr, channel,
 *          timeout, beacon_num_bytes or max_num_beacons is invalid or
 *          \ref APP_RES_RESOURCE_UNAVAILABLE if network beacon scan or
 *          joining beacon transmission or reception is ongoing
 * \note    While beacon reception is ongoing, all other network activity
 *          is paused
 * \note    It is not possible to start receiving joining beacons if normal
 *          network beacon scanning is ongoing. In that case, this function
 *          has to be tried again a few seconds later.
 */
typedef app_res_e (*app_lib_joining_start_joining_beacon_rx_f)(
    const app_lib_joining_beacon_rx_param_t * param);

/**
 * \brief   Stop receiving joining beacons
 * \return  Result code, always \ref APP_RES_OK
 * \note    If beacon reception is currently ongoing, Reception callback
 *          will be called with status \ref APP_LIB_JOINING_RX_STATUS_STOPPED
 *          some time after this function returns with \ref APP_RES_OK
 */
typedef app_res_e (*app_lib_joining_stop_joining_beacon_rx_f)(void);

/**
 * \brief   Start the joining process by joining a cluster as a joining member
 * \param   addr
 *          Network address (radio address) of the network to try joining
 * \param   channel
 *          Network channel of the network to try joining
 * \return  Result code, \ref APP_RES_OK if successful,
 *          \ref APP_RES_INVALID_STACK_STATE if stack is not running,
 *          \ref APP_RES_INVALID_VALUE if addr or channel is invalid,
 *          \ref APP_RES_INVALID_CONFIGURATION if joining beacon transmission
 *          or reception is ongoing, or if the node is a sink
 * \note    A network scan is automatically started. After the scan the node
 *          will join one of the cluster heads currently transmitting joining
 *          beacons. The cluster head will not change during the joining
 *          process.
 * \note    The joining process can be restarted with other addr and channel
 *          parameters by calling this function again
 */
typedef app_res_e (*app_lib_joining_start_joining_process_f)(
    app_lib_settings_net_addr_t addr,
    app_lib_settings_net_channel_t channel);

/**
 * \brief   Stop the joining process and restore network address and channel
 * \return  Result code, always \ref APP_RES_OK
 * \note    The function can be used to cancel the joining process and return
 *          to normal operation. It is also permissible to call function
 *          \ref app_lib_joining_start_joining_process_f
 *          "app_lib_joining_start_joining_process_f"().
 *          again with new parameters, without
 *          calling this function first
 * \note    A network scan is automatically started. After the scan the node
 *          will join a network as a regular member, if a network is available
 */
typedef app_res_e (*app_lib_joining_stop_joining_process_f)(void);

/**
 * \brief   Enable or disable Remote API control for transmitting joining beacons
 * \return  Result code, always \ref APP_RES_OK
 * \note    Remote API control is enabled by default
 */
typedef app_res_e (*app_lib_joining_enable_remote_api_f)(bool enable);

/**
 * \brief   Enable or disable built-in proxy for handling joining data packets
 * \return  Result code, always \ref APP_RES_OK
 * \note    Built-in proxy is enabled by default
 */
typedef app_res_e (*app_lib_joining_enable_proxy_f)(bool enable);

/**
 * \brief   List of library functions
 */
typedef struct
{
    app_lib_joining_start_joining_beacon_tx_f startJoiningBeaconTx;
    app_lib_joining_stop_joining_beacon_tx_f stopJoiningBeaconTx;
    app_lib_joining_start_joining_beacon_rx_f startJoiningBeaconRx;
    app_lib_joining_stop_joining_beacon_rx_f stopJoiningBeaconRx;
    app_lib_joining_start_joining_process_f startJoiningProcess;
    app_lib_joining_stop_joining_process_f stopJoiningProcess;
    app_lib_joining_enable_remote_api_f enableRemoteApi;
    app_lib_joining_enable_proxy_f enableProxy;
} app_lib_joining_t;

#endif /* APP_LIB_JOINING_H_ */
