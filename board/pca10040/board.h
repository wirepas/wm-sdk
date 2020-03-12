/* Copyright 2017 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/**
 * @file
 *
 * Board definition for the
 * <a href="https://www.nordicsemi.com/Software-and-Tools/Development-Kits/nRF52-DK">Nordic semiconductor PCA10040 evaluation board</a>
 */
#ifndef BOARD_PCA10040_BOARD_H_
#define BOARD_PCA10040_BOARD_H_

// Interrupt pin for dual mcu app, unread indication
#define BOARD_UART_IRQ_PIN              11

// Serial port pins
#define BOARD_USART_TX_PIN              6
#define BOARD_USART_RX_PIN              8
#define BOARD_USART_CTS_PIN             7  /* For USE_USART_HW_FLOW_CONTROL */
#define BOARD_USART_RTS_PIN             5  /* For USE_USART_HW_FLOW_CONTROL */

// Pwm output for pwm_driver app
#define BOARD_PWM_OUTPUT_GPIO           28

// List of GPIOs for the leds on the board (LED1 to LED4)
#define BOARD_LED_PIN_LIST {17, 18, 19,20}

// List of GPIOs for the Buttons on the board
#define BOARD_BUTTON_PIN_LIST {13, 14, 15, 16}
#define BOARD_BUTTON_ACTIVE_LOW true

// The board supports DCDC
#define BOARD_SUPPORT_DCDC


#endif /* BOARD_PCA10040_BOARD_H_ */
