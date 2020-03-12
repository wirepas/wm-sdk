/* Copyright 2018 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

#include "persistent.h"
#include <string.h>

/**
 * \brief   Persistent area that is used via lib_storage library
 */
typedef struct __attribute__ ((__packed__))
{
    // Autostart bit
    uint8_t     autostart;
    // Multicast groups
    multicast_group_addr_t multicast_groups[MULTICAST_ADDRESS_AMOUNT];
} persistent_area_t;

// Persistent area
static persistent_area_t m_persistent_image;
// Shortcut whether area is valid
static bool m_persistent_valid;

/**
 * \brief   Load persistent area from flash if necessary
 * \return  Status of the operation
 */
static app_res_e update_image(void)
{
    if (!m_persistent_valid)
    {
        m_persistent_valid = true;
        return lib_storage->readPersistent(&m_persistent_image,
                                           sizeof(m_persistent_image));
    }
    return APP_RES_OK;
}

/**
 * \brief   Write persistent area
 */
static app_res_e write_image(void)
{
    return lib_storage->writePersistent(&m_persistent_image,
                                        sizeof(m_persistent_image));
}

app_res_e Persistent_getAutostart(bool * autostart)
{
    app_res_e retval = update_image();
    *autostart = m_persistent_image.autostart;
    return retval;
}

app_res_e Persistent_setAutostart(bool autostart)
{
    app_res_e retval = update_image();
    m_persistent_image.autostart = autostart;
    if (retval == APP_RES_OK)
    {
        retval = write_image();
    }
    return retval;
}

app_res_e Persistent_getGroups(multicast_group_addr_t * addr)
{
    app_res_e retval = update_image();
    memcpy(addr,
           &m_persistent_image.multicast_groups[0],
           sizeof(m_persistent_image.multicast_groups));
    return retval;
}

app_res_e Persistent_setGroups(multicast_group_addr_t * addr)
{
    app_res_e retval = update_image();
    memcpy(&m_persistent_image.multicast_groups[0],
           addr,
           sizeof(m_persistent_image.multicast_groups));
    if (retval == APP_RES_OK)
    {
        retval = write_image();
    }
    return retval;
}
