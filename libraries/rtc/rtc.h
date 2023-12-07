/* Copyright 2023 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

#ifndef _RTC_H_
#define _RTC_H_

#include <stdint.h>
#include <stdbool.h>


#define RTC_VERSION (uint16_t)(1)


/**
 * \brief   List of return code
 */
typedef enum
{
    /** Operation is successful */
    RTC_RES_OK = 0,
    /** RTC is not yet available from network */
    RTC_UNAVAILABLE_YET = 1,
    /** Cannot register new callback*/
    RTC_NO_MORE_CALLBACKS = 2,
    /** Callback is not known */
    RTC_UNKNOWN_CALLBACK = 3,
    /** One or more parameter value is invalid */
    RTC_INVALID_VALUE = 4,
    /** The library has not been initialized. */
    RTC_UNINITIALIZED = 5,
} rtc_res_e;

/** \brief List of Wirepas Ids for TLV encoded provisioning data. */
typedef enum
{
    RTC_ID_TIMESTAMP = 0,
    RTC_ID_TIMEZONE_OFFSET = 1
} provisioning_data_ids_e;

typedef uint64_t rtc_timestamp_t;

/**
 * \brief Callback called the first time RTC time is aquired from network
 * \note  It is up to registered module to call \ref RTC_getUTCTime() in the callback
 */
typedef void
    (*on_rtc_initialized)(void);

/**
 * @brief   Initialize the stack state library.
 * @return  Return code of operation
 */
rtc_res_e RTC_init(void);

/**
 * @brief   Get current expected RTC time from node.
 * @param   now
 *          Current RTC time if return code is RTC_RES_OK
 * @return  Return code of the operation @ref rtc_res_e
 * @note    RTC time must have been received from network in the last 50 days.
 */
rtc_res_e RTC_getUTCTime(rtc_timestamp_t * now);

/**
 * @brief   Get current expected RTC time with timezone from node.
 * @param   now
 *          Current local time if return code is RTC_RES_OK
 * @return  Return code of the operation @ref rtc_res_e
 */
rtc_res_e RTC_getLocalTime(rtc_timestamp_t * now);

/**
 * @brief   Get configured timezone offset of the node
 * @param   timezoneOffsetInSeconds
 *          timezone offset of the local time if return code is RTC_RES_OK
 * @return  Return code of the operation @ref rtc_res_e
 */
rtc_res_e RTC_getTimezoneOffsetInSeconds(long * timezoneOffsetInSeconds);

/**
 * @brief   Add a new callback to be informed when RTC time is available from network.
 * @param   callback
 *          New callback
 * @return  RTC_RES_OK if ok. See \ref rtc_res_e for
 *          other result codes.
 */
rtc_res_e RTC_addInitializeCb(on_rtc_initialized callback);

/**
 * @brief   Remove an event callback from the list.
 *          Removed item fields are all set to 0.
 * @param   callback
 *          callback to remove.
 * @return  RTC_RES_OK if ok. See \ref app_res_e for
 *          other result codes.
 */
rtc_res_e RTC_removeInitializedCb(on_rtc_initialized callback);

#endif //_RTC_H_
