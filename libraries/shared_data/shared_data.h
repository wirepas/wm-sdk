/* Copyright 2019 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/**
 * @file shared_data.h
 *
 * Helper library to share send and receive functions/callback between multiple
 * modules in the application.
 *
 * This library replaces the following functions and callbacks of the data
 * library:
 *  - @ref app_lib_data_set_data_received_cb_f "lib_data->setDataReceivedCb()",
 *  - @ref app_lib_data_set_data_sent_cb_f "lib_data->setDataSentCb()"
 *  - @ref app_lib_data_send_data_f "lib_data->sendData()"
 * They MUST NOT be used outside of this module.
 *
 * SHARED_DATA_MAX_TRACKED_PACKET defines the maximum number of sent packets
 * that can be tracked at the same time. It defaults to 16. It can be redefined
 * in the application makefile with the drawback of using more RAM.
 */

#ifndef _SHARED_DATA_H_
#define _SHARED_DATA_H_

#include <stdint.h>
#include "api.h"
#include "sl_list.h"

/** Value if Endpoint filtering is not used. */
#define SHARED_DATA_UNUSED_ENDPOINT -1

/** Value if multicast group filtering is not used. */
#define SHARED_DATA_UNUSED_MULTISCAST 0xFFFFFFFF

/** @brief Select what type of packet to receive. */
typedef enum
{
    /** Only receive Unicast packets. */
    SHARED_DATA_NET_MODE_UNICAST = 0,
    /** Only receive Broadcast packets. */
    SHARED_DATA_NET_MODE_BROADCAST = 1,
    /** Only receive Multicast packets. */
    SHARED_DATA_NET_MODE_MULTICAST = 2,
    /** Accept all type of packets (ignore filtering packet by type). */
    SHARED_DATA_NET_MODE_ALL = 3
} shared_data_net_mode_e;

/** @brief Structure holding all parameters for packet filtering. */
typedef struct
{
    /** Types of packet received (only for received packets). */
    shared_data_net_mode_e mode;
    /** Expected source endpoint. valid range [0;255], -1: No filtering by
     *  source endpoint.
     */
    int16_t src_endpoint;
    /** Expected destination endpoint. valid range [0;255], -1: No filtering
     *  by destination endpoint.
     */
    int16_t dest_endpoint;
    /** Will be called when multicast packet is received. Return true to accept
     *  packet. If NULL, all multicast packets are accepted. Only used with
     *  mode SHARED_DATA_NET_MODE_MULTICAST or SHARED_DATA_NET_MODE_ALL.
     *  Otherwise @ref Shared_Data_addDataReceivedCb return
     *  @ref APP_RES_INVALID_VALUE. This callback can be called two times for
     *  each received multicastpacket so its execution time must be kept short.
     */
    app_lib_settings_is_group_cb_f multicast_cb;
} shared_data_filter_t;

/**
 * @brief Forward declaration of shared_data_item_t
 */
typedef struct shared_data_item_s shared_data_item_t;

/**
 * @brief   The data reception callback.
 *
 * This is the callback called when a packet is received (and allowed).
 * The received packet is represented as a pointer to @ref
 * app_lib_data_received_t struct.
 *
 * @param   item
 *          Pointer to the filter item that initiated the callback.
 * @param   data
 *          Pointer to the received data.
 * @return  Result code, @ref app_lib_data_receive_res_e.
 * @note    If APP_LIB_DATA_RECEIVE_RES_NO_SPACE is returned, the whole data
 *          is blocked for all the app. App will start receiving data once
 *          this filter has called @ref Shared_Data_readyToReceive
 */
typedef app_lib_data_receive_res_e
    (*shared_data_received_cb_f)(const shared_data_item_t * item,
                                    const app_lib_data_received_t * data);

/**
 * @brief   Item containing filters and callback to call.
 *          This structure describe a callback to call when a packet is
 *          received if it is allowed by the associated filter.
 */
struct shared_data_item_s
{
    /** Reserved for sl_list use (DO NOT MODIFY). */
    void * reserved;
    /** Reserved for shared_data use (DO NOT MODIFY). */
    bool reserved2;
    /** Reserved for built-in pause mechanism (DO NOT MODIFY). */
    bool reserved3;
    /** Function to call if the received packet is allowed. */
    shared_data_received_cb_f cb;
    /** Packet filter parameters. */
    shared_data_filter_t filter;
};

/**
 * @brief   Initialize the shared data library.
 * @note    This function is automatically called if library is enabled.
 * @note    If Shared data module is used in application, the
 *          @ref app_lib_data_set_data_received_cb_f "lib_data->setDataReceivedCb()",
 *          @ref app_lib_data_set_data_sent_cb_f "lib_data->setDataSentCb()"
 *          and
 *          @ref app_lib_data_send_data_f "lib_data->sendData()"
 *          functions offered by data library MUST NOT be
 *           used outside of this module.
 * @return  @ref APP_RES_OK.
 */
app_res_e Shared_Data_init(void);

/**
 * @brief   Add a new packet received item to the list.
 *          If the item is already in the list it is only updated.
 * @param   item
 *          New item (callback + filter)
 * @return  APP_RES_OK if ok. See @ref app_res_e for
 *          other result codes.
 */
app_res_e Shared_Data_addDataReceivedCb(shared_data_item_t * item);

/**
 * @brief   Enable back the reception for an item.
 *          Reception is automatically paused when an item returns
 *          APP_LIB_DATA_RECEIVE_RES_NO_SPACE from its registered callback
 *          @ref shared_data_received_cb_f
 *          Only way to allow the reception is to call this function.
 * @note    Reception is paused for all items at the same time. All the items
 *          that pause the reception must explicitly reanable it. Otherwise
 *          reception will stay in pause mode. In fact, in pause mode messages
 *          are buffered on stack side and cannot be accepted individually based
 *          on filter set.
 * @param   item
 *          Item that is now ready
 * @return  APP_RES_OK if ok. See @ref app_res_e for
 *          other result codes.
 */
app_res_e Shared_Data_readyToReceive(shared_data_item_t * item);

/**
 * @brief   Remove a received packet item from the list.
 *          Removed item fields are all set to 0.
 * @param   item
 *          item to remove.
 */
void Shared_Data_removeDataReceivedCb(shared_data_item_t * item);

/**
 * @brief   Send data. The packet to send is represented as a
 *          @ref app_lib_data_to_send_t struct.
 * @param   data
 *          Data to send
 * @param   sent_cb
 *          Callback function to be called when a packet has gone through local
 *          processing and has finally been sent or discarded. If NULL is
 *          passed, the callback is disabled. This callback replaces
 *          APP_LIB_DATA_SEND_FLAG_TRACK flag of data structure.
 * @return  Result code, @ref APP_LIB_DATA_SEND_RES_SUCCESS means that data
 *          was accepted for sending. See @ref app_res_e for
 *          other result codes.
 */
app_lib_data_send_res_e Shared_Data_sendData(
                                        app_lib_data_to_send_t * data,
                                        app_lib_data_data_sent_cb_f sent_cb);

#endif //_SHARED_DATA_H_
