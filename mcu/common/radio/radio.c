/* Copyright 2020 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */
#include "board.h"
#include "mcu.h"
#include "api.h"

// Include custom power table
#if defined RADIO_CUSTOM_POWER_TABLE_H
#include RADIO_CUSTOM_POWER_TABLE_H
#endif

void Radio_setPowerTable(const app_lib_radio_cfg_power_t * power_table)
{
    if (lib_radio_cfg)
    {
        lib_radio_cfg->powerSetup(power_table);
    }
}

void Radio_setPA(const void * pa_cfg)
{
    if (lib_radio_cfg)
    {
        lib_radio_cfg->paSetup(pa_cfg);
    }
}

// Perform application level radio init, i.e. set custom power table if any
void Radio_init(void)
{
#if defined RADIO_CUSTOM_POWER_TABLE_H
    Radio_setPowerTable(get_custom_power_table());
    #if defined RADIO_CUSTOM_PA_CONFIG
    Radio_setPA(get_custom_pa_config());
    #endif
#endif

}
