/* Copyright 2017 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/**
 * \file wms_state.h
 *
 * Application library for viewing and controlling stack runtime state
 *
 * Library services are accessed via @ref app_lib_state_t "lib_state"
 * handle.
 */
#ifndef APP_LIB_STATE_H_
#define APP_LIB_STATE_H_

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <limits.h>

#include "wms_app.h"
#include "wms_settings.h" // For app_lib_settings_net_channel_t

/** @brief Library symbolic name */
#define APP_LIB_STATE_NAME 0x02f9c165 //!< "STATE"

/** @brief Maximum supported library version */
#define APP_LIB_STATE_VERSION 0x20C

/**
 * @brief   Macro for cost indicating "no route". Used in @ref
 * app_lib_state_nbor_info_t.cost "cost component" in @ref
 * app_lib_state_get_nbors_f "lib_state->getNbors()" service.
 */
#define APP_LIB_STATE_INVALID_ROUTE_COST UINT8_MAX

/**
 * @brief   Macro for cost being unknown. Used in @ref
 * app_lib_state_nbor_info_t.cost "cost component" in @ref
 * app_lib_state_get_nbors_f "lib_state->getNbors()" service.
 */
#define APP_LIB_STATE_COST_UNKNOWN  0

/**
 * @brief   Macro for link reliability being unknown. Used in @ref
 * app_lib_state_nbor_info_t.link_reliability "link reliability component" in
 * @ref app_lib_state_get_nbors_f "lib_state->getNbors()" service.
 */
#define APP_LIB_STATE_LINKREL_UNKNOWN   0

/**
 * @brief   Macro for using with @ref app_lib_state_set_scan_dur_f
 * "lib_state->setScanDuration()" to disable custom scan duration and use stack
 * default scan duration.
 */
#define APP_LIB_STATE_DEFAULT_SCAN  0

/**
 * @brief   Neighbor type
 * @note    Most reliable information is always from next hop and members
 *          Other entries might be very old
 */
typedef enum
{
    /** Neighbor is specifically a next hop cluster, i.e. used as a route to
     *  sink */
    APP_LIB_STATE_NEIGHBOR_IS_NEXT_HOP  = 0,
    /** Neighbor is specifically a member of this node */
    APP_LIB_STATE_NEIGHBOR_IS_MEMBER    = 1,
    /** Neighbor is heard from network scan */
    APP_LIB_STATE_NEIGHBOR_IS_CLUSTER   = 2,
} app_lib_state_nbor_type_e;

/**
 * @brief   Support type for directed advertiser, i.e. does neighbor support
 *          sending directed advertiser packets to it
 */
typedef enum
{
    /** Directed advertiser is supported by neighbor and packet can be sent to
     * it */
    APP_LIB_STATE_DIRADV_SUPPORTED      = 0,
    /** Sending not supported by neighbor */
    APP_LIB_STATE_DIRADV_NOT_SUPPORTED  = 1,
    /** Unknown state. Directed advertiser maybe or maybe not supported by
     * neighbor but packet can not be sent to it */
    APP_LIB_STATE_DIRADV_UNKNOWN        = 2,
} app_lib_state_diradv_support_e;

/**
 * @brief   Scan neighbor type to specify the scans that trigger the callback
 */
typedef enum
{
    /** All scans will trigger the registered callback */
    APP_LIB_STATE_SCAN_NBORS_ALL = 0,
    /** Only explicitly requested scans from app will trigger the registered
     *  callback */
    APP_LIB_STATE_SCAN_NBORS_ONLY_REQUESTED = 1,
} app_lib_state_scan_nbors_type_e;

/**
 * @brief   Neighbors info definition
 */
