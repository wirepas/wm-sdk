/**
 * @file    posapp_settings.h
 * @brief   Header for the application settings source.
 * @copyright Wirepas Ltd 2020
 */
#ifndef POSAPP_SETTINGS_H
#define POSAPP_SETTINGS_H

#include <stdint.h>
#include <stdbool.h>

#include "api.h"
#include "node_configuration.h"
#if defined (CONF_USE_PERSISTENT_MEMORY)
#include "persistent.h"
#endif

typedef struct
{
    uint32_t node_address;
    uint32_t network_address;
    uint8_t network_channel;
    uint8_t node_role;
    poslib_settings_t poslib_settings;
} persistent_settings_t;

#define MAX_READ_SIZE  sizeof(persistent_settings_t)

/**
 * @brief   Configure the device.
 */
void PosApp_Settings_configureNode(void);

/**
 * @brief   Read configure values from the persistent memory and compares
 *          PosLib existing settings. If Poslib values not equal then
 *          PosLib configuration is wrote to persistent memory.
 *          There is needed to have persistent memory set with correct
 *          PERSISTENT_MAGIC_KEY e.g. with fucntion App_Settings_setfactory
 *          in product production with correct setup.
 * @return  True if read and configured
 */
#if defined(CONF_USE_PERSISTENT_MEMORY)
void PosApp_Settings_persistent_datacheck();
#else
__STATIC_INLINE void PosApp_Settings_persistent_datacheck()
{

}
#endif

/**
 * @brief   Configure persistent memory in factory etc. Persistent memory
 *          needs to be set once before other posapp_settings functions
 *          can be used. Settings can be done by writing default
 *          configuration using this function.
 *
 * @param   persistent_settings_t with default data to store to memory
 * @return  persistent_res_e
 */
#if defined (CONF_USE_PERSISTENT_FACTORY_SET)
persistent_res_e PosApp_Settings_setfactory
    (persistent_settings_t * persistent_data);
#endif

#endif /*POSAPP_SETTINGS*/
