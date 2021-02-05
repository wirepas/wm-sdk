/**
 * @file    posapp_settings.c
 * @brief   Application settings module.
 *
 * @copyright  Wirepas Ltd 2020
 */


#define DEBUG_LOG_MODULE_NAME "POSAPP SETTINGS"
#ifdef DEBUG_POSLIB_LOG_MAX_LEVEL
#define DEBUG_LOG_MAX_LEVEL DEBUG_POSLIB_LOG_MAX_LEVEL
#else
#define DEBUG_LOG_MAX_LEVEL LVL_NOLOG
#endif
#include "debug_log.h"
#include <string.h>
#include "api.h"
#include "poslib.h"
#include "posapp_settings.h"

#if defined(CONF_USE_PERSISTENT_MEMORY)
static persistent_settings_t m_app_settings;
#endif

/*
 *  Network keys define in mcu/common/start.c and
 *  used only if default_network_cipher_key and default_network_authen_key
 *  are defined in one of the config.mk (set to NULL otherwise)
 */
extern const uint8_t * authen_key_p;
extern const uint8_t * cipher_key_p;

/**
 * @brief   Read configure values from the persistent memory and configure
 *          the device. Node role, address and network address is wrote
 *          to persistent memory only if not set in Wirepas Stack.
 *          PosLib settings are wrote from persistent memory always when
 *          PosLib not role is set in persistent memory or Poslib ble type
 *          is set in persistent memory.
 *          There is needed to have persistent memory set with correct
 *          PERSISTENT_MAGIC_KEY e.g. with function App_Settings_setfactory
 *          in product production with correct setup.
 * @return  True if read and configured for node role, address and network
 *          address wrote. No return value change if PosLib settings are wrote.
 */
#if defined(CONF_USE_PERSISTENT_MEMORY)
static bool read_persistent_memory()
{
    bool return_status = false;
    app_addr_t node_addr;
    app_lib_settings_net_addr_t network_addr;
    app_lib_settings_net_channel_t network_ch;
    app_lib_settings_role_t role;

    LOG(LVL_INFO, "");

    if (Mcu_Persistent_isValid(PERSISTENT_MAGIC_KEY) == PERSISTENT_RES_OK)
    {
        persistent_settings_t data_per_read;
        uint8_t buffer[MAX_READ_SIZE+1] = {0};

        Mcu_Persistent_read(buffer, MAX_READ_SIZE);
        memcpy(&data_per_read, buffer, sizeof(persistent_settings_t));

        // If one of the parameters haven't been set then this is the first boot.
        if ((lib_settings->getNodeRole(&role) != APP_RES_OK) ||
            (lib_settings->getNodeAddress(&node_addr) != APP_RES_OK) ||
            (lib_settings->getNetworkAddress(&network_addr) != APP_RES_OK) ||
            (lib_settings->getNetworkChannel(&network_ch) != APP_RES_OK))
        {
            // Node Id
            if (data_per_read.node_address != 0xFFFFFF)
            {
                m_app_settings.node_address = data_per_read.node_address;
                LOG(LVL_INFO, "\"From persist memory node_addr\":%u",
                  m_app_settings.node_address);
            }
            // Network address
            if (data_per_read.network_address != 0xFFFFFF)
            {
                m_app_settings.network_address = data_per_read.network_address;
                LOG(LVL_INFO, "\"From persist memory network_address\":%u",
                  m_app_settings.network_address);
            }
            // Network channel Id
            if (data_per_read.network_channel != 0xFF)
            {
                m_app_settings.network_channel = data_per_read.network_channel;
                LOG(LVL_INFO, "\"From persist memory network_channel\":%u",
                  m_app_settings.network_channel);
            }
            // Node role
            if (data_per_read.node_role != 0xFF)
            {
                m_app_settings.node_role = data_per_read.node_role;
                LOG(LVL_INFO, "\"From persist memory node_role\":%u",
                  m_app_settings.node_role);
            }

            // If the reading configure values from Persistent memory are
            // unformatted, then will be use default values from config.mk
            if (lib_settings->setNodeRole(m_app_settings.node_role) == APP_RES_OK)
            {
                if (configureNode(m_app_settings.node_address,
                                  m_app_settings.network_address,
                                  m_app_settings.network_channel,
                                  authen_key_p,
                                  cipher_key_p) == APP_RES_OK)
                {
                    return_status = true;
                }
            }

            if (data_per_read.poslib_settings.node_mode == POSLIB_MODE_NRLS_TAG &&
                m_app_settings.node_role != APP_LIB_SETTINGS_ROLE_SUBNODE)
            {
                return false;
            }

            if (data_per_read.poslib_settings.node_mode == POSLIB_MODE_DIRECTED_ADV_TAG &&
                m_app_settings.node_role != APP_LIB_SETTINGS_ROLE_ADVERTISER)
             {
                  return false;
             }

             // Set PosLib settings
             PosLib_setConfig(&data_per_read.poslib_settings);
        }
    }

    return return_status;
}
#endif