typedef struct
{
    /** Address of the neighbor node */
    uint32_t address;
    /** Aount of seconds since these values were last updated. */
    uint16_t last_update;
    /** Link reliability to the neighboring node. Scaled so that 0 = 0 %,
     *  255 = 100 %. Value of @ref APP_LIB_STATE_LINKREL_UNKNOWN tells that link
     *  reliability to this neighbor is unknown. */
    uint8_t link_reliability;
    /** Received signal strength, compensated with transmission power, i.e. this
     *  value answers the question "what would the RSSI be, if the neighbor
     *  transmits with its maximum TX power". Larger value means better signal.
     *  * rssi < receiver sensitivity + 10 dB  : Insufficient signal level
     *  * receiver sensitivity + 10 dB <= rssi < receiver_sensitivity + 20 dB :
     *    weak signal level
     *  * rssi >= receiver sensitivity + 20 dB : Good signal level.
     *  @note weak signal level is likely to work in environments without
     *  interference but the probability for connection problems in networks
     *  having some background interference then increases */
    int8_t  norm_rssi;
    /** Route cost to the sink via this neighbor. Value of @ref
     * APP_LIB_STATE_INVALID_ROUTE_COST indicates that a neighbor has no route
     * to a sink. Value of @ref APP_LIB_STATE_COST_UNKNOWN states that cost
     * is unknown for this neighbor. */
    uint8_t cost;
    /** Radio channel used by the neighbor  */
    uint8_t channel;
    /** Type of the neighbor. @ref app_lib_state_nbor_type_e  */
    uint8_t type;
    /**
     * Transmission power used when sending to neighbor.
     * @note Only relevant for @ref app_lib_state_nbor_info_t.type "type"
     * having value of @ref APP_LIB_STATE_NEIGHBOR_IS_NEXT_HOP or @ref
     * APP_LIB_STATE_NEIGHBOR_IS_MEMBER */
    int8_t  tx_power;
    /**
     * Transmission power used when neighbor sending to this device
     * @note Only relevant for @ref app_lib_state_nbor_info_t.type "type"
     * having value of @ref APP_LIB_STATE_NEIGHBOR_IS_NEXT_HOP or @ref
     * APP_LIB_STATE_NEIGHBOR_IS_MEMBER */
    int8_t  rx_power;
    /** Is directed advertiser supported, @ref app_lib_state_diradv_support_e */
    uint8_t diradv_support;
} app_lib_state_nbor_info_t;

/**
 * @brief   Neighbors list definition
 */
typedef struct
{
    /** Input: Maximum amount of neighbors in @ref
     * app_lib_state_nbor_list_t.nbors "nbors" field. Output: Amount of
     * neighbors filled */
    uint32_t number_nbors;
    /** Information of neighbors received */
    app_lib_state_nbor_info_t * nbors;
} app_lib_state_nbor_list_t;

/**
 * @brief   Stack state flags
 */
typedef enum
{
    APP_LIB_STATE_STARTED = 0,                  //!< Stack is started
    APP_LIB_STATE_STOPPED = 1,                  //!< Stack is stopped
    APP_LIB_STATE_NODE_ADDRESS_NOT_SET = 2,     //!< Node address is not set
    APP_LIB_STATE_NETWORK_ADDRESS_NOT_SET = 4,  //!< Network address is not set
    APP_LIB_STATE_NETWORK_CHANNEL_NOT_SET = 8,  //!< Network channel is not set
    APP_LIB_STATE_ROLE_NOT_SET = 16,            //!< Node role is not set
    APP_LIB_STATE_APP_CONFIG_DATA_NOT_SET = 32, //!< App config data is
                                                //!< not set (sink only)
    APP_LIB_STATE_ACCESS_DENIED = 128           //!< Operation is not allowed
} app_lib_state_stack_state_e;

/**
 * @brief   Route state
 */
typedef enum
{
    APP_LIB_STATE_ROUTE_STATE_INVALID = 0,      //!< No next hop / route
    APP_LIB_STATE_ROUTE_STATE_PENDING = 1,      //!< Acquiring next hop / route
    APP_LIB_STATE_ROUTE_STATE_VALID = 2         //!< Valid next hop / route
} app_lib_state_route_state_e;

/**
 * @brief   Type for beacons, passed in @ref app_lib_state_beacon_rx_t
 */
