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

/** Tag that must be present at the beginning of app config */
#define APP_CONFIG_V1_TLV   0x7EF6

/** Format of a app config following the TLV format */
typedef struct __attribute__((packed))
{
    uint16_t version;
    uint8_t entry_number;
} tlv_app_config_header_t;

typedef struct
{
    /** The filter set by app
     */
    shared_app_config_filter_t filter;
    /** Internal state to know if filter was called
     */
    bool called;
} shared_app_config_internal_filter_t;

/**
 * Is library initialized
 */
static bool m_initialized = false;

/**  List of filters */
static shared_app_config_filter_t m_filter[SHARED_APP_CONFIG_MAX_FILTER];

static void dispatch_to_modules(shared_app_config_internal_filter_t filters[],
                                uint16_t num_filters,
                                uint16_t type,
                                uint8_t len,
                                const uint8_t * val)
{
    for (uint8_t i = 0; i < num_filters; i++)
    {
        if ((filters[i].filter.type == type) ||
            (filters[i].filter.type == SHARED_APP_CONFIG_ALL_TYPE_FILTER))
        {
            filters[i].filter.cb(type, len, (uint8_t *) val);
            /* If filter is called, remember it by modyfing the working copy*/
            filters[i].called = true;
        }
    }
}

static void inform_other_modules(shared_app_config_internal_filter_t filters[],
                                 uint16_t num_filters)
{
    for (uint8_t i = 0; i < num_filters; i++)
    {
        // Check if filter is interested by information and not already called
        if (filters[i].called == false && filters[i].filter.call_cb_always)
        {
            filters[i].filter.cb(filters[i].filter.type, 0, NULL);
        }
    }
}

static void inform_filters(const uint8_t * bytes,
                           shared_app_config_internal_filter_t * filters,
                           uint16_t num_filters)
{
    tlv_record record;
    uint8_t entry_number;

    tlv_app_config_header_t * header = (tlv_app_config_header_t *) bytes;
    if (header->version != APP_CONFIG_V1_TLV)
    {
        LOG(LVL_WARNING, "App config with wrong header: 0x%x", header->version);
        // Not the right header/format
        // Dispatch it to the ones interested by incompatible app_config format
        // for backward compatibility reason
        dispatch_to_modules(filters,
                            num_filters,
                            SHARED_APP_CONFIG_INCOMPATIBLE_FILTER,
                            lib_data->getAppConfigNumBytes(),
                            bytes);

        inform_other_modules(filters, num_filters);
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

        dispatch_to_modules(filters, num_filters, item.type, item.length, item.value);
    }

    inform_other_modules(filters, num_filters);
}

/**
 * \brief Callback when a new app config is received form network
 */
static void new_app_config_cb(const uint8_t * bytes,
                              uint8_t seq,
                              uint16_t interval)
{
    uint16_t nb_filter = 0;
    // Allocate on stack the filter. A filter is only 8 bytes and
    // SHARED_APP_CONFIG_MAX_FILTER should remain relatively small.
    // Even if it is 10, it will be only 80 bytes allocated on stack
    shared_app_config_internal_filter_t current_filters[SHARED_APP_CONFIG_MAX_FILTER];

    // Before iterating filters, make a copy of them to avoid any
    // update of them. It also ease the way to know which filter were called
    // and which were not
    Sys_enterCriticalSection();
    for (uint8_t i = 0; i < SHARED_APP_CONFIG_MAX_FILTER; i++)
    {
        if (m_filter[i].cb != NULL)
        {
            memcpy(&(current_filters[i].filter), &(m_filter[i]), sizeof(shared_app_config_filter_t));
            current_filters[i].called = false;
            nb_filter++;
        }
    }
    Sys_exitCriticalSection();

    // Seq and interval are not in use in this module
    (void) seq;
    (void) interval;
    LOG(LVL_DEBUG, "Rx app_conf (s: %d, inter=%d)", seq, interval);

    inform_filters(bytes, current_filters, nb_filter);
}

shared_app_config_res_e Shared_Appconfig_init(void)
{
    if (m_initialized)
    {
        // Library is already initialized
        return SHARED_APP_CONFIG_RES_OK;
    }

    /* Set callback for received unicast and broadcast messages. */
    lib_data->setNewAppConfigCb(new_app_config_cb);

    for (uint8_t i = 0; i < SHARED_APP_CONFIG_MAX_FILTER; i++)
    {
        m_filter[i].cb = NULL;
    }

    m_initialized = true;

    return SHARED_APP_CONFIG_RES_OK;
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
        if (m_filter[i].cb == NULL)
        {
            // One filter found
            memcpy(&m_filter[i],
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

        // Filter added, let's update it with current app_config if set
        uint8_t appconfig[APP_LIB_DATA_MAX_APP_CONFIG_NUM_BYTES];
        uint8_t seq;
        uint16_t interval;
        if (lib_data->readAppConfig(appconfig, &seq, &interval)
             == APP_LIB_DATA_APP_CONFIG_RES_SUCCESS)
        {
            // Convert it to an internal filter
            shared_app_config_internal_filter_t internal_filter;
            memcpy(&internal_filter.filter, filter, sizeof(shared_app_config_filter_t));
            internal_filter.called = false;
            inform_filters(appconfig, &internal_filter, 1);
        }
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
        m_filter[filter_id].cb != NULL)
    {
        m_filter[filter_id].cb = NULL;
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

shared_app_config_res_e Shared_Appconfig_setAppConfig(const uint8_t * bytes)
{
    app_lib_data_app_config_res_e res;
    if (!m_initialized)
    {
        return SHARED_APP_CONFIG_RES_UNINITIALIZED;
    }

    LOG(LVL_DEBUG, "Set new app config");
    res = lib_data->writeAppConfigData(bytes);

    if (res == APP_LIB_DATA_APP_CONFIG_RES_INVALID_NULL_POINTER)
    {
        return SHARED_APP_CONFIG_RES_INVALID_PARAM;
    }
    else if (res == APP_LIB_DATA_APP_CONFIG_RES_INVALID_ROLE)
    {
        return SHARED_APP_CONFIG_RES_INVALID_ROLE;
    }
    else if (res != APP_LIB_DATA_APP_CONFIG_RES_SUCCESS)
    {
        // It should never happen, but if another error code is returned later
        // Just return an invalid param error code instead of OK...
        return SHARED_APP_CONFIG_RES_INVALID_PARAM;
    }

    // Notify ourself of the change
    // Seq and interval are not used in this module so can be set to max value
    // Could be called on a dedicated task but fine for now
    new_app_config_cb(bytes, (uint8_t)(-1), (uint16_t)(-1));

    return SHARED_APP_CONFIG_RES_OK;
}

shared_app_config_res_e Shared_Appconfig_notifyAppConfig(const uint8_t * bytes)
{
    if (!m_initialized)
    {
        return SHARED_APP_CONFIG_RES_UNINITIALIZED;
    }

    // Notify ourself of the change
    // Seq and interval are not used in this module so can be set to max value
    // Could be called on a dedicated task but fine for now
    new_app_config_cb(bytes, (uint8_t)(-1), (uint16_t)(-1));

    return SHARED_APP_CONFIG_RES_OK;
}
