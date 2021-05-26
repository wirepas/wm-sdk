/* Copyright 2017 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/**
 * \file data.h
 *
 * The Data library contains functions for sending and receiving data packets.
 * Also contained within are functions for sending and receiving app config
 * data, which is a small bit of data that gets distributed to all nodes on the
 * network. Any new nodes will receive app config data quickly during joining
 * process to the network. App config very lightweight and can also be
 * considered as 'network persistent data'.
 *
 * Library services are accessed  via @ref app_lib_data_t "lib_data" handle.
 */
#ifndef APP_LIB_DATA_H_
#define APP_LIB_DATA_H_

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "app/app.h"

/** \brief Library symbolic name  */
#define APP_LIB_DATA_NAME 0x0003f161 //!< "DATA"

/** \brief Maximum supported library version */
#define APP_LIB_DATA_VERSION    0x209

/**
 * @brief Type of tracking ID for data packets
 *
 * A type of tracking ID for sent data packets, to keep track of packets sent
 * through local processing, until packet is finally sent or discarded. The
 * valid range for this id is [0, 65534 (0xFFFF -1) ]. The value 65535 (0xFFFF)
 * is used to disable the id tracking (@ref APP_LIB_DATA_NO_TRACKING_ID).
 *
 * @note Tracking ID is used only locally for the communication between the
 * application layer and the stack. These are not actually transmitted on the
 * network. Only 16 requests where tracking is active are allowed at the same
 * time. Without tracking, there is room for plenty of more requests
 * simultaneously.
 *
 * @note In order for this to be activate, flag @ref
 * APP_LIB_DATA_SEND_FLAG_TRACK must be set in arguments when calling @ref
 * app_lib_data_send_data_f "lib_data->sendData".
 */
typedef uint16_t app_lib_data_tracking_id_t;

/**
 * @brief   When sending data and no tracking of packet is requested, this ID
 *          may be used.
 *
 * Used with service @ref app_lib_data_send_data_f "lib_data->sendData".
 */
#define APP_LIB_DATA_NO_TRACKING_ID (app_lib_data_tracking_id_t)(-1)

/**
 * @brief size of app config
 *
 * This is a safe size to use for app config data static buffers. Use
 * @ref app_lib_data_get_app_config_num_bytes_f
 * "lib_data->getAppConfigNumBytes()" to get actual app config data size in
 * bytes, which may be smaller.
 *
 * Used with services  @ref app_lib_data_read_app_config_f
 * "lib_data->readAppConfig()", @ref app_lib_data_write_app_config_f
 * "lib_data->writeAppConfig()" and in callback function set with function
 * @ref app_lib_data_set_new_app_config_cb_f "lib_data->setNewAppConfigCb()".
 */
#define APP_LIB_DATA_MAX_APP_CONFIG_NUM_BYTES   80

/**
 * @brief Data quality of service class. Used when sending and receiving data
 * packets.
 *
 * Used with service @ref app_lib_data_send_data_f "lib_data->sendData()" and
 * callback functions set with services @ref app_lib_data_set_data_received_cb_f
 * "lib_data->setDataReceivedCb()" and @ref
 * app_lib_data_set_data_received_cb_f
 * "lib_data->setBcastDataReceivedCb()".
 */
typedef enum
{
    /** Normal quality of service */
    APP_LIB_DATA_QOS_NORMAL = 0,
    /** High quality of service, i.e. takes priority over @ref
     * APP_LIB_DATA_QOS_NORMAL quality of service packets */
    APP_LIB_DATA_QOS_HIGH   = 1,
} app_lib_data_qos_e;

/**
 * @brief Flags to use with @ref app_lib_data_send_data_f "lib_data->sendData()"
 */
