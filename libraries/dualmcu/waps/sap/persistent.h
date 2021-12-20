/* Copyright 2018 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

#ifndef _PERSISTENT_H_
#define _PERSISTENT_H_

#include <stdint.h>
#include <stdbool.h>
#include "api.h"
#include "multicast.h"

/**
 * \brief   Initializes the persistent module.
 *          Sets autoboot and firstboot flags to their default values.
 */
void Persistent_init(void);

/**
 * \brief   Get autostart bit
 * \param   autostart
 *          out: true: autostart is enabled, false: autostart is disabled
 * \return  Status of the operation
 */
app_res_e Persistent_getAutostart(bool * autostart);

/**
 * \brief   Set autostart bit
 * \param   autostart
 *          true: autostart is enabled, false: autostart is disabled
 * \return  Status of the operation
 */
app_res_e Persistent_setAutostart(bool autostart);

/**
 * \brief   Get firstboot flag
 * \param   firstboot
 *          out: true: this is the first boot, false: not the first boot
 * \note    The logic is inversed in the raw persistant storage
 *          (firstboot==true means 0 is stored in flash).
 * \return  Status of the operation
 */
app_res_e Persistent_isFirstboot(bool * firstboot);

/**
 * \brief   Set firstboot flag
 * \param   firstboot
 *          out: true: this is the first boot, false: not the first boot
 * \return  Status of the operation
 */
app_res_e Persistent_setFirstboot(bool firstboot);

/**
 * \brief   Get multicast groups
 * \param   addr
 *          Multicast addresses stored in storage
 * \return  Status of the operation
 */
app_res_e Persistent_getGroups(multicast_group_addr_t * addr);

/**
 * \brief   Set multicast groups
 * \param   addr
 *          Multicast addresses stored in storage
 * \return  Status of the operation
 */
app_res_e Persistent_setGroups(multicast_group_addr_t * addr);

#endif /* SOURCE_DUALMCU_APP_WAPS_SAP_PERSISTENT_H_ */
