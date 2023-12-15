/* Copyright 2023 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/**
 * @file
 *
 * Board definition for 
 * <a href="https://www.silabs.com/development-tools/wireless/efr32xg24-explorer-kit">xG24-EK2703A</a>
 */
#ifndef BOARD_SILABS_BRD2703A_BOARD_H_
#define BOARD_SILABS_BRD2703A_BOARD_H_

// VCOM port only supports 115200 baudrate
// This speed will be used independently of UART_BAUDRATE flag value
#define BOARD_USART_FORCE_BAUDRATE      115200

// Serial port
#define BOARD_USART_ID                  0
#define BOARD_USART_TX_PORT             GPIO_PORTA
#define BOARD_USART_TX_PIN              5
#define BOARD_USART_RX_PORT             GPIO_PORTA
#define BOARD_USART_RX_PIN              6

// List of GPIO pins
#define BOARD_GPIO_PIN_LIST            {{GPIO_PORTA, 4}, /* PA04 */\
                                        {GPIO_PORTA, 7}, /* PA07 */\
                                        {GPIO_PORTB, 2}, /* PB02 */\
                                        {GPIO_PORTB, 3}, /* PB03 */\
                                        {GPIO_PORTA, 6}} /* PA06. required by the dual_mcu app. usart wakeup pin (= BOARD_USART_RX) */\

// User friendly name for GPIOs (IDs mapped to the BOARD_GPIO_PIN_LIST table)
#define BOARD_GPIO_ID_LED0              0  // mapped to pin PA04
#define BOARD_GPIO_ID_LED1              1  // mapped to pin PA07
#define BOARD_GPIO_ID_BUTTON0           2  // mapped to pin PB02
#define BOARD_GPIO_ID_BUTTON1           3  // mapped to pin PB03
#define BOARD_GPIO_ID_USART_WAKEUP      4  // mapped to pin PA06

// List of LED IDs
#define BOARD_LED_ID_LIST               {BOARD_GPIO_ID_LED0, BOARD_GPIO_ID_LED1}

// LED GPIO polarity
#define BOARD_LED_ACTIVE_LOW            false

// List of button IDs
#define BOARD_BUTTON_ID_LIST            {BOARD_GPIO_ID_BUTTON0, BOARD_GPIO_ID_BUTTON1}

// Button GPIO polarity
#define BOARD_BUTTON_ACTIVE_LOW         true

// Button GPIO internal pull up/down
#define BOARD_BUTTON_INTERNAL_PULL      false

#endif // BOARD_SILABS_BRD2703A_BOARD_H_
