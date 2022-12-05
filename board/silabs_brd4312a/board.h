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

// List of GPIO pins
#define BOARD_GPIO_PIN_LIST            {{GPIO_PORTB, 0}, /* PB00 */\
                                        {GPIO_PORTB, 1}, /* PB01 */\
                                        {GPIO_PORTA, 5}, /* PA05. usart tx pin */\
                                        {GPIO_PORTA, 6}, /* PA06. usart rx pin */\
                                        {GPIO_PORTB, 4}} /* PB04. usart vcom pin */

// User friendly name for GPIOs (IDs mapped to the BOARD_GPIO_PIN_LIST table)
#define BOARD_GPIO_ID_LED0              0  // mapped to pin PB00
#define BOARD_GPIO_ID_BUTTON0           1  // mapped to pin PB01
#define BOARD_GPIO_ID_USART_TX          2  // mapped to pin PA05
#define BOARD_GPIO_ID_USART_RX          3  // mapped to pin PA06
#define BOARD_GPIO_ID_USART_VCOM        4  // mapped to pin PB04

// List of LED IDs
#define BOARD_LED_ID_LIST              {BOARD_GPIO_ID_LED0}

// Active high polarity for LEDs
#define BOARD_LED_ACTIVE_LOW            true

// List of button IDs
#define BOARD_BUTTON_ID_LIST           {BOARD_GPIO_ID_BUTTON0}

// Active low polarity for buttons
#define BOARD_BUTTON_ACTIVE_LOW         true

// Board has external pull-up for buttons
#define BOARD_BUTTON_INTERNAL_PULL      false

#endif /* BOARD_SILABS_BRD4312A_BOARD_H_ */
