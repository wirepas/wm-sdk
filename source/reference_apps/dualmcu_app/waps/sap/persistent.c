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
    union
    {
        struct
        {
            // Autostart bit
            uint8_t     autostart:1;
            uint8_t     firstboot:1;
            uint8_t     reserved:6;
        };
        uint8_t flags;
    };
    // Multicast groups
    multicast_group_addr_t multicast_groups[MULTICAST_ADDRESS_AMOUNT];
} persistent_area_t;

// Persistent area
static persistent_area_t m_persistent_image;

/**
 * \brief   Load persistent area from flash if necessary
 * \return  Status of the operation
 */
static app_res_e update_image(void)
{
    return lib_storage->readPersistent(&m_persistent_image,
                                       sizeof(m_persistent_image));
}

/**
 * \brief   Write persistent area
 */
static app_res_e write_image(void)
{
    return lib_storage->writePersistent(&m_persistent_image,
                                        sizeof(m_persistent_image));
}

void Persistent_init(void)
{
    update_image();

    // After flash, app persistent is filled with 0xFF.
    if (m_persistent_image.flags == 0xFF)
    {
        m_persistent_image.flags = 0;
        write_image();
    }
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

app_res_e Persistent_isFirstboot(bool * firstboot)
{
    app_res_e retval = update_image();
    *firstboot = !m_persistent_image.firstboot;
    return retval;
}

app_res_e Persistent_setFirstboot(bool firstboot)
{
    app_res_e retval = update_image();
    m_persistent_image.firstboot = !firstboot;
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