typedef enum
{
    /** Default value, no tracking of packet */
    APP_LIB_DATA_SEND_FLAG_NONE   = 0,
    /** Track packet through local processing, i.e. call tracking callback (see
     * @ref app_lib_data_set_data_sent_cb_f "lib_data->setDataSentCb()") when
     * packet is finally sent or discarded. */
    APP_LIB_DATA_SEND_FLAG_TRACK  = 1,
    /** @ref app_lib_data_to_send_t.hop_limit "hop_limit" field in transmission
     * definition contains the value of hop limit to be used in sending.
     *
     * Hop limit sets the upper value to the number of hops executed for packet
     * to reach the destination. By using hop limiting, it is possible to limit
     * the distance how far the packet is transmitted to and avoiding causing
     * unnecessary traffic to network. Hop limit value of 0 is used to disable
     * the hop limiting. Hop limiting value does not have any impact when using
     * @ref APP_ADDR_ANYSINK address as destination node address but is
     * discarded.
     * */
    APP_LIB_DATA_SEND_SET_HOP_LIMITING = 4,
    /** The unacknowledged CSMA-CA transmission method can be used in a mixed
     * network (i.e. network consisting of both CSMA-CA and time-slotted mode
     * devices) by CSMA-CA device originated packets transmission only to
     * CSMA-CA devices. The purpose of this method is to avoid a performance
     * bottleneck by NOT transmitting to time-slotted mode devices. Note:
     * when using this flag, transmission is always sent beyond sink routing
     * tree */
    APP_LIB_DATA_SEND_FLAG_UNACK_CSMA_CA = 8,
    /** Send packet on network channel only. Note, when this is set, it
     * overrides any hop limit definitions the packet otherwise has. These
     * packets are never rerouted.
     * When this flag is used, ONLY devices that are scanning at the moment will
     * receive the packet. Could be used, for example, in asset tracking where
     * devices are scanning network channel.*/
    APP_LIB_DATA_SEND_NW_CH_ONLY = 16,
} app_lib_data_send_flags_e;

/**
 * @brief A result code returned from @ref app_lib_data_send_data_f
 * "lib_data->sendData()"
 */
typedef enum
{
    /** Data was accepted in stack buffers */
    APP_LIB_DATA_SEND_RES_SUCCESS              = 0,
    /** Error: stack is not running */
    APP_LIB_DATA_SEND_RES_INVALID_STACK_STATE  = 1,
    /** Error: QoS parameter is invalid */
    APP_LIB_DATA_SEND_RES_INVALID_QOS          = 2,
    /** Error: flags parameter is invalid */
    APP_LIB_DATA_SEND_RES_INVALID_FLAGS        = 3,
    /** Error: there is no space for data in stack buffers */
    APP_LIB_DATA_SEND_RES_OUT_OF_MEMORY        = 4,
    /** Error: destination address parameter is invalid. Special case is for
     * advertiser role where device has tried to send uplink (@ref
     * APP_ADDR_ANYSINK) or tried to send unicast packet to target that is not
     * known.*/
    APP_LIB_DATA_SEND_RES_INVALID_DEST_ADDRESS = 5,
    /** Error: number of bytes parameter is invalid */
    APP_LIB_DATA_SEND_RES_INVALID_NUM_BYTES    = 6,
    /** Error: tracking ID already in use, or there are no more tracking IDs
     * available */
    APP_LIB_DATA_SEND_RES_OUT_OF_TRACKING_IDS  = 7,
    /** Tracking ID already in use or invalid ID */
    APP_LIB_DATA_SEND_RES_INVALID_TRACKING_ID  = 8,
    /** Error: one of the @ref endpoint "endpoints" is invalid, reserved for
     * stack internal use */
    APP_LIB_DATA_SEND_RES_RESERVED_ENDPOINT    = 9,
    /** Error: data sending is forbidden, either:
     * - Disabled by feature lock bits
     * - Sending to prohibited end-point or destination, when joining a network
     *   via open joining */
    APP_LIB_DATA_SEND_RES_ACCESS_DENIED        = 10,
    /** Error: Hop limit value is invalid */
    APP_LIB_DATA_SEND_RES_INVALID_HOP_LIMIT    = 11,
} app_lib_data_send_res_e;

/**
 * @brief Return value of data reception callback
 *
 * This result code needs to be returned from the data reception callback
 * functions set with services @ref app_lib_data_set_data_received_cb_f
 * "lib_data->setDataReceivedCb()" and @ref
 * app_lib_data_set_data_received_cb_f "lib_data->setBcastDataReceivedCb()".
 */
