/* Copyright 2020 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */
#ifndef RADIO_POWER_TABLE_EFR32XG12_19DBM_H_
#define RADIO_POWER_TABLE_EFR32XG12_19DBM_H_

// +19 dBm power table for efr32xg12, with 8 power levels
const app_lib_radio_cfg_power_t power_table_efr32xg12_19dBm =
{
    .rx_current     = 100,  // 10 mA RX current
    .rx_gain_dbm    = 0,    // 0 dBm RX gain
    .power_count    = 8,    // 8 power levels
    .powers =
    {
        {5,     -15,    1,  99},
        {9,     -10,    1,  114},
        {15,    -5,     1,  136},
        {28,    0,      1,  183},
        {46,    5,      1,  268},
        {83,    10,     1,  439},
        {150,   15,     1,  732},
        {255,   19,     1,  1160},
    },
};

#if defined RADIO_CUSTOM_POWER_TABLE_H
__STATIC_INLINE const app_lib_radio_cfg_power_t * get_custom_power_table(void)
{
    return &power_table_efr32xg12_19dBm;
}
#endif

#endif /* RADIO_POWER_TABLE_EFR32XG12_19DBM_H_ */
