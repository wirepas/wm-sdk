/* Copyright 2020 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */
#include "shared_appconfig.h"
#include <string.h>
#define DEBUG_LOG_MODULE_NAME "SH_ACONF"
#define DEBUG_LOG_MAX_LEVEL LVL_NOLOG
#include "debug_log.h"
#include "tlv.h"

/**
 * Maximum filter that can be registered at the same time
 * It is application specific
 */
#ifndef SHARED_APP_CONFIG_MAX_FILTER
// Must be defined from application
#error "Please define SHARED_APP_CONFIG_MAX_FILTER from your application makefile"
#endif

/** Tag that must be present at the beginning of app config */
#define APP_CONFIG_V1_TLV   0x7EF6

/** Internal structure of a filter */
typedef struct
{
    shared_app_config_filter_t  filter; /* Filter set by app */
} filter_t;

/** Format of a app config following the TLV format */
typedef struct __attribute__((packed))
{
    uint16_t version;
    uint8_t entry_number;
} tlv_app_config_header_t;

/**
 * Is library initialized
 */
static bool m_initialized = false;

/**  List of filters */
static filter_t m_filter[SHARED_APP_CONFIG_MAX_FILTER];

static void dispatch_to_modules(filter_t * filters,
                                uint16_t type,
                                uint8_t len,
                                const uint8_t * val)
{
    for (uint8_t i = 0; i < SHARED_APP_CONFIG_MAX_FILTER; i++)
    {
        if (filters[i].filter.cb == NULL)
        {
            continue;
        }

        if ((filters[i].filter.type == type) ||
            (filters[i].filter.type == SHARED_APP_CONFIG_ALL_TYPE_FILTER))
        {
            filters[i].filter.cb(type, len, (uint8_t *) val);
            /* If filter is called, remember it by modyfing the working copy*/
            filters[i].filter.cb = NULL;
        }
    }
}

static void inform_other_modules(filter_t * filters)
{
    for (uint8_t i = 0; i < SHARED_APP_CONFIG_MAX_FILTER; i++)
    {
        if (filters[i].filter.cb == NULL)
        {
            continue;
        }

        // Cb is still in the list so was not called yet
        // Check if filter is interested by information
        if (filters[i].filter.call_cb_always)
        {
            filters[i].filter.cb(filters[i].filter.type, 0, NULL);
        }
    }
}

/**
 * \brief Callback when a new app config is received form network
 */
static void new_app_config_cb(const uint8_t * bytes,
                              uint8_t seq,
                              uint16_t interval)
{
    tlv_record record;
    uint8_t entry_number;
    tlv_app_config_header_t * header = (tlv_app_config_header_t *) bytes;
    // Allocate on stack the filter. A filter is only 8 bytes and
    // SHARED_APP_CONFIG_MAX_FILTER should remain relatively small.
    // Even if it is 10, it will be only 80 bytes allocated on stack
    filter_t current_filter[SHARED_APP_CONFIG_MAX_FILTER];

    // Before iterating filters, make a copy of them to avoid any
    // update of them. It also ease the way to know which filter were called
    // and which were not
    Sys_enterCriticalSection();
    for (uint8_t i = 0; i < SHARED_APP_CONFIG_MAX_FILTER; i++)
    {
        memcpy(&(current_filter[i].filter), &(m_filter[i].filter), sizeof(shared_app_config_filter_t));
    }
    Sys_exitCriticalSection();

    LOG(LVL_DEBUG, "Rx app_conf (s: %d, inter=%d)", seq, interval);

    if (header->version != APP_CONFIG_V1_TLV)
    {
        LOG(LVL_WARNING, "App config with wrong header: 0x%x", header->version);
        // Not the right header/format
        // Dispatch it to the ones interested by incompatible app_config format
        // for backward compatibility reason
        dispatch_to_modules(current_filter,
                            SHARED_APP_CONFIG_INCOMPATIBLE_FILTER,
                            lib_data->getAppConfigNumBytes(),
                            bytes);
        return;
    }

    LOG(LVL_DEBUG,"entry=%d", header->entry_number);

    entry_number = header->entry_number;

    // Check TLV entries one by one up to number of TLV set in
    Tlv_init(&record,
             (uint8_t *)(bytes + sizeof(tlv_app_config_header_t)),
             lib_data->getAppConfigNumBytes()-sizeof(tlv_app_config_header_t));

    while (entry_number--)
    {
        tlv_item_t item;
        tlv_res_e tlv_res;

        tlv_res = Tlv_Decode_getNextItem(&record, &item);
        if (tlv_res == TLV_RES_ERROR)
        {
            LOG(LVL_ERROR,
                "App config wrong format");
            break;
        }
        else if (tlv_res == TLV_RES_END)
        {
            LOG(LVL_ERROR,
                "Not enough TLV entries");
            break;
        }

        dispatch_to_modules(current_filter, item.type, item.length, item.value);
    }

    inform_other_modules(current_filter);
}

