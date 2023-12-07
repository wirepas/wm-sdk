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
#ifndef BOARD_SILABS_BRD4210A_BOARD_H_
#define BOARD_SILABS_BRD4210A_BOARD_H_

// Serial port
#define BOARD_USART_ID                  0
#define BOARD_USART_TX_PORT             GPIO_PORTA
#define BOARD_USART_TX_PIN              8
#define BOARD_USART_RX_PORT             GPIO_PORTA
#define BOARD_USART_RX_PIN              9

// List of GPIO pins
#define BOARD_GPIO_PIN_LIST            {{GPIO_PORTB, 2}, /* PB02 */\
                                        {GPIO_PORTD, 3}, /* PD03 */\
                                        {GPIO_PORTB, 1}, /* PB01 */\
                                        {GPIO_PORTB, 3}, /* PB03 */\
                                        {GPIO_PORTA, 9}, /* PA09. required by the dual_mcu app. usart wakeup pin (= BOARD_USART_RX) */\
                                        {GPIO_PORTB, 0}, /* PB00. usart vcom pin */\
                                        {GPIO_PORTC, 4}} /* PC04 SPI CS */

// User friendly name for GPIOs (IDs mapped to the BOARD_GPIO_PIN_LIST table)
#define BOARD_GPIO_ID_LED0              0  // mapped to pin PB02
#define BOARD_GPIO_ID_LED1              1  // mapped to pin PD03
#define BOARD_GPIO_ID_BUTTON0           2  // mapped to pin PB01
#define BOARD_GPIO_ID_BUTTON1           3  // mapped to pin PB03
#define BOARD_GPIO_ID_USART_WAKEUP      4  // mapped to pin PA09
#define BOARD_GPIO_ID_VCOM_ENABLE       5  // mapped to pin PB00
#define BOARD_GPIO_ID_SPI_CS            6  // mapped to pin PC04

// List of LED IDs
#define BOARD_LED_ID_LIST              {BOARD_GPIO_ID_LED0, BOARD_GPIO_ID_LED1}

// Active high polarity for LEDs
#define BOARD_LED_ACTIVE_LOW            false

// List of button IDs
#define BOARD_BUTTON_ID_LIST           {BOARD_GPIO_ID_BUTTON0, BOARD_GPIO_ID_BUTTON1}

// Active low polarity for buttons
#define BOARD_BUTTON_ACTIVE_LOW         true

// Board has external pull-up for buttons
#define BOARD_BUTTON_INTERNAL_PULL      false

// SPI + External Flash Memory
// Used by the SPI driver
#define BOARD_SPI                       USART0
#define BOARD_SPIROUTE                  GPIO->USARTROUTE[0]

#define BOARD_SPI_EXTFLASH_MOSI_PORT    GPIO_PORTC
#define BOARD_SPI_EXTFLASH_MISO_PORT    GPIO_PORTC
#define BOARD_SPI_EXTFLASH_SCKL_PORT    GPIO_PORTC
#define BOARD_SPI_EXTFLASH_MOSI_PIN     1
#define BOARD_SPI_EXTFLASH_MISO_PIN     2
#define BOARD_SPI_EXTFLASH_SCKL_PIN     3

#endif /* BOARD_SILABS_BRD4210A_BOARD_H_ */
