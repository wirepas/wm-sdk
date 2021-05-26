/* Copyright 2020 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

#include "api.h"

void Board_custom_init()
{
    // Mark two channels as reserved
    // In fact because of hardware issue, channels BLE #13 (2432 MHz)
    // and BLE #29 (2464 MHz) are unusable.
    // It corresponds to channel #14 and #30 in Wirepas space.

    // It doesn't prevent usage of those channels by other boards in the
    // network but at least this board will prevent the usage of those channels
    // for its own cluster channel.
    uint8_t res_chns[4];

    // Channel 1 is bit 0 of byte 0 :
    // - channel 14 is bit 5 (14 mod 8) of byte 1 (14 / 8)
    // - channel 30 is bit 5 (30 mod 8) of byte 3 (30 / 8)

    res_chns[0] = 0;
    res_chns[1] = 1 << 5;
    res_chns[2] = 0;
    res_chns[3] = 1 << 5;

    lib_settings->setReservedChannels(res_chns, sizeof(res_chns));
}