typedef enum
{
    /** Packet was for the application and it was handled successfully. Stack
     * may now discard the packet. */
    APP_LIB_DATA_RECEIVE_RES_HANDLED     = 0,
    /** Packet was not for the application. Stack may offer the packet to some
     * other module, if present, or discard it. */
    APP_LIB_DATA_RECEIVE_RES_NOT_FOR_APP = 1,
    /** Packet was for the application, but the application cannot handle it
     * right now. Stack is requested to keep the packet in its buffers, until
     * @ref app_lib_data_allow_reception_f "lib_data->allowReception(true)" is
     * called. */
    APP_LIB_DATA_RECEIVE_RES_NO_SPACE    = 2,
} app_lib_data_receive_res_e;

/**
 * @brief Result of the app config
 *
 * A result code returned from @ref app_lib_data_read_app_config_f
 * "lib_data->readAppConfig()" and parameter for @ref
 * app_lib_data_write_app_config_f "lib_data->writeAppConfig()"
 */
typedef enum
{
    /** Reading or writing app config data was successful */
    APP_LIB_DATA_APP_CONFIG_RES_SUCCESS              = 0,
    /** Error: cannot write app config data: node is not a sink */
    APP_LIB_DATA_APP_CONFIG_RES_INVALID_ROLE         = 1,
    /** Error: cannot read app config data: no app config data set or received
    */
    APP_LIB_DATA_APP_CONFIG_RES_INVALID_APP_CONFIG   = 2,
    /** Error: invalid sequence number parameter */
    APP_LIB_DATA_APP_CONFIG_RES_INVALID_SEQ          = 3,
    /** Error: invalid interval parameter */
    APP_LIB_DATA_APP_CONFIG_RES_INVALID_INTERVAL     = 4,
    /** Error: invalid NULL pointer parameter */
    APP_LIB_DATA_APP_CONFIG_RES_INVALID_NULL_POINTER = 5,
} app_lib_data_app_config_res_e;

/**
 * @brief Unable to determine hop count
 *
 * This macro declares special value represented in hops- field of
 * @ref app_lib_data_received_t structure when device has been
 * unable to determine the hop count.
 *
 * Used in data reception callback functions set with services @ref
 * app_lib_data_set_data_received_cb_f "lib_data->setDataReceivedCb()" and @ref
 * app_lib_data_set_data_received_cb_f "lib_data->setBcastDataReceivedCb()".
 */
#define APP_LIB_DATA_RX_HOPS_UNDETERMINED 0

/**
 * @brief Struct passed to data reception callback functions
 *
 * This struct is passed to the data reception callback functions set with
 * services @ref app_lib_data_set_data_received_cb_f
 * "lib_data->setDataReceivedCb()" and @ref app_lib_data_set_data_received_cb_f
 * "lib_data->setBcastDataReceivedCb()".
 */
typedef struct
{
    /** Received bytes */
    const uint8_t * bytes;
    /** Number of bytes received */
    size_t num_bytes;
    /** Address of node that sent the packet */
    app_addr_t src_address;
    /** End-to-end transmission delay, in 1 / 128 of seconds. This also
     *  includes the value set in the delay field for @ref
     *  app_lib_data_send_data_f "lib_data->sendData"*/
    uint32_t delay;
    /** Packet quality of service class, see @ref app_lib_data_qos_e */
    app_lib_data_qos_e qos;
    /** Source @ref endpoint "endpoint" of packet */
    uint8_t src_endpoint;
    /** Destination @ref endpoint "endpoint" of packet */
    uint8_t dest_endpoint;
    /** Amount of hops that were used when routing packet to the destination */
    uint8_t hops;
    /** Destination address for reception. For unicast receptions, own address.
     *  For broadcast receptions, @ref APP_ADDR_BROADCAST. For multicast
     *  receptions, group address (with @ref APP_ADDR_MULTICAST bitmask set) */
    app_addr_t dest_address;
    /** Mac source address, i.e. previous hop of the packet. Which device
     *  transmitted the packet. tx_power and rssi fields apply for this device*/
    app_addr_t mac_src_address;
    /** transmit power in dBm (of previous hop). Note: if loopback message
     * (transmission to itself), this is meaningless because packet is not sent
     * via radio at all.*/
    int8_t tx_power;
    /** received signal strength (in dBm). Note: if loopback message
     * (transmission to itself), this is meaningless because packet is not sent
     * via radio at all.*/
    int8_t rssi;
    /** End-to-end transmission delay, in 1 / 1024 of seconds. This also
     *  includes the value set in the delay field for @ref
     *  app_lib_data_send_data_f "lib_data->sendData"*/
    uint32_t delay_hp;
} app_lib_data_received_t;

