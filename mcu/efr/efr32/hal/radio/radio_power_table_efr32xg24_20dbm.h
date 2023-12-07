/* Copyright 2023 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */
#ifndef RADIO_POWER_TABLE_EFR32XG24_20DBM_H_
#define RADIO_POWER_TABLE_EFR32XG24_20DBM_H_

#include "rail_types.h"

/**
 * Power table for efr32xg24 +20dBm.
 * */
const app_lib_radio_cfg_power_t power_table_efr32xg24_20dBm =
{
    .rx_current     = 880,  // 8.8 mA RX current
    .rx_gain_db     = 0,    // 0 dB RX gain
    .power_count    = 8,    // 8 power levels
    .powers =
    {
        //todo: Current consumption figures to be checked
        {   1, -17, 1,  150 },  // -17.29 dBm,  15.00 mA
        {   3, -10, 1,  170 },  //  -9.60 dBm,  17.00 mA
        {   5,  -5, 1,  220 },  //  -5.21 dBm,  22.00 mA
        {  10,   0, 1,  300 },  //   0.35 dBm,  30.00 mA
        {  18,   5, 1,  400 },  //   5.01 dBm,  40.00 mA
        {  33,  10, 1,  600 },  //   9.92 dBm,  60.00 mA
        {  63,  15, 1, 1000 },  //  14.99 dBm, 100.00 mA
        { 160,  20, 1, 2000 },  //  19.98 dBm, 200.00 mA
    },
};

#define RADIO_CUSTOM_PA_CONFIG

#if defined RADIO_CUSTOM_PA_CONFIG
// PA configuration for efr32xg24 +20dbm
const RAIL_TxPowerConfig_t pa_config_efr32xg24_20dBm =
    {
        .mode = RAIL_TX_POWER_MODE_2P4GIG_HP,
        .voltage = 3300,
        .rampTime = 10,
    };
#endif

#if defined RADIO_CUSTOM_POWER_TABLE_H
__STATIC_INLINE const app_lib_radio_cfg_power_t * get_custom_power_table(void)
{
    return &power_table_efr32xg24_20dBm;
}
#endif

#if defined RADIO_CUSTOM_PA_CONFIG
__STATIC_INLINE const RAIL_TxPowerConfig_t * get_custom_pa_config(void)
{
    return &pa_config_efr32xg24_20dBm;
}
#endif

#endif /* RADIO_POWER_TABLE_EFR32XG24_20DBM_H_ */
