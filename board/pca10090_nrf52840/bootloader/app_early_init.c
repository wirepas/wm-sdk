/* Copyright 2022 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

#include "hal_api.h"
#include "board.h"

void app_early_init(void)
{
    // Give NRF9160 side control over the flash memory.
    // The application side setting is too slow for cold boot.
    // In app_early_init this is fastest possible, but still
    // bit too slow, thus nRF9160 bootloader needs to poll
    // until it has access to external flash memory.
    nrf_gpio_cfg(BOARD_EXT_MEM_CTRL,
                 NRF_GPIO_PIN_DIR_OUTPUT,
                 NRF_GPIO_PIN_INPUT_CONNECT,
                 NRF_GPIO_PIN_NOPULL,
                 NRF_GPIO_PIN_S0S1,
                 NRF_GPIO_PIN_NOSENSE);
    nrf_gpio_pin_set(BOARD_EXT_MEM_CTRL);
}
