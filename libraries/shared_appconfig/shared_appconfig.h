/* Copyright 2020 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */
#ifndef _SHARED_APPCONFIG_H_
#define _SHARED_APPCONFIG_H_

#include <stdint.h>
#include "api.h"

/**
 * \brief   List of return code
 */
typedef enum
{
    /** Operation is successful */
    SHARED_APP_CONFIG_RES_OK = 0,
    /** No more filter available */
    SHARED_APP_CONFIG_RES_NO_FILTER = 1,
    /** Invalid Param */
    SHARED_APP_CONFIG_RES_INVALID_PARAM = 2,
    /** Trying to remove a filter that doesn't exist */
    SHARED_APP_CONFIG_RES_INVALID_FILTER = 3,
    /** Using the library without previous initialization */
    SHARED_APP_CONFIG_RES_UNINITIALIZED = 4
} shared_app_config_res_e;

/**
 * \brief   Value to use to receive all types
 */
#define SHARED_APP_CONFIG_ALL_TYPE_FILTER   -1

/**
 * \brief   The app config type reception callback.
 *
 * This is the callback called when a new app config is received and
 * a matching type is present in the content.
 *
 * \param   type
 *          The type that match the filtering
 * \param   length
 *          The length of this TLV entry.
 * \param   value_p
 *          Pointer to the value of the TLV
 */
typedef void
    (*shared_app_config_received_cb_f)(uint16_t type,
                                       uint8_t length,
                                       uint8_t * value_p);

/** \brief Structure holding all parameters for app config type filtering. */
typedef struct
{
    /** Type for this filter. valid range [0;(2^16-1)], -1: to receive all types
     */
    int32_t type;
    /** Will be called when the the received app_config contains a matching type
     *  If the type is @ref SHARED_APP_CONFIG_ALL_TYPE_FILTER the callback will
     *  be called multiple times
     */
    shared_app_config_received_cb_f cb;
} shared_app_config_filter_t;

/**
 * \brief   Initialize the shared app config library library.
 * \note    If Shared app config module is used in application, the
 *          @ref app_lib_data_set_new_app_config_cb_f "lib_data->setNewAppConfigCb()"
 *          function offered by data library MUST NOT be
 *           used outside of this module.
 * \return  @ref SHARED_APP_CONFIG_RES_OK.
 */
shared_app_config_res_e Shared_Appconfig_init(void);

/**
 * \brief   Add a new app config type filter to the list.
 *          If the item is already in the list it is only updated.
 * \param   filter
 *          New filter (type + cb)
 * \param   filter_id
 *          Filter id to be used with @ref Shared_Appconfig_removeTypeReceivedCb.
 *          Set only if return code is SHARED_APP_CONFIG_RES_OK
 * \return  @ref SHARED_APP_CONFIG_RES_OK if ok. See @ref shared_app_config_res_e
 *          for other result codes.
 */
shared_app_config_res_e Shared_Appconfig_addFilter(shared_app_config_filter_t * filter,
                                                   uint16_t * filter_id);

/**
 * \brief   Remove a received packet item from the list.
 *          Removed item fields are all set to 0.
 * \param   filter_id
 *          filter to remove.
 * \return  @ref SHARED_APP_CONFIG_RES_OK if ok. See @ref shared_app_config_res_e
 *          for other result codes.
 */
shared_app_config_res_e Shared_Appconfig_removeFilter(uint16_t filter_id);

#endif //_SHARED_APPCONFIG_H_
