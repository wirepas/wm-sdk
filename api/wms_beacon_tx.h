/* Copyright 2017 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/**
 * \file wms_beacon_tx.h
 *
 * Wirepas  Mesh  stack  is  capable  of  using  the  idle  time  between
 * network  protocol  radio transmissions to transmit Bluetooth LE compatible
 * beacons. The Beacon TX library provides an API for configuring each node to
 * transmit beacons.
 * It is recommended to read document WP-RM-103 - Wirepas Mesh Beacon API
 * Reference Manual for an introduction of beacon support in the Wirepas Mesh
 * stack.
 * The  Beacon  TX  library  is  not  available  on  all  radio  architectures,
 * only supported in 2.4GHz radio architectures.
 *
 * Library services are accessed  via @ref app_lib_beacon_tx_t "lib_beacon_tx"
 * handle.
 *
 * For examples on how to use these services, check out example application
 * @ref ble_beacon/app.c "ble_beacon".
 */
#ifndef APP_LIB_BEACON_TX_H_
#define APP_LIB_BEACON_TX_H_

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "wms_app.h"

/**
 * This is the library name for use with \ref
 * app_global_functions_t.openLibrary */
#define APP_LIB_BEACON_TX_NAME 0x0cb930b8 //!< "BCONTX"

/**
 * This is the library version for use with \ref
 * app_global_functions_t.openLibrary "*/
#define APP_LIB_BEACON_TX_VERSION 0x200

/**
 * The minimum amount of time, in milliseconds, between beacon transmissions.
 * Used as a parameter to \ref app_lib_beacon_tx_set_beacon_interval_f
 * "lib_beacon_tx->setBeaconInterval()".
 */
#define APP_LIB_BEACON_TX_MIN_INTERVAL 100

/**
 * The maximum amount of time, in milliseconds, between beacon transmissions.
 * Used as a parameter to to \ref app_lib_beacon_tx_set_beacon_interval_f
 * "lib_beacon_tx->setBeaconInterval()".
 */
#define APP_LIB_BEACON_TX_MAX_INTERVAL 60000

/**
 * The default interval of beacon transmissions, in milliseconds, to use if
 * \ref app_lib_beacon_tx_set_beacon_interval_f
 * "lib_beacon_tx->setBeaconInterval()" is not called.
 */
#define APP_LIB_BEACON_TX_DEFAULT_INTERVAL 1000

/**
 * Maximum beacon index for \ref app_lib_beacon_tx_set_beacon_power_f
 * "lib_beacon_tx->setBeaconPower()", \ref
 * app_lib_beacon_tx_set_beacon_channels_f "lib_beacon_tx->setBeaconChannels()"
 * and \ref app_lib_beacon_tx_set_beacon_contents_f
 * "lib_beacon_tx->setBeaconContents()". Each index is transmitted in a
 * round-robin fashion, separated by the interval, set by \ref
 * app_lib_beacon_tx_set_beacon_interval_f "lib_beacon_tx->setBeaconInterval()" .
 */
#define APP_LIB_BEACON_TX_MAX_INDEX 7

/**
 * Maximum number of bytes per beacon, for \ref
 * app_lib_beacon_tx_set_beacon_contents_f "lib_beacon_tx->setBeaconContents()".
 *
 * \note   Bluetooth advertising PDU has a maximum length of 39 bytes, but the
 *         radio driver inserts the length byte automatically, instead of
 *         having it as part of the buffer
 */
#define APP_LIB_BEACON_TX_MAX_NUM_BYTES (38)

/**
 * The type is used as a parameter to \ref
 * app_lib_beacon_tx_set_beacon_channels_f "lib_beacon_tx->setBeaconChannels()",
 * to select which advertising channels are used for sending the beacon payload.
 */
