/* Copyright 2023 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

#define DEBUG_LOG_MODULE_NAME "RTC"
#ifdef DEBUG_RTC_LOG_MAX_LEVEL
#define DEBUG_LOG_MAX_LEVEL DEBUG_RTC_LOG_MAX_LEVEL
#else
#define DEBUG_LOG_MAX_LEVEL LVL_NOLOG
#endif

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#include "api.h"
#include "app_scheduler.h"
#include "tlv.h"
#include "debug_log.h"
#include "shared_data.h"
#include "rtc.h"

/** Boolean asserting whether the library has been initialized (RTC_init). */
static bool m_initialized = false;

/** Endpoint to send and receive rtc data. */
#define DATA_RTC_SRC_EP (78u)  // TODO
#define DATA_RTC_DEST_EP (79u)  // TODO

/** Period in milliseconds of update_time_references function. */
#define UPDATE_TIME_REFERENCES_PERIOD_MS (1200000ul)  // 20 minutes

/** Offset between utc time to local timezone in seconds. */
static long m_timezone_offset_s;
/** Time of the last rtc update in high precision unit. */
static uint32_t m_local_reference_time_hp;
/** Time of the last received RTC in milliseconds. */
static rtc_timestamp_t m_rtc_reference_time_ms;

/** Boolean asserting whether the rtc has been received at least once. */
static bool m_is_rtc_set;

typedef struct
{
    on_rtc_initialized cb;
} rtc_cb_t;

/** List of callbacks to be called when first rtc is received. */
static rtc_cb_t m_on_rtc_initialized_cbs[RTC_MAX_CB];


/* RTC Received message type */
typedef struct
{
    /** Timestamp of the rtc in utc time format in milliseconds. */
    rtc_timestamp_t timestamp_ms;
    /** Number of seconds to be added to the utc time to know local time. */
    long timezone_offset;
} rtc_recv_msg_t;


static void handle_tlv_item(tlv_item_t * item, rtc_recv_msg_t * rtc_recv_msg)
{
    switch(item->type)
    {
        case RTC_ID_TIMESTAMP:
        {
            if(item->length == sizeof(rtc_timestamp_t))
            {
                memcpy(&rtc_recv_msg->timestamp_ms,
                   (rtc_timestamp_t *) item->value,
                   sizeof(rtc_recv_msg->timestamp_ms));
            }
            break;
        }
        case RTC_ID_TIMEZONE_OFFSET:
        {
            if(item->length == sizeof(int32_t))
            {
                memcpy(&rtc_recv_msg->timezone_offset,
                   (int32_t *) item->value,
                   sizeof(rtc_recv_msg->timezone_offset));
            }
            break;
        }
        default:
        {
            /* Let other ids not to be checked to be backward compatible. */
            break;
        }
    }
}

/**
 * @brief   Initialize tlv decoder and buffer.
 */
static rtc_res_e parse_rtc_message(uint8_t * rtc_recv_buffer_p, int num_bytes, rtc_recv_msg_t * rtc_recv_msg)
{
    memset(rtc_recv_msg, 0, sizeof(*rtc_recv_msg));

    tlv_record record;
    Tlv_init(&record, rtc_recv_buffer_p, num_bytes);

    tlv_item_t item;
    tlv_res_e res = TLV_RES_OK;  // result of TLV parsing
    while(res == TLV_RES_OK)
    {
        res = Tlv_Decode_getNextItem(&record, &item);
        handle_tlv_item(&item, rtc_recv_msg);
    }
    if(res != TLV_RES_END)
    {
        return RTC_INVALID_VALUE;  // TLV Parsing returned an error
    }
    return RTC_RES_OK;
}

/**
 * \brief   Update local reference time
 * \note    As high precision timestamps are given with a 36 minutes loop
 *          for arithmetical uses, it is necessary to update the time references
 *          before it loops otherwise rtc time will be incorrect.
*/
static uint32_t update_time_references(void)
{
    LOG(LVL_DEBUG, "update_time_references - update time references");
    uint32_t new_local_reference_time_hp = lib_time->getTimestampHp();
    uint32_t time_spent_since_ref_us = lib_time->getTimeDiffUs(
        new_local_reference_time_hp, m_local_reference_time_hp);

    /* Updating time references. */
    Sys_enterCriticalSection();
    m_local_reference_time_hp = new_local_reference_time_hp;
    // 1 ms precision is lost, but it can be neglected
    // due to other time precision loses that are greater as,
    // this function is called once every 20 minutes.
    m_rtc_reference_time_ms += time_spent_since_ref_us/1000;
    Sys_exitCriticalSection();

    return UPDATE_TIME_REFERENCES_PERIOD_MS;
}

