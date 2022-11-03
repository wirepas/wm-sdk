/* Copyright 2020 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/**
 * @file
 *
 * Board definition for a board composed of a Wirepas Evaluation Kit mother board
 * and a silabs <a href="https://www.silabs.com/documents/public/user-guides/ug507-xg23-rb4210a.pdf">brd4210a radio module</a>
 */
#ifndef BOARD_WIREPAS_BRD4210A_BOARD_H_
#define BOARD_WIREPAS_BRD4210A_BOARD_H_

// Waps usart defines
#define BOARD_USART_ID                  0

#define BOARD_USART_GPIO_PORT           GPIO_PORTA
#define BOARD_USART_TX_PIN              8
#define BOARD_USART_RX_PIN              9

// Enadle vcom in silabs kit board
#define BOARD_USART_VCOM_PORT           GPIO_PORTB
#define BOARD_USART_VCOM_PIN            0

// List of GPIO ports and pins for the LEDs on the board:
#define BOARD_LED_PIN_LIST              {{GPIO_PORTB, 2}, {GPIO_PORTD, 3}}

// Active high polarity for LEDs
#define BOARD_LED_ACTIVE_LOW            false

// List of ext. ints, GPIO ports and pins for buttons on the board: PB0, PB1
#define BOARD_BUTTON_PIN_LIST           {{0, GPIO_PORTB, 1},{2, GPIO_PORTB, 3}}

// Active low polarity for buttons
#define BOARD_BUTTON_ACTIVE_LOW         true

// Board has external pull-up for buttons
#define BOARD_BUTTON_INTERNAL_PULL      false

// Buttons use even external interrupts
#define BOARD_BUTTON_USE_EVEN_INT       true

#endif /* BOARD_WIREPAS_BRD4210A_BOARD_H_ */
