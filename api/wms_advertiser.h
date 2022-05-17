/* Copyright 2018 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/**
 * \file wms_advertiser.h
 *
 * Application library for direct advertiser functionality.
 *
 * Library services are accessed  via @ref app_lib_advertiser_t "lib_advertiser"
 * handle.
 *
 * For examples on how to use these services, check out
 * @ref diradv_example/app.c "example application"
 */
#ifndef APP_LIB_ADVERTISER_H_
#define APP_LIB_ADVERTISER_H_

#include <stdint.h>
#include <stdbool.h>

#include "wms_app.h"

/** \brief Library symbolic name  */
#define APP_LIB_ADVERTISER_NAME 0x06cc5e24 //!< "ADVERT"

/** \brief Maximum supported library version */
#define APP_LIB_ADVERTISER_VERSION 0x205

/**
 * \brief Headnode acknowledges the packet by using this source endpoint
 *
 * Used in headnode callback function set with  @ref
 * app_lib_advertiser_ackdatacb_f "lib_advertiser->setBeaconDataAckListenCb()"
 * and receiving packet in advertiser in callback function set with @ref
 * app_lib_data_set_data_received_cb_f "lib_data->setDataReceivedCb()"
 */
#define DIRADV_EP_SRC_ACK  249
/**
 * \brief  Headnode acknowledges the packet by using this destination endpoint
 *
 * Used in headnode callback function set with  @ref
 * app_lib_advertiser_ackdatacb_f "lib_advertiser->setBeaconDataAckListenCb()"
 * and receiving packet in advertiser in callback function set with @ref
 * app_lib_data_set_data_received_cb_f "lib_data->setDataReceivedCb()"
 */
#define DIRADV_EP_DEST     248

/**
 * \brief  Absolute maximum size of the ACK message
 *
 * This can be used for allocating memory structures for callback function set
 * with @ref app_lib_advertiser_ackdatacb_f
 * "lib_advertiser->setBeaconDataAckListenCb()".
 */
#define DIRADV_MAX_ACK_LEN  102

/**
 * \brief   Default value used with @ref app_lib_advertiser_set_queuing_time_f
 *          "lib_advertiser->setQueuingTimeHp()"
 */
#define DIRADV_DEFAULT_QUEUE_TIME   250

/**
 * \brief       Input data structure for callback function set by @ref
 *              app_lib_advertiser_ackdatacb_f
 *              "lib_advertiser->setRouterAckGenCb()"
 */
typedef struct
{
    /** Address of the sender (advertiser) */
    app_addr_t sender;
    /** Max len of the ack payload supported by transmitter whom the ack is sent
     */
    uint8_t ack_length;
    /** Source endpoint of packet. Used to distinguish different applications
     * sending advertiser packets.
     */
    uint8_t src_endpoint;
    /** Destination endpoint of packet. Used to distinguish different
     * applications sending advertiser packets.
     */
    uint8_t dest_endpoint;
    /** Pointer to data payload sent by advertiser */
    void * data;
    /** Amount of data */
    size_t num_bytes;
} ack_gen_input_t;

/**
 * \brief       Output structure for for callback function set by @ref
 *              app_lib_advertiser_ackdatacb_f
 *              "lib_advertiser->setRouterAckGenCb()"
 */
typedef struct
{
    /** Acknowledgement payload. Payload can be allocated by @ref
     * DIRADV_MAX_ACK_LEN definition. */
    void * data;
    /** length of the acknowledgement (maximum size is limited to @ref
     * ack_gen_input_t.ack_length "supported ack length" bytes) */
    uint8_t length;
} ack_gen_output_t;

/**
 * \brief       Callback function type used with @ref
 *              app_lib_advertiser_ackdatacb_f
 *              "lib_advertiser->setRouterAckGenCb()"
 *
 * \param[in]   in
 *              Information about received advertiser packet
 * \param[out]  out
 *              Generated acknowledgement
 * \return      true: output generated, false: use default acknowledgement
 */
