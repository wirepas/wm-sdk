/* Copyright 2020 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/**
 * @file
 *
 * Board definition for a board composed of a Wirepas Evaluation Kit mother board
 * and a silabs <a href="https://www.silabs.com/documents/public/reference-manuals/brd4183a-rm.pdf">brd4183a radio module</a>
 */
#ifndef BOARD_WIREPASEFR32XG22KIT_BOARD_H_
#define BOARD_WIREPASEFR32XG22KIT_BOARD_H_

// Waps usart defines
#define BOARD_USART_ID                  0

#define BOARD_USART_GPIO_PORT           GPIO_PORTA
#define BOARD_USART_TX_PIN              5
#define BOARD_USART_RX_PIN              6

// List of GPIO ports and pins for the LEDs on the board: LED1 (LED2 not connected)
#define BOARD_LED_PIN_LIST {{GPIO_PORTB, 1}}

// Active high polarity for LEDs
#define BOARD_LED_ACTIVE_LOW false

#endif /* BOARD_WIREPASEFR32XG22KIT_BOARD_H_ */
