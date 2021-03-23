/* Copyright 2020 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/**
 * @file
 *
 * Board definition for a board composed of a Wirepas Evaluation Kit mother board
 * and a silabs <a href="https://www.silabs.com/documents/public/reference-manuals/brd4183a-rm.pdf">brd4183a radio module</a>
 */
#ifndef BOARD_WIREPASEFR32KIT_BOARD_H_
#define BOARD_WIREPASEFR32KIT_BOARD_H_

// Waps usart defines
#define BOARD_USART_ID                  0

#define BOARD_USART_GPIO_PORT           GPIO_PORTA
#define BOARD_USART_TX_PIN              5
#define BOARD_USART_RX_PIN              6

// List of GPIO ports and pins for the LEDs on the board: LED1 (LED2 not connected)
#define BOARD_LED_PIN_LIST {{GPIO_PORTB, 1}}

// Active high polarity for LEDs
#define BOARD_LED_ACTIVE_LOW false

// INTERNAL_USE_ONLY_BEGIN
/**
 *    Connector  +  Pin   +   EFR32_port_pin
 *    -----------+--------+--------------------------
 *       X12     |   1    |     PC05
 *       X12     |   2    |     (n.c.)
 *       X12     |   3    |     (PA05 / USART TX)
 *       X12     |   4    |     (n.c.)
 *       X12     |   5    |     (n.c.)
 *       X12     |   6    |     (n.c.)
 *       X12     |   7    |     PA03
 *       X12     |   8    |     (n.c.)
 *       X12     |   9    |     (PA02 / SWDIO)
 *       X12     |   10   |     (n.c.)
 *       X12     |   11   |     (n.c.)
 *       X12     |   12   |     (n.c.)
 *       X12     |   13   |     (n.c.)
 *       X12     |   14   |     (GND)
 *
 *    Connector  +  Pin   +   EFR32_port_pin
 *    -----------+--------+--------------------------
 *       X11     |   1    |     (3v3)
 *       X11     |   2    |     (PA06 / USART RX)
 *       X11     |   3    |     (n.c.)
 *       X11     |   4    |     (n.c.)
 *       X11     |   5    |     (n.c.)
 *       X11     |   6    |     (PA05 / USART TX)
 *       X11     |   7    |     (GND)
 *       X11     |   8    |     (n.c.)
 *
 *    Connector  +  Pin   +   EFR32_port_pin
 *    -----------+--------+--------------------------
 *       X10     |   1    |     (GND)
 *       X10     |   2    |     (n.c.)
 *       X10     |   3    |
 *       X10     |   4    |     (PA06 / USART RX)
 *       X10     |   5    |     (PA05 / USART TX)
 *       X10     |   6    |     PB02
 *
 *    Connector  +  Pin   +   EFR32_port_pin
 *    -----------+--------+--------------------------
 *       X4      |   1    |     PB0 / IF_CONFIG0
 *       X4      |   2    |     (GND)
 *
 *    Connector  +  Pin   +   EFR32_port_pin
 *    -----------+--------+--------------------------
 *       X6      |   1    |     PB01 / IF_CONFIG1
 *       X6      |   2    |     (GND)
 *
 *       Miscellaneous:
 *       LED1 = (n.c.)
 *       KIT_DETECT = (n.c.)
 *
 */

/** Only limited number of I/O available for debug purposes in this board.
 *  Select for what purpose you like to use them, here is default set. */
#define HAL_DBG_MCU_PORT                GPIOC
#define HAL_DBG_MCU_PIN                 5

#define HAL_DBG_RADIO_TX_PORT           GPIOA
#define HAL_DBG_RADIO_TX_PIN            3

#define HAL_DBG_RADIO_CRC_PORT          GPIOB
#define HAL_DBG_RADIO_CRC_PIN           2

#define HAL_DBG_IRQ_RADIO_PORT          GPIOB
#define HAL_DBG_IRQ_RADIO_PIN           0

/** Also these debug I/O macros are embedded into the code.
 *  Define pin and port and uncomment the line to take it in use. */
//#define HAL_DBG_IRQ_RTC_PORT
//#define HAL_DBG_IRQ_RTC_PIN

//#define HAL_DBG_IRQ_USART_PORT
//#define HAL_DBG_IRQ_USART_PIN

//#define HAL_DBG_TASK_MAC_PORT
//#define HAL_DBG_TASK_MAC_PIN

//#define HAL_DBG_TASK_MGMT_PORT
//#define HAL_DBG_TASK_MGMT_PIN

//#define HAL_DBG_TASK_ROUTE_PORT
//#define HAL_DBG_TASK_ROUTE_PIN

//#define HAL_DBG_TASK_WAPS_PORT
//#define HAL_DBG_TASK_WAPS_PIN
// INTERNAL_USE_ONLY_END
#endif /* BOARD_WIREPASEFR32KIT_BOARD_H_ */
