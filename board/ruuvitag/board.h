/* Copyright 2018 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/**
 * @file
 *
 * Board definition for the
 * <a href="https://ruuvi.com/ruuvitag-specs/">Ruuvitag device</a>
 */
#ifndef BOARD_RUUVITAG_H_
#define BOARD_RUUVITAG_H_

// Define the SPI instance to use
#define USE_SPI0
// SPI Port pins
#define BOARD_SPI_SCK_PIN   29
#define BOARD_SPI_MOSI_PIN  25
#define BOARD_SPI_MISO_PIN  28

// SPI Chip Select for various sensor
#define BOARD_SPI_LIS2DH12_CS_PIN 8
#define BOARD_SPI_BME280_CS_PIN   3

// List of GPIOs for the leds on the board (LED1 to LED2)
#define BOARD_LED_PIN_LIST {17, 19}

// The board supports DCDC
#define BOARD_SUPPORT_DCDC


#endif /* BOARD_RUUVITAG_H_ */
