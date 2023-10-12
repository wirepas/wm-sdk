/* Copyright 2023 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/**
 * @file
 *
 * Board definition for a board composed of a Wirepas Evaluation Kit mother board
 * and a silabs <a href="https://www.silabs.com/documents/public/schematic-files/BRD4187C-A01-schematic.pdf">brd4187c radio module schematic</a>
 */
#ifndef BOARD_SILABS_BRD4187C_BOARD_H_
#define BOARD_SILABS_BRD4187C_BOARD_H_

// Serial port
#define BOARD_USART_ID                  0
#define BOARD_USART_TX_PORT             GPIO_PORTA
#define BOARD_USART_TX_PIN              8
#define BOARD_USART_RX_PORT             GPIO_PORTA
#define BOARD_USART_RX_PIN              9

// List of GPIO pins
#define BOARD_GPIO_PIN_LIST             {{GPIO_PORTB, 2}, /* PB02 EZR_LED0 */ \
                                         {GPIO_PORTB, 4}, /* PB04 EZR_LED1*/ \
                                         {GPIO_PORTA, 9}, /* PA09. required by the dual_mcu app. usart wakeup pin (= BOARD_USART_RX) */ \
                                         {GPIO_PORTB, 0}} /* PB00. usart vcom pin */

// User friendly name for GPIOs (IDs mapped to the BOARD_GPIO_PIN_LIST table)
#define BOARD_GPIO_ID_LED0              0  // mapped to pin PB02
#define BOARD_GPIO_ID_LED1              1  // mapped to pin PB04
#define BOARD_GPIO_ID_USART_WAKEUP      2  // mapped to pin PA09
#define BOARD_GPIO_ID_VCOM_ENABLE       3  // mapped to pin PB00

// List of LED IDs
#define BOARD_LED_ID_LIST              {BOARD_GPIO_ID_LED0, BOARD_GPIO_ID_LED1}

// Active high polarity for LEDs
#define BOARD_LED_ACTIVE_LOW            false

#endif /* BOARD_SILABS_BRD4187C_BOARD_H_ */