/**
 * @brief A struct for @ref app_lib_data_send_data_f "lib_data->sendData()"
 */
typedef struct
{
    /** Bytes to send */
    const uint8_t * bytes;
    /** Number of bytes to send */
    size_t num_bytes;
    /** Destination address of packet */
    app_addr_t dest_address;
    /** Initial end-to-end transmission delay, in 1 / 128 seconds. This could
     *  be used, for example, to represent actual measurement time if done
     *  earlier but generated for transmission on later time moment. */
    uint32_t delay;
    /**
     * Packet tracking ID
     */
    app_lib_data_tracking_id_t tracking_id;
    /** Packet quality of service class */
    app_lib_data_qos_e qos;
    /** Send flags, see @ref app_lib_data_send_flags_e */
    uint8_t flags;
    /** Source @ref endpoint "endpoint" of packet */
    uint8_t src_endpoint;
    /** Destination @ref endpoint "endpoint" of packet */
    uint8_t dest_endpoint;
    /** Maximum amount of hops allowed for transmission. Requires also flag
     * @ref APP_LIB_DATA_SEND_SET_HOP_LIMITING to be set in flags field in order
     * to be active. When used, value must be >0. */
    uint8_t hop_limit;
} app_lib_data_to_send_t;

/**
 * @brief Struct to tracking callback function
 *
 * This struct is passed to the tracking callback function (set with service
 * @ref app_lib_data_data_sent_cb_f "lib_data->setDataSentCb()") when a packet
 * is either sent or discarded.
 */
typedef struct
{
    /** Destination address of packet */
    app_addr_t dest_address;
    /** Time the packet spent in the local buffer, in 1 / 128 seconds. This also
     * includes the value set in the delay field for @ref
     * app_lib_data_send_data_f "lib_data->sendData".
    */
    uint32_t queue_time;
    /**
     * Packet tracking ID to distinguish which packet was sent */
    app_lib_data_tracking_id_t tracking_id;
    /** Source @ref endpoint "endpoint" of packet */
    uint8_t src_endpoint;
    /** Destination @ref endpoint "endpoint" of packet */
    uint8_t dest_endpoint;
    /** True if packet was sent, false if packet was discarded */
    bool success;
} app_lib_data_sent_status_t;

/**
 * @brief Data reception callback.
 *
 * The application sets a data reception
 * callback by calling either @ref app_lib_data_set_data_received_cb_f
 * "lib_data->setDataReceivedCb()" or @ref app_lib_data_set_data_received_cb_f
 * "lib_data->setBcastDataReceivedCb()".
 *
 * The received packet is represented as a pointer to @ref
 * app_lib_data_received_t struct. Depending on the return value, the stack
 * either keeps or discards the packet.
 *
 * \param   data
 *          Received data
 * \return  Result code, @ref app_lib_data_receive_res_e
 */
typedef app_lib_data_receive_res_e
    (*app_lib_data_data_received_cb_f)(const app_lib_data_received_t * data);

/**
 * @brief Sent packet tracking callback.
 *
 * The application sets a tracking callback
 * by calling @ref app_lib_data_set_data_sent_cb_f "lib_data->setDataSentCb()".
 *
 * @param   status
 *          Status of the sent packet
 */
typedef void
    (*app_lib_data_data_sent_cb_f)(const app_lib_data_sent_status_t * status);

/**
 * @brief Type of the new app config callback function.
 *
 * New app config callback is called whenever new app config is received and
 * when the node first joins a network. There is no return value from the
 * callback.
 *
 * The application sets a new app config callback by calling @ref
 * app_lib_data_set_new_app_config_cb_f "lib_data->setNewAppConfigCb()".
 *
 * \param   bytes
 *          New app config data
 * \param   seq
 *          New app config data sequence number
 * \param   interval
 *          New app config data diagnostic interval, in seconds
 */
typedef void (*app_lib_data_new_app_config_cb_f)(const uint8_t * bytes,
                                                 uint8_t seq,
                                                 uint16_t interval);

