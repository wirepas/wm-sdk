/* Copyright 2018 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/**
 * @file
 *
 * Board definition for the
 * <a href="https://www.silabs.com/products/development-tools/thunderboard/thunderboard-sense-two-kit">Silabs Thunderboard Sense 2</a>
 */
#ifndef BOARD_TBSENSE2_BOARD_H_
#define BOARD_TBSENSE2_BOARD_H_

// Waps usart defines
#if defined USE_FTDI
#define BOARD_USART_ID                  1
#define BOARD_USART_GPIO_PORT           GPIOF
#define BOARD_USART_TX_PIN              3
#define BOARD_USART_RX_PIN              4
// ROUTELOC are dependent on GPIO defined above and mapping
// can be founded chip datasheet from Silabs
#define BOARD_USART_ROUTELOC_RXLOC      USART_ROUTELOC0_RXLOC_LOC27
#define BOARD_USART_ROUTELOC_TXLOC      USART_ROUTELOC0_TXLOC_LOC27

#else
#define BOARD_USART_ID                  0
#define BOARD_USART_GPIO_PORT           GPIOA
#define BOARD_USART_TX_PIN              0
#define BOARD_USART_RX_PIN              1

// VCOM port only supports 115200 baudrate
// This speed will be used independently of UART_BAUDRATE flag value
#define BOARD_USART_FORCE_BAUDRATE      115200

// ROUTELOC are dependent on GPIO defined above and mapping
// can be founded chip datasheet from Silabs
#define BOARD_USART_ROUTELOC_RXLOC      USART_ROUTELOC0_RXLOC_LOC0
#define BOARD_USART_ROUTELOC_TXLOC      USART_ROUTELOC0_TXLOC_LOC0
#endif

// Interrupt pin for dual mcu app, unread indication
#define BOARD_UART_INT_PIN              6
#define BOARD_UART_INT_PORT             GPIOF

// I2C configuration: SDA on PC4, SCL on PC5 (ENV_I2C on Thunderboard Sense 2)
#define USE_I2C1
#define BOARD_I2C_GPIO_PORT             GPIOC
#define BOARD_I2C_SDA_PIN               4
#define BOARD_I2C_SCL_PIN               5
#define BOARD_I2C_ROUTELOC_SDALOC I2C_ROUTELOC0_SDALOC_LOC17
#define BOARD_I2C_ROUTELOC_SCLLOC I2C_ROUTELOC0_SCLLOC_LOC17

// List of GPIO ports and pins for the LEDs on the board: red LED, green LED
#define BOARD_LED_PIN_LIST              {{GPIOD, 8}, {GPIOD, 9}}

// Active high polarity for LEDs
#define BOARD_LED_ACTIVE_LOW            false

// List of ext. ints, GPIO ports and pins for buttons on the board: BTN0, BTN1
#define BOARD_BUTTON_PIN_LIST           {{12, GPIOD, 14}, {14, GPIOD, 15}}

// Active low polarity for buttons
#define BOARD_BUTTON_ACTIVE_LOW         true

// Board has external pull-up for buttons
#define BOARD_BUTTON_INTERNAL_PULL false

// Buttons use even external interrupts
#define BOARD_BUTTON_USE_EVEN_INT       true


#endif /* BOARD_TBSENSE2_BOARD_H_ */
