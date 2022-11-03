/* Copyright 2017 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

#ifndef SOURCE_WAPS_APP_WAPS_WAPS_PRIVATE_H_
#define SOURCE_WAPS_APP_WAPS_WAPS_PRIVATE_H_

#include <stdint.h>

#include "wms_settings.h"

/**
 * \brief   Get information about queued indications
 * \return  1 if indications queued
 *          0 if indications not queued
 */
uint8_t queued_indications(void);

/**
 * \brief   Wake WAPS up (something to do)
 */
void wakeup_task(void);

/**
 * \brief   Get number of channels available
 * \return  Number of channels
 * \note    Value is cached on startup
 */
app_lib_settings_net_channel_t get_num_channels(void);

#endif /* SOURCE_WAPS_APP_WAPS_WAPS_PRIVATE_H_ */