/**
 * @brief Set data reception callback
 *
 * Set the callback function to be called when new data is received. There are
 * two distinct callbacks: one for receiving unicast data is data that is
 * addressed directly to a specific node (calling @ref
 * app_lib_data_set_data_received_cb_f "lib_data->setDataReceivedCb()") and
 * another one for receiving multicast and broadcasts (calling @ref
 * app_lib_data_set_data_received_cb_f "lib_data->setBcastDataReceivedCb()").
 * If NULL is passed, the callback is disabled.
 *
 * Example on use. Application handles destination @ref endpoint "endpoint" of
 * 12 as incoming data and triggers temperature measurement.
 * @code
 *
 * #define GET_TEMPERATURE_EP  12
 *
 * static app_lib_data_receive_res_e dataReceivedCb(
 *     const app_lib_data_received_t * data)
 * {
 *     if (data->dest_endpoint == GET_TEMPERATURE_EP)
 *     {
 *         //start_temperature_measurement();
 *         return APP_LIB_DATA_RECEIVE_RES_HANDLED;
 *     }
 *
 *     return APP_LIB_DATA_RECEIVE_RES_NOT_FOR_APP;
 * }
 *
 * void App_init(const app_global_functions_t * functions)
 * {
 *     // Register for unicast, multicast & broadcast messages
 *     lib_data->setDataReceivedCb(dataReceivedCb);
 *     lib_data->setBcastDataReceivedCb(dataReceivedCb);
 *
 *     // Start the stack
 *     lib_state->startStack();
 * }
 * @endcode
 *
 * \param   cb
 *          The function to be executed, or NULL to unset
 * \return  Result code, always @ref APP_RES_OK
 */
typedef app_res_e
    (*app_lib_data_set_data_received_cb_f)(app_lib_data_data_received_cb_f cb);

/**
 * @brief This service has been DEPRACATED!
 *
 * \return  \ref APP_RES_RESOURCE_UNAVAILABLE
 */
typedef app_res_e (*app_lib_data_depracated_f)(void);

/**
 * @brief Set data sent tracking callback
 *
 * Set the callback function to be called when a packet has gone through local
 * processing and has finally been sent or discarded. If NULL is passed, the
 * callback is disabled
 *
 * Example:
 * @code
 * static void cb_data_ack(const app_lib_data_sent_status_t * status)
 * {
 *     (void) status;
 *     // Here add operation to track when packet has been sent
 * }
 *
 * void App_init(const app_global_functions_t* functions)
 * {
 *     lib_data->setDataSentCb(cb_data_ack);
 *     // When lib_data->sendData is called with tracking, callback is called
 *     // start the stack
 *     lib_state->startStack();
 * }
 * @endcode
 *
 * \param   cb
 *          The function to be executed, or NULL to unset
 * \return  Result code, always @ref APP_RES_OK
 */
typedef app_res_e
    (*app_lib_data_set_data_sent_cb_f)(app_lib_data_data_sent_cb_f cb);

/**
 * @brief   Set the callback function to be called when new @ref appconfig
 *          "app config data" is received.
 *
 * Example: see example application @ref appconfig_app/app.c "appconfig_app"
 *
 * \param   cb
 *          The function to be executed, or NULL to unset
 * \return  Result code, always @ref APP_RES_OK
 */
typedef app_res_e
    (*app_lib_data_set_new_app_config_cb_f)
    (app_lib_data_new_app_config_cb_f cb);

/**
 * @brief Return the maximum number of bytes per data packet
 *
 * Different platforms have
 * differently sized radio packets, so this value varies by platform.
 *
 * Example:
 * @code
 * size_t maxlen;
 * maxlen = lib_data->getDataMaxNumBytes();
 * @endcode
 *
 * \return  Maximum size of data packet in bytes
 */
typedef size_t
    (*app_lib_data_get_data_max_num_bytes_f)(void);

/**
 * @brief Get total number of packet buffers
 *
 * Return the total number of packet buffers for sent data that are tracked. See
 * @ref app_lib_data_get_num_free_buffers_f "lib_data->getNumFreeBuffers()"
 * for reading the number of available buffers. This function applies for
 * transmitted packets that are tracked.
 *
 * Example to use:
 * @code
 * size_t total_tracked = lib_data->getNumBuffers();
 * @endcode
 *
 * \return  Total number of data packets that can be buffered
 * \note    When packets are send with @ref app_lib_data_send_data_f
 *          "lib_data->sendData()" without tracking (tracking_id == @ref
 *          APP_LIB_DATA_NO_TRACKING_ID), there are plenty of more buffers
 *          available than with tracking enabled. For tracking enabled, there
 *          is only 16 buffers available.
 */
