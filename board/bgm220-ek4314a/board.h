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

// List of GPIO pins
#define BOARD_GPIO_PIN_LIST             {{GPIO_PORTA, 4},\
                                         {GPIO_PORTC, 7},\
                                         {GPIO_PORTA, 5}, /* usart tx pin */\
                                         {GPIO_PORTA, 6}} /* usart rx pin */

// User friendly name for GPIOs (IDs mapped to the BOARD_GPIO_PIN_LIST table)
#define BOARD_GPIO_ID_LED0              0 // mapped to pin PA04
#define BOARD_GPIO_ID_BUTTON0           1 // mapped to pin PC07
#define BOARD_GPIO_ID_USART_TX          2 // mapped to pin PA05
#define BOARD_GPIO_ID_USART_RX          3 // mapped to pin PA06

// List of LED IDs
#define BOARD_LED_ID_LIST               {BOARD_GPIO_ID_LED0}

// Active high polarity for LEDs
#define BOARD_LED_ACTIVE_LOW            false

// List of button IDs
// NOTE! EFR32xG22 can wake up from deep sleep (EM2) using GPIO input trigger
//       only from A or B ports. Having the button in port C prevents button
//       to be used for waking up from deep sleep.
#define BOARD_BUTTON_ID_LIST           {BOARD_GPIO_ID_BUTTON0}

// Active low polarity for buttons
#define BOARD_BUTTON_ACTIVE_LOW         true

// Board has external pull-up for buttons
#define BOARD_BUTTON_INTERNAL_PULL      false

#endif /* BOARD_BGM220_EK4314A_BOARD_H_ */