typedef enum
{
    /** Use advertising channel 37 */
    APP_LIB_BEACON_TX_CHANNELS_37 = 0,
    /** Use advertising channel 38 */
    APP_LIB_BEACON_TX_CHANNELS_38,
    /** Use advertising channel 39 */
    APP_LIB_BEACON_TX_CHANNELS_39,
    /** Use advertising channels 37 and 38 */
    APP_LIB_BEACON_TX_CHANNELS_37_38,
    /** Use advertising channels 37 and 39 */
    APP_LIB_BEACON_TX_CHANNELS_37_39,
    /** Use advertising channels 38 and 39 */
    APP_LIB_BEACON_TX_CHANNELS_38_39,
    /** Use advertising channels 37, 38 and 39 */
    APP_LIB_BEACON_TX_CHANNELS_37_38_39,
    /** Use advertising channels 37, 38 and 39 */
    APP_LIB_BEACON_TX_CHANNELS_ALL = APP_LIB_BEACON_TX_CHANNELS_37_38_39
} app_lib_beacon_tx_channels_mask_e;

/**
 * Clear all beacon contents and set all values to defaults:
 * - Beacon transmission disabled
 * - Interval: 1 s
 * - Power: maximum
 * - Channels: all
 * - Contents: cleared
 * \note    Default is not enabled, maximum power,
 *          \ref APP_LIB_BEACON_TX_DEFAULT_INTERVAL,
 *          \ref APP_LIB_BEACON_TX_CHANNELS_ALL and
 *          no payload set for any index
 * \return  always returns \ref APP_RES_OK
 */
typedef app_res_e
    (*app_lib_beacon_tx_clear_beacons_f)(void);

/**
 * Enable or disable beacon transmissions. A true value enables transmissions,
 * false value disables beacon transmissions. Usually the beacon parameters are
 * set up first and the transmission is enabled after that.
 *
 * \param   enabled
 *          New state, true for enabled, false otherwise
 * \return  Result code, \ref APP_RES_OK if successful,
 *          \ref APP_RES_INVALID_STACK_STATE if stack is not running
 * @note    Stack must be running (i.e. @ref app_lib_state_start_stack_f
 *          "lib_state->startStack()" must have been called) in order for
 *          enabling being possible.
 */
typedef app_res_e
    (*app_lib_beacon_tx_enable_beacons_f)(bool enabled);

/**
 * Set the interval of beacon transmissions. If more than one beacon is set up,
 * different beacon is transmitted after each interval.
 * \param   interval
 *          Sending interval in milliseconds
 * \return  Result code, \ref APP_RES_OK if successful,
 *          \ref APP_RES_INVALID_VALUE if interval is invalid,
 *          \ref APP_RES_INVALID_STACK_STATE if stack is not running
 * @note    Stack must be running (i.e. @ref app_lib_state_start_stack_f
 *          "lib_state->startStack()" must have been called) in order for
 *          set being possible.
 * \note    Beacons are sent on all enabled channels right after each other,
 *          for every interval, i.e. the more channels are enabled, the longer
 *          a transmission takes
 * \note    Different beacon payloads are sent on succeeding transmissions,
 *          \p interval milliseconds apart
 */
typedef app_res_e
    (*app_lib_beacon_tx_set_beacon_interval_f)(uint32_t interval);

/**
 * Set the output power of a beacon. The \p index parameter is the beacon number
 * to change, from 0 to \ref APP_LIB_BEACON_TX_MAX_INDEX. \p power_p parameter
 * is a pointer to the power value. The value is rounded to the nearest
 * available power and the new value is stored in the address pointed by \p
 * power_p.
 * \param   index
 *          Number of beacon (0 .. \ref APP_LIB_BEACON_TX_MAX_INDEX)
 * \param   power_p
 *          Pointer to radio transmission power, in dBm
 * \return  Result code, \ref APP_RES_OK if successful,
 *          \ref APP_RES_RESOURCE_UNAVAILABLE if index is too large
 *          \ref APP_RES_INVALID_STACK_STATE if stack is not running
 * @note    Stack must be running (i.e. @ref app_lib_state_start_stack_f
 *          "lib_state->startStack()" must have been called) in order for
 *          set being possible.
 * \note    Power is rounded to the closest possible value and the value
 *          pointed by \p power_p is modified to the used value
 */
