/* Copyright 2019 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/**
 * @file
 *
 * Board definition for the
 * <a href="https://promistel.com/en/iot.html">Promistel raspberry pi hat</a>
 */
#ifndef BOARD_PROMISTEL_RPI_HAT_BOARD_H_
#define BOARD_PROMISTEL_RPI_HAT_BOARD_H_

// Interrupt pin for dual mcu app, unread indication
#define BOARD_UART_IRQ_PIN              23

// Serial port pins
#define BOARD_USART_TX_PIN              45 //P1.13
#define BOARD_USART_RX_PIN              29
#define BOARD_USART_CTS_PIN             7  /* For USE_USART_HW_FLOW_CONTROL */
#define BOARD_USART_RTS_PIN             5  /* For USE_USART_HW_FLOW_CONTROL */

// List of GPIOs for the leds on the board (Red, Green, Blue)
#define BOARD_LED_PIN_LIST {15, 16, 24}

// List of GPIOs for the Buttons on the board
#define BOARD_BUTTON_PIN_LIST {14}
#define BOARD_BUTTON_ACTIVE_LOW true

// Define the SPI instance to use
#define USE_SPI0
// SPI Port pins
#define BOARD_SPI_SCK_PIN   19
#define BOARD_SPI_MOSI_PIN  20
#define BOARD_SPI_MISO_PIN  21

// SPI Chip Select pin
#define BOARD_SPI_CS_PIN   17

// The board supports DCDC
#define BOARD_SUPPORT_DCDC

#endif /* BOARD_PROMISTEL_RPI_HAT_BOARD_H_ */