#if defined(CONF_USE_PERSISTENT_MEMORY)
void PosApp_Settings_persistent_datacheck()
{
    poslib_settings_t poslib_read_settings;
    persistent_settings_t data_per_read;
    uint8_t buffer[MAX_READ_SIZE+1] = {0};

    memset(&poslib_read_settings, 0, sizeof(poslib_settings_t));
    memset(&data_per_read, 0, sizeof(persistent_settings_t));
    PosLib_getConfig(&poslib_read_settings);

    if (Mcu_Persistent_isValid(PERSISTENT_MAGIC_KEY) == PERSISTENT_RES_OK)
    {
        Mcu_Persistent_read(buffer, MAX_READ_SIZE);
        memcpy(&data_per_read, buffer, sizeof(persistent_settings_t));

        if (memcmp(&data_per_read.poslib_settings, &poslib_read_settings,
                   sizeof(poslib_settings_t)))
        {
            memcpy(&data_per_read.poslib_settings, &poslib_read_settings,
                   sizeof(poslib_settings_t));
            memcpy(buffer, &data_per_read, sizeof(persistent_settings_t));
            Mcu_Persistent_write(&buffer[0], 0, MAX_READ_SIZE);
        }
    }
}
#endif

void PosApp_Settings_configureNode(void)
{
#if defined(CONF_USE_PERSISTENT_MEMORY)
    read_persistent_memory();
#else
    app_lib_settings_role_t new_role = 0;
    app_addr_t node_address;

    if (lib_settings->getNodeAddress(&node_address) ==
       APP_RES_INVALID_CONFIGURATION)
    {
        new_role = app_lib_settings_create_role(CONF_ROLE, CONF_ROLE_FLAG);
        lib_settings->setNodeRole(new_role);
    }

    configureNode(getUniqueAddress(),
                  CONF_NETWORK_ADDRESS,
                  CONF_NETWORK_CHANNEL,
                  authen_key_p,
                  cipher_key_p);
#endif
    node_address = 0;
    app_lib_settings_net_addr_t nw_address = 0;
    app_lib_settings_net_channel_t nw_channel = 0;
    app_lib_settings_role_t node_role = 0;

    lib_settings->getNodeAddress(&node_address);
    lib_settings->getNetworkAddress(&nw_address);
    lib_settings->getNetworkChannel(&nw_channel);
    lib_settings->getNodeRole(&node_role);

    LOG(LVL_INFO, "node_address: %u, nw_address: %u, nw_ch: %d, role: %d",
        node_address, nw_address, nw_channel, node_role);
}

#if defined (USE_PERSISTENT_FACTORY_SET)
persistent_res_e App_Settings_setfactory(persistent_settings_t * persistent_data)
{
    uint8_t buffer[MAX_READ_SIZE+1] = {0};

    if (sizeof(persistent_settings_t) <= MAX_READ_SIZE)
    {
        memcpy(buffer, &persistent_data, sizeof(persistent_settings_t));
        return Mcu_Persistent_write(&buffer[0], 0, MAX_READ_SIZE);
    }
    else
    {
        return PERSISTENT_RES_DATA_AREA_OVERFLOW;
    }
}
#endif