typedef enum
{
    APP_LIB_STATE_BEACON_TYPE_NB = 0,   //!< Network beacon
    APP_LIB_STATE_BEACON_TYPE_CB = 1,   //!< Cluster beacon
} app_lib_state_beacon_type_e;

/**
 * @brief   Structure to hold the information about received beacons
 */
typedef struct
{
    app_addr_t  address; //!< Address of the beacon sender
    /**
     * @brief RSSI in dBm.
     *
     * Larger value means better signal.
     *  * rssi < receiver sensitivity + 10 dB  : Insufficient signal level
     *  * receiver sensitivity + 10 dB <= rssi < receiver_sensitivity + 20 dB :
     *    weak signal level
     *  * rssi >= receiver sensitivity + 20 dB : Good signal level
     *  @note weak signal level is likely to work in environments without
     *  interference but the probability for connection problems in networks
     *  having some background interference then increases */
    int8_t      rssi;
    /**
     * @brief Tx power in dB
     * This equals maximum transmission power that sender can transmit (which
     * is used when transmitting beacons)
     */
    int8_t      txpower;
    bool        is_sink; //!< Device is sink
    bool        is_ll;   //!< Device is LL
    uint8_t     cost;    //!< Cost of the device. 255==no route
    uint8_t     type;    //!< Type of beacon @ref app_lib_state_beacon_type_e
    /**
     * @brief   Sender supports Directed Advertiser sending packets to it
     */
    bool        is_da_support;
} app_lib_state_beacon_rx_t;

/**
 * @note    This struct is deprecated in Wirepas Mesh version 5.2.0 and later.
 */
typedef struct
{
    app_addr_t  address;
    uint8_t     hops_left;
} app_lib_state_hops_adjust_t;

/**
 * @brief   Structure for route information
 */
typedef struct
{
    /** Route state: invalid, pending or valid */
    app_lib_state_route_state_e     state;
    /** Sink's unicast address or 0 if no valid route */
    app_addr_t                      sink;
    /** Next hop's unicast address or 0 if no valid route */
    app_addr_t                      next_hop;
    /** Next hop's (logical) data (cluster) channel or 0 if no valid route */
    app_lib_settings_net_channel_t  channel;
    /** Route cost or APP_LIB_STATE_INVALID_ROUTE_COST if no valid route */
    uint8_t                         cost;
} app_lib_state_route_info_t;

/**
 * @brief   Error codes for installation quality, if an error code is active,
 *          corrective action regarding the installation location is required.
 */
typedef enum
{
    /** No installation quality errors detected */
    APP_LIB_STATE_INSTALL_QUALITY_ERROR_NONE    = 0x00,
    /** Error: Node has no route to sink */
    APP_LIB_STATE_INSTALL_QUALITY_ERROR_NOROUTE = 0x01,
    /** Error: Node does not have enough good quality neighbors */
    APP_LIB_STATE_INSTALL_QUALITY_ERROR_NONBORS = 0x02,
    /** Error: Node has bad RSSI to next hop neighbor */
    APP_LIB_STATE_INSTALL_QUALITY_ERROR_BADRSSI = 0x04,
} install_quality_error_code_e;

/**
 * @brief   Installation quality information. Contains information about the
 *          nodes installation location i.e. its quality indicated by a numeric
 *          value, as well as error codes if something is wrong with the
 *          location.
 *
 *          For more detailed information about the value(s) presented here,
 *          see application note about installation quality API, document
 *          reference: AN-XXX
 */
typedef struct
{
    /** Quality reported as u8. Limits are as follows:
     *  Quality >= 127      : Good installation
     *  127 > Quality > 63  : Average installation
     *  Quality <= 63       : Bad installation */
    uint8_t quality;
    /** Error codes, @ref install_quality_error_code_e */
    uint8_t error_codes;
} app_lib_state_install_quality_t;

/**
 * @brief   Types of scans
 */
