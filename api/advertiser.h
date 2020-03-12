/* Copyright 2018 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/**
 * \file advertiser.h
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

/** \brief Library symbolic name  */
#define APP_LIB_ADVERTISER_NAME 0x06cc5e24 //!< "ADVERT"

/** \brief Maximum supported library version */
#define APP_LIB_ADVERTISER_VERSION 0x201

/**
 * \brief  Advertiser sends data packet to headnode by using this src endpoint.
 *
 * Used with service @ref app_lib_data_send_data_f "lib_data->sendData()"
 */
#define DIRADV_EP_SRC_DATA 248
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
 * \brief  Destination endpoint, used always in advertiser
 *
 * Used when advertiser is sending data using service @ref
 * app_lib_data_send_data_f "lib_data->sendData()" as well as when
 * acknowledgement is handled, i.e. in callback function set with @ref
 * app_lib_advertiser_ackdatacb_f "lib_advertiser->setBeaconDataAckListenCb()"
 * and receiving packet in advertiser callback function set with @ref
 * app_lib_data_set_data_received_cb_f "lib_data->setDataReceivedCb".
 *
 * Summary of endpoints:
 *
 * <table>
 * <tr><th>Direction</th><th>Source endpoint</th><th>Destination endpoint</th>
 * </tr>
 * <tr><td>Advertiser --> headnode</td><td>@ref DIRADV_EP_SRC_DATA "248"</td>
 * <td>@ref DIRADV_EP_DEST "248"</td></tr>
 * <tr><td>Headnode --> advertiser</td><td>@ref DIRADV_EP_SRC_ACK "249"</td>
 * <td>@ref DIRADV_EP_DEST "248"</td></tr>
 * </table>
 */
#define DIRADV_EP_DEST     248

/**
 * \brief  Maximum length of the ACK message
 *
 * Used with callback function set with @ref
 * app_lib_advertiser_ackdatacb_f "lib_advertiser->setBeaconDataAckListenCb()".
 */
#define DIRADV_MAX_ACK_LEN  13

/**
 * \brief       Scan operation to find directed advertiser network head nodes
 *              network beacons. \param max_scan_time is used to end scan even
 *              if application has not aborted scan (by using @ref
 *              app_lib_advertiser_scan_stop_f
 *              "lib_advertiser->scanOffAdvertiserBeacons()"). Beacon
 *              information is acquired by using to @ref
 *              app_lib_state_set_on_beacon_cb_f "lib_state->setOnBeaconCb()"
 *              service.
 * \param       max_scan_time
 *              Maximum scan time in ms used for scanning.
 */
typedef void (*app_lib_advertiser_scan_f) (uint32_t max_scan_time);

/**
 * \brief       Stops active directed advertiser scan operation.
 *
 */
typedef void (*app_lib_advertiser_scan_stop_f)(void);

/**
 * \brief       Input data structure for callback function set by @ref
 *              app_lib_advertiser_ackdatacb_f
 *              "lib_advertiser->setRouterAckGenCb()"
 */
typedef struct
{
    /** Address of the sender (advertiser) */
    app_addr_t sender;
} ack_gen_input_t;

/**
 * \brief       Output structure for for callback function set by @ref
 *              app_lib_advertiser_ackdatacb_f
 *              "lib_advertiser->setRouterAckGenCb()"
 */
typedef struct
{
    /** Acknowledgement payload */
    void * data;
    /** length of the acknowledgement (maximum size is limited to @ref
     * DIRADV_MAX_ACK_LEN bytes) */
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
 *              Callback function to be used
 */
typedef void (*app_lib_advertiser_ackdatacb_f)
    (app_llhead_acklistener_f callback);

/**
 * \brief       Callback function type used with @ref
 *              app_lib_advertiser_scanendcb_f "lib_advertiser->setScanEndCb()".
 */
typedef void (*app_mac_scanendlistener_f)(void);

/**
 * \brief       This callback is used to set callback to be call when scan has
 *              ended. Scan can end either when application has called
 *              @ref app_lib_advertiser_scan_stop_f
 *              "lib_advertiser->scanOffAdvertiserBeacons()" or scan timeout
 *              set in @ref app_lib_advertiser_scan_f
 *              "lib_advertiser->scanOnAdvertiserBeacons()" has been reached.
 *
 * \param       Callback function type of @ref app_mac_scanendlistener_f.
 */
typedef void (*app_lib_advertiser_scanendcb_f)(app_mac_scanendlistener_f);

/**
 * \brief   Set maximum queueing time for advertiser data packets
 * \param   time_ms
 *          Time in milliseconds how soon packet should be transmitted
 *          0 to disable the feature (default value)
 * \return  Result code, always @ref APP_RES_OK
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
 */
typedef app_res_e
    (*app_lib_advertiser_set_queuing_time_f)(uint16_t time_ms);



/**
 * \brief       List of library functions
 */

typedef struct
{
    app_lib_advertiser_scan_f                         scanOnAdvertiserBeacons;
    app_lib_advertiser_scan_stop_f                    scanOffAdvertiserBeacons;
    app_lib_advertiser_ackdatacb_f                    setRouterAckGenCb;
    app_lib_advertiser_scanendcb_f                    setScanEndCb;
    app_lib_advertiser_set_queuing_time_f             setQueuingTimeHp;
} app_lib_advertiser_t;

#endif /* APP_LIB_ADVERTISER_H_ */
