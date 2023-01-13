/* Copyright 2022 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/**
 * @file
 *
 * Board definition for the
 * <a href="https://infocenter.nordicsemi.com/pdf/nRF9160_DK_HW_User_Guide_v1.0.pdf</a>
 */
#ifndef BOARD_PCA10090_NRF52840_BOARD_H_
#define BOARD_PCA10090_NRF52840_BOARD_H_

// NRF_GPIO is mapped to NRF_P0 , for pins P0.00 ... P0.31
// Use NRF_P1 for pins P1.00 ... P1.15
// With nrf_gpio.h, use SW_pin (logical pins, port-aware)

/**
NRF_P0  SW_pin  PCA10090       Notes (recommended usage)
-----------------------------------------------------------------------------
P0.19   19      EXT_MEM_CTRL   external flash routed to 0=nRF52840, 1=nRF9160
*/

// External flash routing
#define BOARD_EXT_MEM_CTRL              19

#endif /* BOARD_PCA10090_NRF52840_BOARD_H_ */