typedef enum
{
    // Scan originated by application (by using call @ref
    // app_lib_state_start_scan_nbors_f "lib_state->startScanNbors()")
    SCAN_TYPE_APP_ORIGINATED = 0x00,
    // Scan originated by stack
    SCAN_TYPE_STACK_ORIGINATED = 0x01,
} app_lib_state_scan_type_e;

/**
 * @brief   Information on started scan
 */
typedef struct
{
    // Type of scan, @ref app_lib_state_scan_type_e
    uint8_t scan_type;
} app_lib_state_on_scan_start_info_t;

/**
 * @brief   Function type for scan start callback
 * @param   scan_info
 *          Info on started scan
 */
typedef void (*app_lib_state_on_scan_start_cb_f)
    (const app_lib_state_on_scan_start_info_t * scan_info);

/**
 * @brief   Information on neighbor scan
 */
typedef struct
{
    // Type (originator) of the scan
    app_lib_state_scan_type_e scan_type;
    // Scan completion status: complete (true) or aborted (false)
    bool complete;
} app_lib_state_neighbor_scan_info_t;

/**
 * @brief   Function type for a neighbor scan completion callback
 * @param   scan_info
 *          Info on neighbor scan
 */
typedef void (*app_lib_state_on_scan_nbors_cb_f)
    (const app_lib_state_neighbor_scan_info_t * scan_info);

/**
 * @brief   Function type for a beacon reception callback
 * @param   beacon
 *          Information about received beacon
 */
typedef void (*app_lib_state_on_beacon_cb_f)
    (const app_lib_state_beacon_rx_t * beacon);

/**
 * @brief   Start the stack
 *
 * This is most commonly used in the end of the @ref app_init "App_init()"
 * function to start the radio operation.
 *
 * Example:
 *
 * @code
 * void App_init(const app_global_functions_t * functions)
 * {
 *     ...
 *     // Start the stack
 *     lib_state->startStack();
 * }
 * @endcode
 *
 * In order to start, the stack needs minimum of four following attributes to be
 * configured: @ref app_lib_settings_set_node_role_f "device role", @ref
 * app_lib_settings_set_node_address_f "device address", @ref
 * app_lib_settings_set_network_address_f "network address" and @ref
 * app_lib_settings_set_network_channel_f "network channel". These attributes
 * can be set by the application thanks to the Single-MCU API during the
 * initialization step. Device role, network address and network channel can be
 * hardcoded in the application image.
 *
 * \return  Result code, @ref APP_RES_OK if successful
 */
typedef app_res_e (*app_lib_state_start_stack_f)(void);

/**
 * @brief   Stop the stack
 *
 * @return  Result code, @ref APP_RES_OK if successful
 * @note    Stopping the stack will reboot the system. The system shutdown
 *          callback set with @ref app_lib_system_set_shutdown_cb_f
 *          "lib_system->setShutdownCb()"
 *          is called just before rebooting. Node configuration may
 *          be done in this callback
 * @note    This function never returns
 *
 * Example:
 * @code
 * lib_state->stopStack();
 * @endcode
 */
typedef app_res_e (*app_lib_state_stop_stack_f)(void);

/**
 * @brief   Get the stack state
 * @return  Bit field of stack state, @ref app_lib_state_stack_state_e
 *
 * Service indicates whether the stack is running or not, and whether it can
 * be started.
 *
 * Example:
 * @code
 * app_lib_state_stack_state_e state;
 * state = lib_state->getStackState();
 * @endcode
 */
typedef uint8_t (*app_lib_state_get_stack_state_f)(void);

/**
 * @brief   Get the number of routes this node has to a sink
 * @param   count_p
 *          Pointer to store the result. Whenever there is a route to sink,
 *          value is 1, otherwise 0.
 * @return  Result code, @ref APP_RES_OK if successful,
 *          @ref APP_RES_INVALID_STACK_STATE if stack not running
 *
 * Example:
 * @code
 * size_t hasroute;
 * lib_state->getRouteCount(&hasroute);
 * @endcode
 */
typedef uint8_t (*app_lib_state_get_route_count_f)(size_t * count_p);

