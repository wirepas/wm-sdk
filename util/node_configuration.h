/* Copyright 2017 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

#ifndef NODECONFIGURATION_H_
#define NODECONFIGURATION_H_

#include "api.h"
#include "uniqueId.h"

/**
 * \brief   Helper function to initially setup a node if not already
 *          configured. This configuration can be modified later with
 *          remote API.
 *
 * Example on use. Address, network address and channel is set if they are not
 * yet set:
 *
 * @code
 *
 * #define NETWORK_ADDRESS  0x123456
 * #define NETWORK_CHANNEL  1
 *
 * void App_init(const app_global_functions_t * functions)
 * {
 *     // Open Wirepas public API
 *     API_Open(functions);
 *
 *     // Basic configuration of the node with a unique node address
 *     if (configureNode(getUniqueAddress(),
 *                       NETWORK_ADDRESS,
 *                       NETWORK_CHANNEL) != APP_RES_OK)
 *     {
 *         // Could not configure the node
 *         // It should not happen except if one of the config value is invalid
 *         return;
 *     }
 *     ...
 * }
 * @endcode
 *
 * \param   my_addr
 *          Node Address to set if not present
 * \param   my_network_addr
 *          Network address to set if not present
 * \param   my_network_ch
 *          Network channel to set if not present
 * \return  APP_RES_OK if the configuration is successful, an error
 *          code otherwise
 * \note    API_Open() MUST be called before using this function
 * \note    Only unset parameters are set
 */
__STATIC_INLINE app_res_e configureNode(app_addr_t my_addr,
                                        app_lib_settings_net_addr_t my_network_addr,
                                        app_lib_settings_net_channel_t my_network_ch)
{
    // Check node address
    app_addr_t node_addr;
    app_res_e res;

    if (lib_settings->getNodeAddress(&node_addr) != APP_RES_OK)
    {
        // Not set
        res = lib_settings->setNodeAddress(my_addr);
        if (res != APP_RES_OK)
        {
            return res;
        }
    }

    // Check network address
    app_lib_settings_net_addr_t network_addr;
    if (lib_settings->getNetworkAddress(&network_addr) != APP_RES_OK)
    {
        // Not set
        res = lib_settings->setNetworkAddress(my_network_addr);
        if (res != APP_RES_OK)
        {
            return res;
        }
    }

    // Check network channel
    app_lib_settings_net_channel_t network_ch;
    if (lib_settings->getNetworkChannel(&network_ch) != APP_RES_OK)
    {
        // Not set
        res = lib_settings->setNetworkChannel(my_network_ch);
        if (res != APP_RES_OK)
        {
            return res;
        }
    }

    return APP_RES_OK;
}

/**
 * \brief   Helper function to generate a unique unicast address
 * \return  A unique unicast address generated from chip unique id
 */
__STATIC_INLINE app_addr_t getUniqueAddress()
{
    app_addr_t address = getUniqueId();

    // Check the address is in correct range
    if (address == 0x0)
    {
        // Zero is not a valid address
        address = 1;
    }
    else if ((address & 0xFF000000) == 0x80000000)
    {
        // Address would be in the multicast space
        address &= 0x7ffffffd;
    }

    // Workaround:
    // there is a bug in Wirepas reference gateway fixed in version 1.3.
    // It doesn't forward messages from nodes with an address with the
    // MSB set to one.
    // In order to avoid this situation, explicitly reset the highest bit when
    // generating a unique address.
    // If the target network only has gateways with version >= 1.3, or with different
    // gateway implementation it is safe to remove this line
    address &= 0x7fffffff;

    return address;
}

/**
 * \brief   Helper function to apply new configuration on a node.
 *          Will override old configuration.
 * \param   new_addr
 *          Node Address to set
 * \param   new_role
 *          Node Role to set
 * \param   new_network_addr
 *          Network address to set
 * \param   new_network_ch
 *          Network channel to set
 * \return  APP_RES_OK if the configuration is successful, an error
 *          code otherwise
 * \note    API_Open() MUST be called before using this function
 */
__STATIC_INLINE app_res_e OverrideNodeConfig(app_addr_t new_addr,
                                             app_lib_settings_role_t new_role,
                                             app_lib_settings_net_addr_t
                                             new_network_addr,
                                             app_lib_settings_net_channel_t
                                             new_network_ch)
{
    app_res_e res;

    res = lib_settings->setNodeAddress(new_addr);
    if (res != APP_RES_OK)
    {
        return res;
    }
    res = lib_settings->setNodeRole(new_role);
    if (res != APP_RES_OK)
    {
        return res;
    }
    res = lib_settings->setNetworkAddress(new_network_addr);
    if (res != APP_RES_OK)
    {
        return res;
    }
    res = lib_settings->setNetworkChannel(new_network_ch);
    if (res != APP_RES_OK)
    {
        return res;
    }

    return APP_RES_OK;
}

#endif /* NODECONFIGURATION_H_ */
