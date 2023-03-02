/* Copyright 2022 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */
#ifndef RADIO_POWER_TABLE_EFR32XG23_13DBM_H_
#define RADIO_POWER_TABLE_EFR32XG23_13DBM_H_

#include "rail_chip_specific.h"

/**
 * Power table for efr32xg23 +13dBm.
 * */
const app_lib_radio_cfg_power_t power_table_efr32xg23_13dBm =
{
    .rx_current     = 69, // 6.9 mA RX current
    .rx_gain_dbm    = 0,  // 0 dBm RX gain
    .power_count    = 8,  // 8 power levels
    .powers =
    {
        {   2, -15, 1, 100 }, // -15.46 dBm, 10 mA
        {   5,  -7, 1, 120 }, //  -6.72 dBm, 12 mA
        {   7,  -4, 1, 140 }, //  -3.81 dBm, 14 mA
        {  11,   0, 1, 160 }, //  -0.18 dBm, 16 mA
        {  18,   4, 1, 210 }, //   3.81 dBm, 21 mA
        {  30,   8, 1, 300 }, //   7.93 dBm, 30 mA
        {  45,  11, 1, 390 }, //  11.13 dBm, 39 mA
        {  55,  13, 1, 460 }, //  12.94 dBm, 46 mA
    },
};

#if defined RADIO_CUSTOM_POWER_TABLE_H
__STATIC_INLINE const app_lib_radio_cfg_power_t * get_custom_power_table(void)
{
    return &power_table_efr32xg23_13dBm;
}
#endif

#endif /* RADIO_POWER_TABLE_EFR32XG23_13DBM_H_ */