typedef size_t
    (*app_lib_data_get_num_buffers_f)(void);

/**
 * @brief Get number of currently available buffers
 *
 * Query the number of currently available buffers for sending data with
 * tracking enabled. If there are no buffers left, sending data is not possible.
 * Function @ref app_lib_data_get_num_buffers_f "lib_data->getNumBuffers()"
 * returns the maximum number of buffers available. This function applies for
 * transmitted packets that are tracked.
 *
 * Example:
 * @code
 * size_t num_buffers;
 * lib_data->getNumBuffers(@num_buffers);
 * @endcode
 *
 * \param   num_buffers_p
 *          Pointer to a value where the number of data packets
 *          that can be buffered will be stored
 * \return  Result code, @ref APP_RES_OK if stack is running, otherwise
 *          @ref APP_RES_INVALID_STACK_STATE
 * \note    When packets are send with @ref app_lib_data_send_data_f
 *          "lib_data->sendData()" without tracking (tracking_id == @ref
 *          APP_LIB_DATA_NO_TRACKING_ID), there are plenty of more buffers
 *          available than with tracking enabled. For tracking enabled, there
 *          is only 16 buffers available.
 */
typedef app_res_e
    (*app_lib_data_get_num_free_buffers_f)(size_t * num_buffers_p);

/**
 * @brief Send data
 *
 * The packet to send is represented as @ref app_lib_data_to_send_t
 * struct.
 *
 * Example: See example application @ref custom_app/app.c "custom_app"
 *
 * \param   data
 *          Data to send
 * \return  Result code, @ref APP_LIB_DATA_SEND_RES_SUCCESS means that data
 *          was accepted for sending. See @ref app_lib_data_send_res_e for
 *          other result codes.
 */
typedef app_lib_data_send_res_e
    (*app_lib_data_send_data_f)(const app_lib_data_to_send_t * data);

/**
 * @brief Allow or block reception
 * The application may temporarily tell the stack to
 * not call the reception data reception callbacks (set with services @ref
 * app_lib_data_set_data_received_cb_f "lib_data->setDataReceivedCb()" and @ref
 * app_lib_data_set_data_received_cb_f "lib_data->setBcastDataReceivedCb()"),
 * if an external interface is not
 * responding, for example. The stack will then keep the received packets in its
 * internal buffers until reception is allowed again.
 *
 * If the reception callback returns @ref APP_LIB_DATA_RECEIVE_RES_NO_SPACE, it
 * is the same as calling @ref app_lib_data_allow_reception_f
 * "lib_data->allowReception(false)".
 *
 * Example of use:
 * @code
 * lib_data->allowReception(true);
 * @endcode
 *
 * \param   allow
 *          True if it is OK to call the reception callback, false otherwise
 * \note    If reception is not allowed and there is buffered data, calling
 *          this function with a parameter of true will cause the reception
 *          callback to be called right away
 */
typedef void
    (*app_lib_data_allow_reception_f)(bool allow);

/**
 * \brief   Read @ref appconfig "app config"
 * \param   bytes
 *          Pointer to a buffer for app config data
 * \param   seq
 *          Pointer to app config sequence
 * \param   interval
 *          Pointer to diagnostic interval in seconds
 * \return  Result code, @ref APP_LIB_DATA_APP_CONFIG_RES_SUCCESS if
 *          successful.  See @ref app_lib_data_app_config_res_e for
 *          other result codes.
 *
 * Example:
 * @code
 * uint8_t appconfig[APP_LIB_DATA_MAX_APP_CONFIG_NUM_BYTES];
 * uint8_t appconfig_seq;
 * uint16_t appconfig_interval;
 * lib_data->readAppConfig(&appconfig[0],
 *                         &appconfig_seq,
 *                         &appconfig_interval);
 * @endcode
 */
typedef app_lib_data_app_config_res_e
    (*app_lib_data_read_app_config_f)(uint8_t * bytes,
                                      uint8_t * seq,
                                      uint16_t * interval);

