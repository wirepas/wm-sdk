/* Copyright 2020 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */
#ifndef RADIO_POWER_TABLE_EFR32XG21_20DBM_H_
#define RADIO_POWER_TABLE_EFR32XG21_20DBM_H_

#include "radio_config.h"
#include "rail_chip_specific.h"

/**
 * Power table for efr32xg21 +20dBm.
 * */
const app_lib_radio_cfg_power_t power_table_efr32xg21_20dBm =
{
    .rx_current     = 880,  // 8.8 mA RX current
    .rx_gain_dbm    = 0,    // 0 dBm RX gain
    .power_count    = 8,    // 8 power levels
    .powers =
    {
        //todo: Current consumption figures to be checked
        { 1,  -15, 1, 164 },    // -14.60 dBm, 16.37 mA
        { 2,  -11, 1, 176 },    // -11,20 dBm, 17.62 mA
        { 5,   -5, 1, 214 },    // -5.4 dBm, 21.35 mA
        { 10,   0, 1, 272 },    // -0.5 dBm, 27.23 mA
        { 20,   5,  1, 412 },   // 5.00 dBm, 41.21 mA
        { 37,  10,  1, 635 },   // 10.00 dBm, 63.45 mA
        { 70,  15,  1, 1014 },  // 14.5 dBm, 101.37 mA
        { 180, 20,  1, 2000 },  // 20 dBm, 200mA
    },
};

/**
 *  The default radio radio configuration is set to MP (Medium Power) which
 *  can achieve max. +10dbm TX power. For higher power levels the TX mode needs
 *  to be changed to HP (High Power) by defining the flag RADIO_CUSTOM_PA_CONFIG
 *  and changing the '.mode' to HP as done in this example.
 * */
#define RADIO_CUSTOM_PA_CONFIG

#if defined RADIO_CUSTOM_PA_CONFIG
// PA configuration for efr32xg21 +20dbm
const RAIL_TxPowerConfig_t pa_config_efr32xg21_20dBm =
    {
        .mode = RAIL_TX_POWER_MODE_2P4GIG_HP,
        .voltage = 3300,
        .rampTime = 10,
    };
#endif

#if defined RADIO_CUSTOM_POWER_TABLE_H
__STATIC_INLINE const app_lib_radio_cfg_power_t * get_custom_power_table(void)
{
    return &power_table_efr32xg21_20dBm;
}
#endif

#if defined RADIO_CUSTOM_PA_CONFIG
__STATIC_INLINE const RAIL_TxPowerConfig_t * get_custom_pa_config(void)
{
    return &pa_config_efr32xg21_20dBm;
}
#endif

#endif /* RADIO_POWER_TABLE_EFR32XG21_20DBM_H_ */
