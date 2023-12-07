/* Copyright 2023 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */
#ifndef RADIO_POWER_TABLE_EFR32XG13_12DBM_H_
#define RADIO_POWER_TABLE_EFR32XG13_12DBM_H_

/**
 * Power table for efr32xg13 +12dBm.
 * */
const app_lib_radio_cfg_power_t power_table_efr32xg13_12dBm =
{
    .rx_current     = 100, // 10 mA RX current
    .rx_gain_db     = 0,   // 0 dB RX gain
    .power_count    = 6,   // 6 power levels
    .powers =
    {
        {   5, -12, 1, 100 }, // -12.00 dBm, 10 mA
        {  12,  -5, 1, 130 }, //  -5.31 dBm, 13 mA
        {  21,   0, 1, 170 }, //  -0.18 dBm, 17 mA
        {  33,   4, 1, 220 }, //   3.81 dBm, 22 mA
        {  54,   8, 1, 310 }, //   7.93 dBm, 31 mA
        {  87,  12, 1, 440 }, //  12.08 dBm, 44 mA
    },
};

#if defined RADIO_CUSTOM_POWER_TABLE_H
__STATIC_INLINE const app_lib_radio_cfg_power_t * get_custom_power_table(void)
{
    return &power_table_efr32xg13_12dBm;
}
#endif

#endif /* RADIO_POWER_TABLE_EFR32XG13_12DBM_H_ */