/**
 * \brief   Write @ref appconfig "app config"
 * \param   bytes
 *          Pointer to app config data to write. The format can be decided by
 *          the application.
 * \param   seq
 *          Sequence number for filtering old and already received application
 *          configuration data packets at the nodes. The sequence number must
 *          be increment by 1 every time new configuration is written, i.e.
 *          new diagnostic data interval and/or new application configuration
 *          data is updated. A sequence number that is the current value of
 *          existing application configuration data is invalid. A value of 255
 *          is invalid. Therefore, after value of 254, the next valid value is
 *          0.
 * \param   interval
 *          Diagnostic data transmission interval in seconds, i.e. how often
 *          the nodes on the network should send diagnostic PDUs. If the value
 *          is 0, diagnostic data transmission is disabled.
 *          Valid values are: 0 (default), 30, 60, 120, 300, 600 and 1800.
 *
 * \return  Result code, @ref APP_LIB_DATA_APP_CONFIG_RES_SUCCESS if
 *          successful.  See @ref app_lib_data_app_config_res_e for
 *          other result codes.
 *
 * @note    It is recommended that the configuration data is not written too
 *          often, as new configuration data is always written to the non-
 *          volatile memory of the sink and disseminated to the network. This
 *          can cause unnecessary wearing of the memory with devices that
 *          need to use the program memory to store persistent variables and
 *          unnecessary load to the network.
 * @note    This service is deprecated and has been replaced by
 *          @ref app_lib_data_write_app_config_data_f "writeAppConfigData" and
 *          @ref app_lib_data_write_diagnostic_interval_f "writeDiagnosticInterval"
 *          that manages sequence increment automatically.
 *
 * Example:
 * @code
 * uint8_t appconfig[APP_LIB_DATA_MAX_APP_CONFIG_NUM_BYTES];
 * uint8_t appconfig_seq;
 * uint16_t appconfig_interval;
 * lib_data->readAppConfig(&appconfig[0],
 *                         &appconfig_seq,
 *                         &appconfig_interval);
 *
 * // Use otherwise same appconfig but set diagnostics to 30s
 * appconfig_seq++;
 * if (appconfig_seq == 255)
 * {
 *     appconfig_seq = 0;
 * }
 * lib_data->writeAppConfig(&appconfig[0],
 *                         &appconfig_seq,
 *                         30);
 * @endcode
 */
typedef app_lib_data_app_config_res_e
    (*app_lib_data_write_app_config_f)(const uint8_t * bytes,
                                       uint8_t seq,
                                       uint16_t interval);

/**
 * \brief   Get size of @ref appconfig "app config"
 * \return  App config size in bytes
 *
 * Example of use:
 * @code
 * size_t app_config_size = lib_data->getAppConfigNumBytes();
 * @endcode
 */
typedef size_t
    (*app_lib_data_get_app_config_num_bytes_f)(void);


/**
 * \brief   Set maximum queuing time for messages
 * \param   priority
 *          Message priority which queuing time to be set
 * \param   time
 *          Queuing time in seconds. Accepted range: 2 - 65534s.
 *          Select queuing time carefully, too short value might cause
 *          unnecessary message drops and too big value filling up message
 *          queues. For consistent performance it is recommended to use the
 *          same queuing time in the whole network.
 *
 * \note    Minimum queuing time shall be bigger than access cycle
 *          interval in time-slotted mode networks. It is recommended to use
 *          multiples of access cycle interval (+ extra) to give time for
 *          message repetitions, higher priority messages taking over the access
 *          slot etc. Access cycle is not limiting the minimum value in
 *          CSMA-CA networks.
 * \return  Result code, @ref APP_RES_OK if successful
 *          @ref APP_RES_INVALID_VALUE if unsupported message priority or time
 *
 * Example:
 * @code
 * // Set queueing time for low priority to 5 seconds
 * lib_data->setMaxMsgQueuingTime(APP_LIB_DATA_QOS_NORMAL, 5);
 * @endcode
 */
typedef app_res_e
    (*app_lib_data_set_max_msg_queuing_time_f)(app_lib_data_qos_e priority,
                                               uint16_t time);