shared_app_config_res_e Shared_Appconfig_init(void)
{
    /* Set callback for received unicast and broadcast messages. */
    lib_data->setNewAppConfigCb(new_app_config_cb);

    for (uint8_t i = 0; i < SHARED_APP_CONFIG_MAX_FILTER; i++)
    {
        m_filter[i].filter.cb = NULL;
    }

    m_initialized = true;

    return APP_RES_OK;
}

shared_app_config_res_e Shared_Appconfig_addFilter(
                            shared_app_config_filter_t * filter,
                            uint16_t * filter_id)
{
    shared_app_config_res_e res = SHARED_APP_CONFIG_RES_OK;

    if (!m_initialized)
    {
        return SHARED_APP_CONFIG_RES_UNINITIALIZED;
    }

    if (filter->cb == NULL)
    {
        LOG(LVL_ERROR, "Invalid Cb");
        return SHARED_APP_CONFIG_RES_INVALID_PARAM;
    }

    res = SHARED_APP_CONFIG_RES_NO_FILTER;

    Sys_enterCriticalSection();
    for (uint8_t i = 0; i < SHARED_APP_CONFIG_MAX_FILTER; i++)
    {
        if (m_filter[i].filter.cb == NULL)
        {
            // One filter found
            memcpy(&m_filter[i].filter,
                   filter,
                   sizeof(shared_app_config_filter_t));

            // Set the id
            *filter_id = i;

            res = SHARED_APP_CONFIG_RES_OK;
            break;
        }
    }
    Sys_exitCriticalSection();

    if (res == SHARED_APP_CONFIG_RES_OK)
    {
        LOG(LVL_DEBUG,
            "App conf filter added (type: %d id: %d)",
            filter->type,
            *filter_id);
    }
    else
    {
        LOG(LVL_ERROR,
            "Cannot add app conf filter (type: %d, res: %d)",
            filter->type,
            res);
    }
    return res;
}

shared_app_config_res_e Shared_Appconfig_removeFilter(uint16_t filter_id)
{
    if (!m_initialized)
    {
        return SHARED_APP_CONFIG_RES_UNINITIALIZED;
    }

    LOG(LVL_DEBUG, "Remove app conf filter (id: %d)", filter_id);

    shared_app_config_res_e res = SHARED_APP_CONFIG_RES_OK;
    Sys_enterCriticalSection();
    if (filter_id < SHARED_APP_CONFIG_MAX_FILTER &&
        m_filter[filter_id].filter.cb != NULL)
    {
        m_filter[filter_id].filter.cb = NULL;
    }
    else
    {
        res = SHARED_APP_CONFIG_RES_INVALID_FILTER;
    }
    Sys_exitCriticalSection();

    if (res != SHARED_APP_CONFIG_RES_OK)
    {
        LOG(LVL_ERROR, "Cannot remove filter %d", filter_id);
    }
    return res;
}