/**
 * \brief   Return the local time elapsed since the last rtc update.
 */
static uint32_t time_since_last_update_ms(void)
{
    return lib_time->getTimeDiffUs(
            lib_time->getTimestampHp(), m_local_reference_time_hp)/1000;
}

/**
 * \brief   Reference locally the rtc time sent by the sink.
 * \return  Return code of the operation \ref rtc_res_e
 */
static rtc_res_e set_rtc(rtc_recv_msg_t rtc_recv_msg)
{
    Sys_enterCriticalSection();
    /* Change time references for new ones */
    m_timezone_offset_s = rtc_recv_msg.timezone_offset;
    // End-to-end transmission delay has to be added to the time
    // sent in the network for the time to be relevant
    m_rtc_reference_time_ms = rtc_recv_msg.timestamp_ms;

    /* Variables to calculate rtc at any time */
    m_local_reference_time_hp = lib_time->getTimestampHp();
    m_is_rtc_set = true;
    Sys_exitCriticalSection();

    LOG(LVL_INFO,
        "received time is 0x%x%08x and time zone has a shift of %lis on utc",
        (uint32_t) (m_rtc_reference_time_ms>>32),
        (uint32_t) m_rtc_reference_time_ms,
        m_timezone_offset_s);
    return RTC_RES_OK;
}

rtc_res_e RTC_getUTCTime(rtc_timestamp_t * now)
{
    if (!m_is_rtc_set)
    {
        LOG(LVL_WARNING, "RTC_getUTCTime - Time has not been received yet");
        return RTC_UNINITIALIZED;
    }
    /* The RTC time in UTC Timezone is basically the sum of rtc reference time
     * and the time spent since the last rtc reference update. */
    *now = m_rtc_reference_time_ms + time_since_last_update_ms();
    return RTC_RES_OK;
}

rtc_res_e RTC_getLocalTime(rtc_timestamp_t * now)
{
    if (!m_is_rtc_set)
    {
        LOG(LVL_WARNING, "RTC_getLocalTime - Time has not been received yet");
        return RTC_UNINITIALIZED;
    }
    long timezone_offset;
    rtc_res_e res = RTC_getTimezoneOffsetInSeconds(&timezone_offset);
    if (res != RTC_RES_OK)
    {
        LOG(LVL_WARNING, "RTC_getLocalTime - Timezone has not been set yet");
        return res;
    }
    /* Local time is basically the sum of rtc reference time,
     * the time spent since the last rtc reference update and
     * the timezone offset. */
    *now = m_rtc_reference_time_ms + time_since_last_update_ms() + timezone_offset*1000;
    return res;
}

rtc_res_e RTC_getTimezoneOffsetInSeconds(long * timezoneOffsetInSeconds)
{
    if (!m_is_rtc_set)
    {
        LOG(LVL_WARNING, "RTC_getTimezoneOffsetInSeconds - Time has not been received yet");
        return RTC_UNINITIALIZED;
    }
    *timezoneOffsetInSeconds = m_timezone_offset_s;
    return RTC_RES_OK;
}

/**
 * \brief   Launch all callbacks present in the list m_on_rtc_initialized_cbs.
 * \return  Return code of the operation \ref rtc_res_e
 */
static rtc_res_e lauch_rtc_cbs(void)
{
    LOG(LVL_DEBUG, "Callbacks which need RTC time to be set are being called.");
    for (uint8_t i = 0; i < RTC_MAX_CB; i++)
    {
        rtc_cb_t on_rtc_initialized_cb = m_on_rtc_initialized_cbs[i];
        if(on_rtc_initialized_cb.cb != NULL)
        {
            on_rtc_initialized_cb.cb();
        }
    }
    return RTC_RES_OK;
}

/**
 * \brief   Data reception callback
 * \param   data
 *          Received data, \ref app_lib_data_received_t
 * \return  Result code, \ref app_lib_data_receive_res_e
 */