/**
 * @brief   Get diagnostics interval
 * @return  Diagnostics interval in seconds
 */
typedef uint16_t (*app_lib_state_get_diag_interval_f)(void);

/**
 * @brief   Get current access cycle
 * @param   ac_value_p
 *          Pointer to store the current access cycle value in milliseconds
 *          Updated if return code is @ref APP_RES_OK
 * @return  Result code, @ref APP_RES_OK if successful,
 *          @ref APP_RES_INVALID_STACK_STATE if stack not running
 */
typedef app_res_e (*app_lib_state_get_access_cycle_f)(uint16_t * ac_value_p);

/**
 * @brief   Set a callback to be called when neighbor scan is complete
 * @param   cb
 *          The function to be executed, or NULL to unset
 * @param   type
 *          The type of scan event app is interested (Unused if cb is NULL)
 * @return  Result code
 *
 * Example:
 * @code
 *
 * static void onScannedNborsCb(void)
 * {
 *    ...
 * }
 *
 * ...
 * lib_state->setOnScanNborsCb(onScannedNborsCb, APP_LIB_STATE_SCAN_NBORS_ALL);
 * ...
 *
 * @endcode
 */
typedef app_res_e (*app_lib_state_set_on_scan_nbors_cb_f)
    (app_lib_state_on_scan_nbors_cb_f cb);

/**
 * @brief   Set a callback to be called when neighbor scan starts
 * @param   cb
 *          Function to be executed, or NULL to unregister
 * @return  Result code, always @ref APP_RES_OK
 *
 * Example:
 * @code
 *
 * static
 * void onScanStartCb(const app_lib_state_on_scan_start_info_t * scan_info)
 * {
 *    // Here scan_info->scan_type tells who originated the scan
 *    ...
 * }
 *
 * ...
 * lib_state->setOnScanStartCb(onScanStartCb);
 * ...
 *
 * @endcode
 */
typedef app_res_e (*app_lib_state_set_on_scan_start_cb_f)
    (app_lib_state_on_scan_start_cb_f cb);

/**
 * @brief   Start neighbor scan
 *
 * This service can be used by the application to get fresh information about
 * neighbors. Application can trigger to measurement all neighbors and once
 * the measurement is done, application is informed it over API (see @ref
 * app_lib_state_set_on_scan_nbors_cb_f
 * "lib_state->setOnScanNborsCb" service).
 *
 * @return  Result code, always @ref APP_RES_OK unless stack is not running when
 *          @ref APP_RES_INVALID_STACK_STATE is returned.
 *
 * Example:
 * @code
 * lib_state->startScanNbors();
 * @endcode
 */
typedef app_res_e (*app_lib_state_start_scan_nbors_f)(void);

/**
 * @brief   Get list of neighbors
 * @param   nbors_list
 *          Pointer to store the information of list of neighbors.
 * @return  Result code, always @ref APP_RES_OK
 *
 * This service can be used to tell the status of a node's neighbors. This
 * information may be used for various purposes, for example to estimate where
 * a node is located.
 *
 * Example:
 * @code
 * #define NBOR_LIST_SIZE   6
 * app_lib_state_nbor_info_t nbors[NBOR_LIST_SIZE];
 * app_lib_state_nbor_list_t nbors_list =
 * {
 *     .number_nbors = NBOR_LIST_SIZE,
 *     .nbors = &nbors[0]
 * };
 * ...
 * lib_state->getNbors(&nbors_list);
 * @endcode
 */
typedef app_res_e (*app_lib_state_get_nbors_f)
    (app_lib_state_nbor_list_t * nbors_list);

/**
 * @brief   Set a callback to be called when a beacon is received
 * @param   cb
 *          The function to be executed, or NULL to unset
 * @return  Result code, always @ref APP_RES_OK
 */
typedef app_res_e
    (*app_lib_state_set_on_beacon_cb_f)(app_lib_state_on_beacon_cb_f cb);