typedef bool (*app_llhead_acklistener_f)(const ack_gen_input_t * in,
                                         ack_gen_output_t * out);

/**
 * \brief       Sets callback function to be called when ack is generated as a
 *              response for advertiser device transmission (in other devices in
 *              the network).
 * \param       callback
 *              Callback function to be used. NULL to disable
 * \note        Device must have @ref APP_LIB_SETTINGS_ROLE_FLAG_LL in device
 *              role (service @ref app_lib_settings_set_node_role_f
 *              "lib_settings->setNodeRole()" in order for callback to be
 *              active.
 */
typedef void (*app_lib_advertiser_ackdatacb_f)
    (app_llhead_acklistener_f callback);

/**
 * \brief   Set maximum queueing time for advertiser data packets
 * \param   time_ms
 *          Time in milliseconds how soon packet should be transmitted
 *          0 to disable the feature
 * \return  Result code, always @ref APP_RES_OK or @ref
 *          APP_RES_INVALID_CONFIGURATION if device is not configured as
 *          @ref APP_LIB_SETTINGS_ROLE_ADVERTISER
 *
 * Operation @ref app_lib_data_set_max_msg_queuing_time_f
 * "lib_data->setMaxMsgQueuingTime()" only allows setting the TTL value
 * in second precision. In CSMA-CA networks, the granularity of that service is
 * not enough. By using this service, advertiser can set the higher-precision
 * TTL and when advertiser sends data to CSMA-CA device, this TTL is also
 * checked (in addition to normal, second-granularity TTL set by @ref
 * app_lib_data_set_max_msg_queuing_time_f "lib_data->setMaxMsgQueuingTime()").
 * For time-slotted mode networks, this value has no impact due to nature of the
 * time-slotted mode transmission. Neither this service has impact if device is
 * not advertiser.
 *
 * Default value used is @ref DIRADV_DEFAULT_QUEUE_TIME
 *
 * Recommendation is to use values larger than 80 ms. Smaller values than that
 * will cause failed transmission even in normal operation conditions due to the
 * time periods when transmission is forbidden.
 *
 * @note Using @p time_ms with value 0 to disabling the feature may have quite
 * a large impact on power consumption. This means that device may make lot of
 * transmission attempts to target instead of giving up and trying another
 * , alternative, target and thus resulting high power consumption. This could
 * happen, for example, when target device is doing network scan.
 */
typedef app_res_e
    (*app_lib_advertiser_set_queuing_time_f)(uint16_t time_ms);

/**
 * @brief   Option flags to be used with advertiser
 */
typedef struct
{
    // If true, advertiser will follow the network in order to have valid target
    // to send data to. If false, advertiser does not follow the network and
    // should perform scan (service @ref app_lib_state_start_scan_nbors_f
    // "lib_state->startScanNbors()") before sending data packets.
    // Default value for this is false.
    bool follow_network;
} adv_option_t;

/**
 * @brief   Set options for advertiser
 * @param   option
 *          Options to set
 * @return  Result code, normally @ref APP_RES_OK. If @p option==NULL, @ref
 *          APP_RES_INVALID_NULL_POINTER. If role is not @ref
 *          APP_LIB_SETTINGS_ROLE_ADVERTISER , @ref
 *          APP_RES_INVALID_CONFIGURATION
 */
typedef app_res_e
    (*app_lib_advertiser_set_options_f)(adv_option_t * option);

/**
 * \brief       List of library functions
 */

typedef struct
{
    app_lib_advertiser_ackdatacb_f                    setRouterAckGenCb;
    app_lib_advertiser_set_queuing_time_f             setQueuingTimeHp;
    app_lib_advertiser_set_options_f                  setOptions;
} app_lib_advertiser_t;

#endif /* APP_LIB_ADVERTISER_H_ */
