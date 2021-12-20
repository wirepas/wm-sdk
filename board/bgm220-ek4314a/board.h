/* Copyright 2020 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/**
 * @file
 *
 * Board definition for a Silabs' board named as "Explorer Kit BGM220 Module"
 * Part number BGM220-EK4314A
 */
#ifndef BOARD_BGM220_EK4314A_BOARD_H_
#define BOARD_BGM220_EK4314A_BOARD_H_

// Waps usart defines
#define BOARD_USART_ID                  0

#define BOARD_USART_GPIO_PORT           GPIO_PORTA
#define BOARD_USART_TX_PIN              5
#define BOARD_USART_RX_PIN              6

// List of GPIO ports and pins for the LEDs on the board:
#define BOARD_LED_PIN_LIST {{GPIO_PORTA, 4}}

// Active high polarity for LEDs
#define BOARD_LED_ACTIVE_LOW false

// List of ext. ints, GPIO ports and pins for buttons on the board:
// NOTE! EFR32xG22 can wake up from deep sleep (EM2) using GPIO input trigger
//       only from A or B ports. Having the button in port C prevents button
//       to be used for waking up from deep sleep.
#define BOARD_BUTTON_PIN_LIST           {{4, GPIO_PORTC, 7}}

// Active low polarity for buttons
#define BOARD_BUTTON_ACTIVE_LOW         true

// Board has external pull-up for buttons
#define BOARD_BUTTON_INTERNAL_PULL      false

// Buttons use even external interrupts
#define BOARD_BUTTON_USE_EVEN_INT       true

// Buttons use even external interrupts
//#define BOARD_BUTTON_USE_EVEN_INT true

#endif /* BOARD_BGM220_EK4314A_BOARD_H_ */
