/* Copyright 2018 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/**
 * @file
 *
 * Board definition for a board composed of a
 * <a href="https://www.silabs.com/products/development-tools/wireless/proprietary/slwstk6005a-sub-ghz-bluetooth-multiband-wireless-starter-kit">Silabs starter kit</a>
 * and a <a href="https://www.silabs.com/documents/public/reference-manuals/brd4253a-rm.pdf">brd4253a radio module</a>
 */
#ifndef BOARD_SILABSEFR32KIT_BOARD_H_
#define BOARD_SILABSEFR32KIT_BOARD_H_

// NOTE! The VCOM on the kit board supports ONLY the standart baud rates
// e.g. 125000 is not working.

// Waps usart defines
#define BOARD_USART_ID                  0

#define BOARD_USART_GPIO_PORT           GPIOA
#define BOARD_USART_TX_PIN              0
#define BOARD_USART_RX_PIN              1
// ROUTELOC are dependent on GPIO defined above and mapping
// can be founded chip datasheet from Silabs
#define BOARD_USART_ROUTELOC_RXLOC      USART_ROUTELOC0_RXLOC_LOC0
#define BOARD_USART_ROUTELOC_TXLOC      USART_ROUTELOC0_TXLOC_LOC0

// Interrupt pin for dual mcu app, unread indication
#define BOARD_UART_INT_PIN              8
#define BOARD_UART_INT_PORT             GPIOD

// Enadle vcom in silabs kit board
// NOTE! To enable virtual com port (VCOM): When the target device drives the
// VCOM_ENABLE (PA5) signal high, a communication line to the Board Controller
// is enabled.
#define BOARD_USART_VCOM_PORT           GPIOA
#define BOARD_USART_VCOM_PIN            5

// For further information about Silicon Labs Kit board pin configuration see:
// UG265: EFR32FG12 2400/868 MHz 10 dBm Wireless Starter Kit User's Guide

// List of GPIO ports and pins for the LEDs on the board: LED0, LED1
#define BOARD_LED_PIN_LIST {{GPIOF, 4}, {GPIOF, 5}}

// Active high polarity for LEDs
#define BOARD_LED_ACTIVE_LOW false

// List of ext. ints, GPIO ports and pins for buttons on the board: PB0, PB1
#define BOARD_BUTTON_PIN_LIST {{4, GPIOF, 6}, {6, GPIOF, 7}}

// Active low polarity for buttons
#define BOARD_BUTTON_ACTIVE_LOW true

// Board has external pull-up for buttons
#define BOARD_BUTTON_INTERNAL_PULL false

// Buttons use even external interrupts
#define BOARD_BUTTON_USE_EVEN_INT true


#endif /* BOARD_SILABSEFR32KIT_BOARD_H_ */
