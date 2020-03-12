/* Copyright 2018 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

#include "hal_api.h"
#include "board.h"
#include "io.h"

void Io_init(void)
{
    nrf_gpio_pin_dir_set(BOARD_SPI_BME280_CS_PIN, NRF_GPIO_PIN_DIR_OUTPUT);
    nrf_gpio_cfg_output(BOARD_SPI_BME280_CS_PIN);
    nrf_gpio_pin_set(BOARD_SPI_BME280_CS_PIN);
}

void Io_select_BME280(bool select)
{
    if (select)
    {
        nrf_gpio_pin_clear(BOARD_SPI_BME280_CS_PIN);
    }
    else
    {
        nrf_gpio_pin_set(BOARD_SPI_BME280_CS_PIN);
    }
}
