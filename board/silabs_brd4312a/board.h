/* Copyright 2021 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/**
 * @file
 *
 * Board definition for a board composed of a
 * <a href="https://www.silabs.com/products/development-tools/wireless/proprietary/slwstk6005a-sub-ghz-bluetooth-multiband-wireless-starter-kit">Silabs starter kit</a>
 * and a <a href="https://www.silabs.com/documents/public/user-guides/ug458-brd4312a-user-guide.pdf">brd4312a radio module</a>
 */
#ifndef BOARD_SILABS_BRD4312A_BOARD_H_
#define BOARD_SILABS_BRD4312A_BOARD_H_

// Waps usart defines
#define BOARD_USART_ID                  0

#define BOARD_USART_GPIO_PORT           GPIO_PORTA
#define BOARD_USART_TX_PIN              5
#define BOARD_USART_RX_PIN              6

// Enadle vcom in silabs kit board
#define BOARD_USART_VCOM_PORT           GPIO_PORTB
#define BOARD_USART_VCOM_PIN            4

// List of GPIO ports and pins for the LEDs on the board:
#define BOARD_LED_PIN_LIST              {{GPIO_PORTB, 0}}  // Only LED0
//If both LEDs are defined, buttons cannot be configured
//#define BOARD_LED_PIN_LIST              {{GPIO_PORTB, 0},{GPIO_PORTB, 1}}  // Both LED0 and LED1

// Active high polarity for LEDs
#define BOARD_LED_ACTIVE_LOW            true

// List of ext. ints, GPIO ports and pins for buttons on the board:
#define BOARD_BUTTON_PIN_LIST           {{0, GPIO_PORTB, 1}} // Only button1
//If both buttons are defined, LEDs cannot be configured
//#define BOARD_BUTTON_PIN_LIST           {{0, GPIO_PORTB, 0},{2, GPIO_PORTB, 1}} // Both button0 and button1

// Active low polarity for buttons
#define BOARD_BUTTON_ACTIVE_LOW         true

// Board has external pull-up for buttons
#define BOARD_BUTTON_INTERNAL_PULL      false

// Buttons use even external interrupts
#define BOARD_BUTTON_USE_EVEN_INT       true

#endif /* BOARD_SILABS_BRD4312A_BOARD_H_ */