/**
 * @brief   Get available energy
 * @param   energy_p
 *          Pointer to store the available energy as a
 *          proportional value from 0 to 255
 * @return  Result code, always @ref APP_RES_NOT_IMPLEMENTED
 *
 * @note This is legacy service used in stack versions <5.1.0.
 */
typedef app_res_e (*app_lib_state_get_energy_f)(uint8_t * energy_p);

/**
 * @brief   Set available energy
 * @param   energy
 *          Available energy as a proportional value from 0 to 255
 *          * 0 corresponds to a state where the node is almost out of energy
 *          * 255 corresponds to a state where maximum amount of energy is
 *            available. Also if there is no means to evaluate remaining energy,
 *            this value should be used.
 * @return  Result code, always @ref APP_RES_NOT_IMPLEMENTED
 *
 * @note This is legacy service used in stack versions <5.1.0.
 *
 */
typedef app_res_e (*app_lib_state_set_energy_f)(uint8_t energy);

/**
 * @brief   Query the currently set additional penalty for the sink usage
 * @param   cost_p
 *          Pointer to store the current initial cost
 * @return  Result code, @ref APP_RES_OK if successful,
 *          @ref APP_RES_INVALID_CONFIGURATION if node is not a sink,
 *          @ref APP_RES_INVALID_NULL_POINTER if \p cost_p is NULL
 *
 * Example:
 * @code
 * uint8_t current_cost;
 * lib_state->getSinkCost(&current_cost);
 * @endcode
 */
typedef app_res_e (*app_lib_state_get_sink_cost_f)(uint8_t * cost_p);

/**
 * @brief   Set additional penalty for the sink usage
 *
 * This service can be used to inform the sink that the backend communication
 * has problems. In order to keep the entire network operational, other nodes
 * can be forced to use other sinks with working backend communication
 *
 * @param   cost
 *          Value of 0 means that connection is good and no additional penalty
 *          is sent to sink usage. Value of 254 includes maximum penalty
 * @return  Result code, @ref APP_RES_OK if successful,
 *          @ref APP_RES_INVALID_CONFIGURATION if node is not a sink
 *
 *
 * Example on use:
 * @code
 * lib_state->setSinkCost(8);
 * @endcode
 */
typedef app_res_e (*app_lib_state_set_sink_cost_f)(const uint8_t cost);

/**
 * @brief   Callback when route is changed
 *
 * Used with @ref app_lib_state_set_route_cb_f
 * Function prototype for a route information changed callback.
 * The stack invokes this callback whenever anything in the route
 * information structure has changed. This can be utilized to wake up
 * the application to reach such information. Do not read the route
 * information directly in this callback's context.
 */
typedef void (*app_lib_state_route_changed_cb_f)(void);

/**
 * @brief   Get route information
 * @param   info [out]
 *          Route information is provided here
 * @return  Result code, @ref APP_RES_OK if successful
 */
typedef app_res_e (*app_lib_state_get_route_f)
    (app_lib_state_route_info_t * info);

/**
 * @brief   Set callback for route change notification
 *
 * When route information changes, the application is notified via this handle.
 *
 * @param   cb
 *          Callback called on route change
 * @param   unused
 *          Unused parameter, always set to 0
 * @return  Result code, @ref APP_RES_OK if successful
 */
typedef app_res_e (*app_lib_state_set_route_cb_f)
    (const app_lib_state_route_changed_cb_f cb, uint32_t unused);

/**
 * @note    This callback is deprecated in Wirepas Mesh version 5.2.0 and later.
 */
typedef uint8_t (*app_lib_state_adjust_hops_cb_f)
    (const app_lib_state_hops_adjust_t * info);

/**
 * @return  Result code, always @ref APP_RES_NOT_IMPLEMENTED
 * @note    This service is deprecated in Wirepas Mesh version 5.2.0 and later.
 *          User service @ref app_lib_data_set_local_mc_f
 *          "lib_data->setLocalMulticastInfo()" instead.
 */
typedef app_res_e (*app_lib_state_set_adjust_hops_cb_f)
    (app_lib_state_adjust_hops_cb_f cb);

