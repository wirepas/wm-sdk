/* Copyright 2021 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/**
 * @file
 *
 * Board definition for the
 * <a href="https://www.silabs.com/development-tools/thunderboard/thunderboard-bg22-kit">Silabs Thunderboard BG22</a>
 */
#ifndef BOARD_SILABSBRD4184A_BOARD_H_
#define BOARD_SILABSBRD4184A_BOARD_H_

#define BOARD_USART_ID                  0
#define BOARD_USART_GPIO_PORT           GPIO_PORTA
#define BOARD_USART_TX_PIN              5
#define BOARD_USART_RX_PIN              6

// VCOM port only supports 115200 baudrate
// This speed will be used independently of UART_BAUDRATE flag value
#define BOARD_USART_FORCE_BAUDRATE      115200

// List of GPIO ports and pins for the LEDs on the board: yellow LED
#define BOARD_LED_PIN_LIST              {{GPIO_PORTB, 0}}

// Active high polarity for LEDs
#define BOARD_LED_ACTIVE_LOW            false

// List of ext. ints, GPIO ports and pins for buttons on the board: BTN0
#define BOARD_BUTTON_PIN_LIST           {{0, GPIO_PORTB, 1}}

// Active low polarity for buttons
#define BOARD_BUTTON_ACTIVE_LOW         true

// Board has external pull-up for buttons
#define BOARD_BUTTON_INTERNAL_PULL      false

// Buttons use even external interrupts
#define BOARD_BUTTON_USE_EVEN_INT       true


#endif /* BOARD_SILABSBRD4184A_BOARD_H_ */