typedef app_res_e
    (*app_lib_beacon_tx_set_beacon_power_f)(uint8_t index, int8_t * power_p);

/**
 * Set the channels to use for a beacon. The index parameter is the beacon
 * number to change, from 0 to \ref APP_LIB_BEACON_TX_MAX_INDEX. \p mask is the
 * channel mask, \ref app_lib_beacon_tx_channels_mask_e.
 * \param   index
 *          Number of beacon (0 .. \ref APP_LIB_BEACON_TX_MAX_INDEX)
 * \param   mask
 *          Channels to use for transmission
 * \return  Result code, \ref APP_RES_OK if successful,
 *          \ref APP_RES_RESOURCE_UNAVAILABLE if index is too large,
 *          \ref APP_RES_INVALID_VALUE if mask is invalid
 *          \ref APP_RES_INVALID_STACK_STATE if stack is not running
 * @note    Stack must be running (i.e. @ref app_lib_state_start_stack_f
 *          "lib_state->startStack()" must have been called) in order for
 *          set being possible.
 */
typedef app_res_e
    (*app_lib_beacon_tx_set_beacon_channels_f)(
        uint8_t index,
        app_lib_beacon_tx_channels_mask_e mask);

/**
 * Set beacon contents. The \p index parameter is the beacon number to change,
 * from 0 to \ref APP_LIB_BEACON_TX_MAX_INDEX. \p bytes is the beacon contents
 * to set and \p num_bytes is the number of bytes, or size, of the beacon.
 * Anatomy of a beacon is shown in WP-RM-103 - Wirepas Mesh Beacon API Reference
 * Manual.
 *
 * If more than one beacon index is set up, the beacons are transmitted one
 * after another, starting from index 0, separated by a gap set by the
 * \ref app_lib_beacon_tx_set_beacon_interval_f
 * "lib_beacon_tx->setBeaconInterval()" function. Setting bytes to NULL or \p
 * num_bytes to 0 disables the beacon for that index.
 * \param   index
 *          Number of beacon (0 .. \ref APP_LIB_BEACON_TX_MAX_INDEX)
 * \param   bytes
 *          Beacon payload, or NULL to disable payload for this index
 * \param   num_bytes
 *          Payload size in bytes, or 0 to disable payload for this index
 * \return  Result code, \ref APP_RES_OK if successful,
 *          \ref APP_RES_RESOURCE_UNAVAILABLE if index is too large,
 *          \ref APP_RES_INVALID_VALUE if num_bytes is invalid
 *          \ref APP_RES_INVALID_STACK_STATE if stack is not running
 * @note    Stack must be running (i.e. @ref app_lib_state_start_stack_f
 *          "lib_state->startStack()" must have been called) in order for
 *          set being possible.
 */
typedef app_res_e
    (*app_lib_beacon_tx_set_beacon_contents_f)(uint_fast8_t index,
                                               const uint8_t *bytes,
                                               size_t num_bytes);

/**
 * The function table returned from \ref app_open_library_f
 */
typedef struct
{
    app_lib_beacon_tx_clear_beacons_f           clearBeacons;
    app_lib_beacon_tx_enable_beacons_f          enableBeacons;
    app_lib_beacon_tx_set_beacon_interval_f     setBeaconInterval;
    app_lib_beacon_tx_set_beacon_power_f        setBeaconPower;
    app_lib_beacon_tx_set_beacon_channels_f     setBeaconChannels;
    app_lib_beacon_tx_set_beacon_contents_f     setBeaconContents;
} app_lib_beacon_tx_t;

#endif /* APP_LIB_BEACON_TX_H_ */
