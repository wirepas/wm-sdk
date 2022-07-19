/* Copyright 2020 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */
#ifndef RADIO_POWER_TABLE_NRF52840_4DBM_H_
#define RADIO_POWER_TABLE_NRF52840_4DBM_H_

// For RADIO_TX_POWER_XXX
#include "mcu/nrf/common/vendor/mdk/nrf52840_bitfields.h"

// +4 dBm power table for nrf52840, with 8 power levels
const app_lib_radio_cfg_power_t power_table_nrf52840_4dBm =
{
    .rx_current     = 46,   // 4.6 mA RX current
    .rx_gain_dbm    = 0,    // 0 dBm RX gain
    .power_count    = 8,    // 8 power levels
    .powers =
    {
        {RADIO_TXPOWER_TXPOWER_Neg40dBm, -40, 1, 23},
        {RADIO_TXPOWER_TXPOWER_Neg20dBm, -20, 1, 27},
        {RADIO_TXPOWER_TXPOWER_Neg16dBm, -16, 1, 28},
        {RADIO_TXPOWER_TXPOWER_Neg12dBm, -12, 1, 30},
        {RADIO_TXPOWER_TXPOWER_Neg8dBm,  -8,  1, 33},
        {RADIO_TXPOWER_TXPOWER_Neg4dBm,  -4,  1, 37},
        {RADIO_TXPOWER_TXPOWER_0dBm,      0,  1, 48},
        {RADIO_TXPOWER_TXPOWER_Pos4dBm,   4,  1, 96},
    },
};

#if defined RADIO_CUSTOM_POWER_TABLE_H
__STATIC_INLINE const app_lib_radio_cfg_power_t * get_custom_power_table(void)
{
    return &power_table_nrf52840_4dBm;
}
#endif

#endif /* RADIO_POWER_TABLE_NRF52840_4DBM_H_ */
