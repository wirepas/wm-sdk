/* Copyright 2021 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/**
 * @file
 *
 * Board definition for a board composed of a Wirepas Evaluation Kit mother board
 * and a silabs <a href="https://www.silabs.com/documents/public/schematic-files/BRD4181B-A01-schematic.pdf">brd4181a radio module schematic</a>
 */
#ifndef BOARD_WIREPASEFR32KIT_BOARD_H_
#define BOARD_WIREPASEFR32KIT_BOARD_H_

// Waps usart defines
#define BOARD_USART_ID                  0

#define BOARD_USART_GPIO_PORT           GPIO_PORTA
#define BOARD_USART_TX_PIN              5
#define BOARD_USART_RX_PIN              6

// List of GPIO ports and pins for the LEDs on the board: LED1, LED2
#define BOARD_LED_PIN_LIST {{GPIO_PORTB, 0}, {GPIO_PORTB, 1}}

// Active high polarity for LEDs
#define BOARD_LED_ACTIVE_LOW false


#endif /* BOARD_WIREPASEFR32KIT_BOARD_H_ */
