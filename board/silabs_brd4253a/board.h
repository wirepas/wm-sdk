/* Copyright 2018 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/**
 * @file
 *
 * Board definition for a board composed of a
 * <a href="https://www.silabs.com/products/development-tools/wireless/proprietary/slwstk6005a-sub-ghz-bluetooth-multiband-wireless-starter-kit">Silabs starter kit</a>
 * and a <a href="https://www.silabs.com/documents/public/reference-manuals/brd4253a-rm.pdf">brd4253a radio module</a>
 */
#ifndef BOARD_SILABSEFR32KIT_BOARD_H_
#define BOARD_SILABSEFR32KIT_BOARD_H_

// NOTE! The VCOM on the kit board supports ONLY the standart baud rates
// e.g. 125000 is not working.

// Serial port
#define BOARD_USART_ID                  0
#define BOARD_USART_TX_PORT             GPIOA
#define BOARD_USART_TX_PIN              0
#define BOARD_USART_RX_PORT             GPIOA
#define BOARD_USART_RX_PIN              1

// ROUTELOC are dependent on GPIO defined above and mapping
// can be founded chip datasheet from Silabs
#define BOARD_USART_ROUTELOC_RXLOC      USART_ROUTELOC0_RXLOC_LOC0
#define BOARD_USART_ROUTELOC_TXLOC      USART_ROUTELOC0_TXLOC_LOC0

// For further information about Silicon Labs Kit board pin configuration see:
// UG265: EFR32FG12 2400/868 MHz 10 dBm Wireless Starter Kit User's Guide

// List of GPIO pins
#define BOARD_GPIO_PIN_LIST            {{GPIOF, 4}, /* PF04 */\
                                        {GPIOF, 5}, /* PF05 */\
                                        {GPIOF, 6}, /* PF06 */\
                                        {GPIOF, 7}, /* PF07 */\
                                        {GPIOA, 1}, /* PA01. required by the dual_mcu app. usart wakeup pin (= BOARD_USART_RX) */\
                                        {GPIOD, 8}, /* PD08. required by the dual_mcu app (indication signal) */\
                                        {GPIOA, 5}} /* PA05. usart vcom pin */

// User friendly name for GPIOs (IDs mapped to the BOARD_GPIO_PIN_LIST table)
#define BOARD_GPIO_ID_LED0              0 // mapped to pin PF04
#define BOARD_GPIO_ID_LED1              1 // mapped to pin PF05
#define BOARD_GPIO_ID_BUTTON0           2 // mapped to pin PF06
#define BOARD_GPIO_ID_BUTTON1           3 // mapped to pin PF07
#define BOARD_GPIO_ID_USART_WAKEUP      4 // mapped to pin PA01
#define BOARD_GPIO_ID_UART_IRQ          5 // mapped to pin PD08
#define BOARD_GPIO_ID_VCOM_ENABLE       6 // mapped to pin PA05

// List of LED IDs
#define BOARD_LED_ID_LIST              {BOARD_GPIO_ID_LED0, BOARD_GPIO_ID_LED1}

// Active high polarity for LEDs
#define BOARD_LED_ACTIVE_LOW false

// List of button IDs
#define BOARD_BUTTON_ID_LIST           {BOARD_GPIO_ID_BUTTON0, BOARD_GPIO_ID_BUTTON1}

// Active low polarity for buttons
#define BOARD_BUTTON_ACTIVE_LOW true

// Board has external pull-up for buttons
#define BOARD_BUTTON_INTERNAL_PULL false


#endif /* BOARD_SILABSEFR32KIT_BOARD_H_ */