static app_lib_data_receive_res_e rtc_received_cb(
    const shared_data_item_t * item,
    const app_lib_data_received_t * data)
{
    LOG(LVL_INFO, "rtc_received_cb - A RTC message has been received");
    rtc_recv_msg_t rtc_recv_msg;
    uint8_t * rtc_recv_buffer_p = (uint8_t *) data->bytes;
    uint16_t * version = (uint16_t *) data->bytes;
    uint64_t travel_time_hp = ((uint64_t) data->delay_hp);
    bool rtc_already_set = m_is_rtc_set;

    // Verify that the message contains more than just a version
    if(data->num_bytes <= sizeof(uint16_t))
    {
        LOG(LVL_WARNING, "rtc_received_cb - RTC message received is too small");
        return APP_LIB_DATA_RECEIVE_RES_HANDLED;  // discard the message
    }

    // Verify the message version
    if (*version != RTC_VERSION)
    {
        LOG(LVL_WARNING, "rtc_received_cb - RTC message has not the good version");
        return APP_LIB_DATA_RECEIVE_RES_HANDLED;  // discard the message
    }

    if(parse_rtc_message(rtc_recv_buffer_p+2, data->num_bytes-2, &rtc_recv_msg) != RTC_RES_OK)
    {
        LOG(LVL_WARNING, "rtc_received_cb - RTC message could not be parsed");
        return APP_LIB_DATA_RECEIVE_RES_HANDLED;  // discard the message
    }

    // Sets the new time reference of the rtc.
    rtc_recv_msg.timestamp_ms += ((travel_time_hp * 1000) >> 10);
    set_rtc(rtc_recv_msg);

    // Updates the time references in 20 minutes.
    App_Scheduler_addTask_execTime(&update_time_references,
                        UPDATE_TIME_REFERENCES_PERIOD_MS,
                        50);

    if(!rtc_already_set)
    {
        // Some callbacks must be launched the first time RTC time is set.
        lauch_rtc_cbs();
    }

    // Data handled successfully
    return APP_LIB_DATA_RECEIVE_RES_HANDLED;
}

/* Unicast messages filter */
static shared_data_item_t alltype_packets_filter =
{
    .cb = rtc_received_cb,
    .filter = {
        .mode = SHARED_DATA_NET_MODE_ALL,
        /* Filtering by source endpoint. */
        .src_endpoint = DATA_RTC_SRC_EP,
        /* Filtering by destination endpoint. */
        .dest_endpoint = DATA_RTC_DEST_EP
    }
};

rtc_res_e RTC_init(void)
{
    if (m_initialized)
    {
        LOG(LVL_DEBUG, "RTC_init: already initialized)");
        return RTC_RES_OK;
    }
    m_is_rtc_set = false;

    for (uint8_t i = 0; i < RTC_MAX_CB; i++)
    {
        m_on_rtc_initialized_cbs[i].cb = NULL;
    }
    m_initialized = true;
    LOG(LVL_DEBUG, "RTC_init (%d)", RTC_MAX_CB);

    /* Set unicast & broadcast received messages callback. */
    Shared_Data_addDataReceivedCb(&alltype_packets_filter);

    return RTC_RES_OK;
}

rtc_res_e RTC_addInitializeCb(on_rtc_initialized callback)
{
    if (!m_initialized)
    {
        return RTC_INVALID_VALUE;
    }
    rtc_res_e res = RTC_NO_MORE_CALLBACKS;
    int free_slot = -1;

    if (callback == NULL)
    {
        return RTC_INVALID_VALUE;
    }

    Sys_enterCriticalSection();
    for (uint8_t i = 0; i < RTC_MAX_CB; i++)
    {
        if (m_on_rtc_initialized_cbs[i].cb == NULL)
        {
            /* One free room found */
            free_slot = i;
            continue;
        }
        else if (m_on_rtc_initialized_cbs[i].cb == callback)
        {
            /* Callback already present */
            res = RTC_RES_OK;
            break;
        }
    }

    if (res != RTC_RES_OK && free_slot >= 0)
    {
        /* Callback was not already present and a free room was found */
        m_on_rtc_initialized_cbs[free_slot].cb = callback;
        res = RTC_RES_OK;
    }
    Sys_exitCriticalSection();

    if (res == RTC_RES_OK)
    {
        LOG(LVL_DEBUG, "Add rtc cb (0x%x)", callback);
    }
    else
    {
        LOG(LVL_ERROR, "Cannot add rtc cb (0x%x)", callback);
    }
    return res;
}

rtc_res_e RTC_removeInitializedCb(on_rtc_initialized callback)
{
    if (!m_initialized)
    {
        return RTC_INVALID_VALUE;
    }
    rtc_res_e res = RTC_UNKNOWN_CALLBACK;

    LOG(LVL_DEBUG, "Removing event cb (0x%x)", callback);

    Sys_enterCriticalSection();
    for (uint8_t i = 0; i < RTC_MAX_CB; i++)
    {
        if (m_on_rtc_initialized_cbs[i].cb == callback)
        {
            m_on_rtc_initialized_cbs[i].cb= NULL;
            res = RTC_RES_OK;
        }
    }
    Sys_exitCriticalSection();

    if (res != RTC_RES_OK)
    {
        LOG(LVL_ERROR, "Cannot remove event cb (0x%x)", callback);
    }

    return res;
}
