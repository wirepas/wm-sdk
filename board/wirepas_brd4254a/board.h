/* Copyright 2018 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/**
 * @file
 *
 * Board definition for a board composed of a Wirepas Evaluation Kit mother board
 * and a silabs <a href="https://www.silabs.com/documents/public/reference-manuals/brd4254a-rm.pdf">brd4254a radio module</a>
 */
#ifndef BOARD_WIREPASEFR32KIT_BOARD_H_
#define BOARD_WIREPASEFR32KIT_BOARD_H_

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

// List of GPIO ports and pins for the LEDs on the board: LED1, LED2
#define BOARD_LED_PIN_LIST {{GPIOF, 4}, {GPIOF, 5}}

// Active high polarity for LEDs
#define BOARD_LED_ACTIVE_LOW false

/**
 *    Connector  +  Pin   +   EFR32_port_pin
 *    -----------+--------+--------------------------
 *       X12     |   1    |     PB13
 *       X12     |   2    |     (boden)
 *       X12     |   3    |     (PA0 / USART TX)
 *       X12     |   4    |     PD12
 *       X12     |   5    |     PC9
 *       X12     |   6    |     PF9
 *       X12     |   7    |     PF2
 *       X12     |   8    |     PC5
 *       X12     |   9    |     (n.c.)
 *       X12     |   10   |     PB10
 *       X12     |   11   |     (n.c.)
 *       X12     |   12   |     PD14
 *       X12     |   13   |     PD15
 *       X12     |   14   |     (GND)
 *
 *    Connector  +  Pin   +   EFR32_port_pin
 *    -----------+--------+--------------------------
 *       X11     |   1    |
 *       X11     |   2    |
 *       X11     |   3    |     PA7
 *       X11     |   4    |     PA6
 *       X11     |   5    |     PA8
 *       X11     |   6    |     PB6
 *       X11     |   7    |     (GND)
 *       X11     |   8    |     PA9
 *
 *    Connector  +  Pin   +   EFR32_port_pin
 *    -----------+--------+--------------------------
 *       X10     |   1    |     (GND)
 *       X10     |   2    |     PA3
 *       X10     |   3    |
 *       X10     |   4    |     (PA1 / USART RX)
 *       X10     |   5    |     (PA0 / USART TX)
 *       X10     |   6    |     PA2
 *
 *       Miscellaneous:
 *       LED0 = PF4
 *       LED1 = PF5
 *       KIT_DETECT = PD8
 *       IF_CONFIG0 = PD10
 *       IF_CONFIG1 = PD11
 *
 */


#endif /* BOARD_WIREPASEFR32KIT_BOARD_H_ */