/**
 * \brief   Get maximum queuing time of messages
 * \param   priority
 *          Message priority which queuing time to be read
 * \param   time_p
 *          Pointer where to store maximum queuing time
 * \return  Result code, @ref APP_RES_OK if ok,
 *          @ref APP_RES_INVALID_VALUE if unsupported message priority,
 *          @ref APP_RES_INVALID_NULL_POINTER if @p time_p is null
 *
 * Example:
 * @code
 * uint16_t qos_normal_qt;
 * lib_data->getMaxMsgQueuingTime(APP_LIB_DATA_QOS_NORMAL, &qos_normal_qt);
 * @endcode
 */
typedef app_res_e
    (*app_lib_data_get_max_msg_queuing_time_f)(app_lib_data_qos_e priority,
                                               uint16_t * time_p);

/**
 * \brief   Write @ref appconfig "app config DATA"
 * \param   bytes
 *          Pointer to app config data to write. The format can be decided by
 *          the application.
 *
 * \return  Result code, @ref APP_LIB_DATA_APP_CONFIG_RES_SUCCESS if
 *          successful.  See @ref app_lib_data_app_config_res_e for
 *          other result codes.
 *
 * @note    It is recommended that the app config data is not written too
 *          often, as new configuration is always written to the non-
 *          volatile memory of the sink and disseminated to the network. This
 *          can cause unnecessary wearing of the memory with devices that
 *          need to use the program memory to store persistent variables and
 *          unnecessary load to the network.
 *
 * Example:
 * @code
 * uint8_t appconfig[APP_LIB_DATA_MAX_APP_CONFIG_NUM_BYTES];
 *
 * lib_data->writeAppConfigData(&appconfig[0]);
 * @endcode
 */
typedef app_lib_data_app_config_res_e
    (*app_lib_data_write_app_config_data_f)(const uint8_t * bytes);

/**
 * \brief   Write @ref appconfig "Diagnostic interval"
 * \param   interval
 *          Diagnostic data transmission interval in seconds, i.e. how often
 *          the nodes on the network should send diagnostic PDUs. If the value
 *          is 0, diagnostic data transmission is disabled.
 *          Valid values are: 0 (default), 30, 60, 120, 300, 600 and 1800.
 *
 * \return  Result code, @ref APP_LIB_DATA_APP_CONFIG_RES_SUCCESS if
 *          successful.  See @ref app_lib_data_app_config_res_e for
 *          other result codes.
 *
 * @note    It is recommended that the diagnostic interval is not written too
 *          often, as new configuration is always written to the non-
 *          volatile memory of the sink and disseminated to the network. This
 *          can cause unnecessary wearing of the memory with devices that
 *          need to use the program memory to store persistent variables and
 *          unnecessary load to the network.
 *
 * Example:
 * @code
 * lib_data->writeDiagnosticInterval(30);
 * @endcode
 */
typedef app_lib_data_app_config_res_e
    (*app_lib_data_write_diagnostic_interval_f)(uint16_t interval);

/**
 * \brief       List of library services
 */
typedef struct
{
    app_lib_data_set_data_received_cb_f setDataReceivedCb;
    app_lib_data_set_data_received_cb_f setBcastDataReceivedCb;
    app_lib_data_set_data_sent_cb_f setDataSentCb;
    app_lib_data_set_new_app_config_cb_f setNewAppConfigCb;
    app_lib_data_get_data_max_num_bytes_f getDataMaxNumBytes;
    app_lib_data_get_num_buffers_f getNumBuffers;
    app_lib_data_get_num_free_buffers_f getNumFreeBuffers;
    app_lib_data_send_data_f sendData;
    app_lib_data_allow_reception_f allowReception;
    app_lib_data_read_app_config_f readAppConfig;
    app_lib_data_write_app_config_f writeAppConfig;
    app_lib_data_get_app_config_num_bytes_f getAppConfigNumBytes;
    app_lib_data_depracated_f reserved;
    app_lib_data_set_max_msg_queuing_time_f  setMaxMsgQueuingTime;
    app_lib_data_get_max_msg_queuing_time_f  getMaxMsgQueuingTime;
    app_lib_data_write_app_config_data_f writeAppConfigData;
    app_lib_data_write_diagnostic_interval_f writeDiagnosticInterval;
} app_lib_data_t;

#endif /* APP_LIB_DATA_H_ */
