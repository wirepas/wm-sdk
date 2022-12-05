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

// List of GPIO pins
#define BOARD_GPIO_PIN_LIST            {{GPIO_PORTB, 2}, /* PB02 */\
                                        {GPIO_PORTD, 3}, /* PD03 */\
                                        {GPIO_PORTB, 1}, /* PB01 */\
                                        {GPIO_PORTB, 3}, /* PB03 */\
                                        {GPIO_PORTA, 8}, /* PA08. usart tx pin */\
                                        {GPIO_PORTA, 9}, /* PA09. usart rx pin */\
                                        {GPIO_PORTB, 0}} /* PB00. usart vcom pin */

// User friendly name for GPIOs (IDs mapped to the BOARD_GPIO_PIN_LIST table)
#define BOARD_GPIO_ID_LED0              0  // mapped to pin PB02
#define BOARD_GPIO_ID_LED1              1  // mapped to pin PD03
#define BOARD_GPIO_ID_BUTTON0           2  // mapped to pin PB01
#define BOARD_GPIO_ID_BUTTON1           3  // mapped to pin PB03
#define BOARD_GPIO_ID_USART_TX          4  // mapped to pin PA08
#define BOARD_GPIO_ID_USART_RX          5  // mapped to pin PA09
#define BOARD_GPIO_ID_USART_VCOM        6  // mapped to pin PB00

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
// Used by the bootloader extension
#define BOARD_BOOTLOADER_SPI                        USART0
#define BOARD_BOOTLOADER_SPIROUTE                   GPIO->USARTROUTE[0]

#define BOARD_BOOTLOADER_SPI_EXTFLASH_MOSI_PORT     GPIO_PORTC
#define BOARD_BOOTLOADER_SPI_EXTFLASH_MISO_PORT     GPIO_PORTC
#define BOARD_BOOTLOADER_SPI_EXTFLASH_SCKL_PORT     GPIO_PORTC
#define BOARD_BOOTLOADER_SPI_EXTFLASH_CS_PORT       GPIO_PORTC
#define BOARD_BOOTLOADER_SPI_EXTFLASH_MOSI_PIN      1
#define BOARD_BOOTLOADER_SPI_EXTFLASH_MISO_PIN      2
#define BOARD_BOOTLOADER_SPI_EXTFLASH_SCKL_PIN      3
#define BOARD_BOOTLOADER_SPI_EXTFLASH_CS_PIN        4

#endif /* BOARD_WIREPAS_BRD4210A_BOARD_H_ */
