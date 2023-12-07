/* Copyright 2023 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/**
 * @file
 *
 * Board definition for the Radietum reference board (v1.0)
 * It has a MX25R8035F external flash
 */
#ifndef BOARD_RADIENTUM_WP_V1_0_BOARD_H_
#define BOARD_RADIENTUM_WP_V1_0_BOARD_H_

// Serial port
#define BOARD_USART_ID                  1
#define BOARD_USART_TX_PORT             GPIO_PORTA
#define BOARD_USART_TX_PIN              5
#define BOARD_USART_RX_PORT             GPIO_PORTA //Use I/O3 from board instead of UART1_RX
#define BOARD_USART_RX_PIN              6

// List of GPIO pins
#define BOARD_GPIO_PIN_LIST            {{GPIO_PORTA, 7}, /* PA07 */\
                                        {GPIO_PORTA, 8}, /* PA08 */\
                                        {GPIO_PORTA, 0}} /* PA09. required by the dual_mcu app. usart wakeup pin (= BOARD_USART_RX) */

// User friendly name for GPIOs (IDs mapped to the BOARD_GPIO_PIN_LIST table)
#define BOARD_GPIO_ID_LED0              0  // mapped to pin PA07
#define BOARD_GPIO_ID_LED1              1  // mapped to pin PA08
#define BOARD_GPIO_ID_USART_WAKEUP      2  // mapped to pin PA09

// List of LED IDs mapped to GPIO IDs
#define BOARD_LED_ID_LIST              {BOARD_GPIO_ID_LED0, BOARD_GPIO_ID_LED1}

// Active high polarity for LEDs
#define BOARD_LED_ACTIVE_LOW            true

// External Flash Memory
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


#endif /* BOARD_RADIENTUM_WP_V1_0_BOARD_H_ */