/**
 * @brief   Set scan duration to be used.
 * @param   duration_us
 *          Scan duration in microseconds. If @p duration_us is @ref
 *          APP_LIB_STATE_DEFAULT_SCAN, use default-length value and perform
 *          similar length as standard stack scan operation
 * @return  @ref APP_RES_OK if values is ok, @ref APP_RES_INVALID_VALUE if value
 *          is too large or small. @ref APP_RES_INVALID_CONFIGURATION if device
 *          is not @ref APP_LIB_SETTINGS_ROLE_ADVERTISER nor @ref
 *          APP_LIB_SETTINGS_ROLE_SUBNODE.
 *
 * Example:
 * @code
 *
 * uint32_t do_short_scan(void)
 * {
 *     // Perform scan of 0.5 seconds only
 *     lib_state->setScanDuration(500000);
 *     lib_state->startScanNbors();
 *     // Perform only once
 *     return APP_LIB_SYSTEM_STOP_PERIODIC;
 * }
 *
 * void App_init(const app_global_functions_t * functions)
 * {
 *     // ...
 *     // Create short scan in 10 seconds
 *     lib_system->setPeriodicCb(do_short_scan, 10000000, 1000);
 *     lib_state->startStack();
 * }
 * @endcode
 */
typedef app_res_e (*app_lib_state_set_scan_dur_f)(uint32_t duration_us);

/**
 * @brief       Stops ongoing scan operation before ended
 * @return      Normally @ref APP_RES_OK. @ref APP_RES_INVALID_CONFIGURATION if
 *              device is not @ref APP_LIB_SETTINGS_ROLE_ADVERTISER nor @ref
 *              APP_LIB_SETTINGS_ROLE_SUBNODE, @ref APP_RES_RESOURCE_UNAVAILABLE
 *              if memory has ran out, @ref
 *              APP_RES_INVALID_STACK_STATE if stack is not running.
 *
 * This service aborts ongoing scan operation started with @ref
 * app_lib_state_start_scan_nbors_f "lib_state->startScanNbors()".
 */
typedef app_res_e (*app_lib_state_scan_stop_f)(void);

/**
 * @brief   Read installation quality, @ref app_lib_state_install_quality_t
 * @param   qual_out [out]
 *          The installation quality information is copied to this pointer
 * @return  APP_RES_OK if value was read OK
 */
typedef app_res_e (*app_lib_state_get_install_quality_f)
    (app_lib_state_install_quality_t * qual_out);

/**
 * @brief   List of library functions of v3 version (0x202)
 */
typedef struct
{
    app_lib_state_start_stack_f                     startStack;
    app_lib_state_stop_stack_f                      stopStack;
    app_lib_state_get_stack_state_f                 getStackState;
    app_lib_state_get_route_count_f                 getRouteCount;
    app_lib_state_get_diag_interval_f               getDiagInterval;
    app_lib_state_get_access_cycle_f                getAccessCycle;
    app_lib_state_set_on_scan_nbors_cb_f            setOnScanNborsCb;
    app_lib_state_start_scan_nbors_f                startScanNbors;
    app_lib_state_get_nbors_f                       getNbors;
    app_lib_state_set_on_beacon_cb_f                setOnBeaconCb;
    app_lib_state_get_energy_f                      getEnergy;
    app_lib_state_set_energy_f                      setEnergy;
    app_lib_state_get_sink_cost_f                   getSinkCost;
    app_lib_state_set_sink_cost_f                   setSinkCost;
    app_lib_state_get_route_f                       getRouteInfo;
    app_lib_state_set_route_cb_f                    setRouteCb;
    app_lib_state_set_adjust_hops_cb_f              setHopsLeftCb;
    app_lib_state_set_scan_dur_f                    setScanDuration;
    app_lib_state_scan_stop_f                       stopScanNbors;
    app_lib_state_get_install_quality_f             getInstallQual;
    app_lib_state_set_on_scan_start_cb_f            setOnScanStartCb;
} app_lib_state_t;

#endif /* APP_LIB_STATE_H_ */
