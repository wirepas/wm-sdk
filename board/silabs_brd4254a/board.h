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
 * and a <a href="https://www.silabs.com/documents/public/reference-manuals/brd4254a-rm.pdf">brd4254a radio module</a>
 */
#ifndef BOARD_SILABSEFR32KIT_BOARD_H_
#define BOARD_SILABSEFR32KIT_BOARD_H_

// NOTE! The VCOM on the kit board supports ONLY the standart baud rates
// e.g. 125000 is not working.

// Waps usart defines
#define BOARD_USART_ID                  0

#define BOARD_USART_GPIO_PORT           GPIOA
#define BOARD_USART_TX_PIN              0
#define BOARD_USART_RX_PIN              1
// ROUTELOC are dependent on GPIO defined above and mapping
// can be founded chip datasheet from Silabs
#define BOARD_USART_ROUTELOC_RXLOC      USART_ROUTELOC0_RXLOC_LOC0
#define BOARD_USART_ROUTELOC_TXLOC      USART_ROUTELOC0_TXLOC_LOC0

// Interrupt pin for dual mcu app, unread indication
#define BOARD_UART_INT_PIN              8
#define BOARD_UART_INT_PORT             GPIOD

// Enadle vcom in silabs kit board
#define BOARD_USART_VCOM_PIN            5

// NOTE! To enable virtual com port (VCOM): When the target device drives the
// VCOM_ENABLE (PA5) signal high, a communication line to the Board Controller
// is enabled. Add the following code snippet to the
// mcu/efr32xg12/hal/usart.c in function Usart_init starting
// from Enable vcom comment:

////    /* Enable clocks */
//    CMU->HFBUSCLKEN0 |= CMU_HFBUSCLKEN0_GPIO;
//
//    // Enable vcom
//    hal_gpio_set_mode(BOARD_USART_GPIO_PORT,
//                      BOARD_USART_VCOM_PIN,
//                      GPIO_MODE_DISABLED);
//    hal_gpio_clear(BOARD_USART_GPIO_PORT,
//                   BOARD_USART_VCOM_PIN);
//    hal_gpio_set_mode(BOARD_USART_GPIO_PORT,
//                      BOARD_USART_VCOM_PIN,
//                      GPIO_MODE_OUT_PP);
//    hal_gpio_set(BOARD_USART_GPIO_PORT,
//                 BOARD_USART_VCOM_PIN);

// For further information about Silicon Labs Kit board pin configuration see:
// UG265: EFR32FG12 2400/868 MHz 10 dBm Wireless Starter Kit User's Guide

// List of GPIO ports and pins for the LEDs on the board: LED0, LED1
#define BOARD_LED_PIN_LIST {{GPIOF, 4}, {GPIOF, 5}}

// Active high polarity for LEDs
#define BOARD_LED_ACTIVE_LOW false

// List of ext. ints, GPIO ports and pins for buttons on the board: PB0, PB1
#define BOARD_BUTTON_PIN_LIST {{4, GPIOF, 6}, {6, GPIOF, 7}}

// Active low polarity for buttons
#define BOARD_BUTTON_ACTIVE_LOW true

// Buttons use even external interrupts
#define BOARD_BUTTON_USE_EVEN_INT true


#endif /* BOARD_SILABSEFR32KIT_BOARD_H_ */
