/* Copyright 2017 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/**
 * @file
 *
 * Template to be used for nRF52 board definitions
 */

#ifndef _BOARD_NRF52_TEMPLATE_BOARD_H_
#define _BOARD_NRF52_TEMPLATE_BOARD_H_

// Serial port pins
/**
 * @brief   Transmission pin number
 *
 * @note in order for application to use USART, see @ref
 * source_makefile_hal_uart "here".
 */
#define BOARD_USART_TX_PIN              6
/**
 * @brief Reception pin number
 */
#define BOARD_USART_RX_PIN              8
/**
 * @brief CTS pin number, for @ref USE_USART_HW_FLOW_CONTROL
 */
#define BOARD_USART_CTS_PIN             7
/**
 * @brief RTS pin number, for @ref USE_USART_HW_FLOW_CONTROL
 */
#define BOARD_USART_RTS_PIN             5

/**
 * @brief Interrupt pin for dual mcu app, unread indication
 *
 * This only used in @ref source/dualmcu_app.c "dualmcu_app" application to
 * announce with GPIO pin that there is incoming indication to be read from
 * device.
 *
 * It is optional definition. If not present, no irq pin is present
 */
#define BOARD_UART_IRQ_PIN              11

/**
 * @brief   LED definitions
 *
 * If board contains LEDs, they are defined here. When defined, application
 * using LEDs may use the board.
 *
 * For Nordic nRF52 family, LEDs are defined as list of GPIO pin numbers
 *
 * @note in order for application to use LEDs, see @ref source_makefile_hal_led
 * "here".
 */
#define BOARD_LED_PIN_LIST {17, 18, 19,20}

/**
 * @brief Button definitions
 *
 * When defined, application using buttons may use the board.
 * For Nordic nRF52 family, buttons are defined simply by the GPIO numbers.
 *
 * @note in order for application to use buttons, see @ref
 * source_makefile_hal_button "here".
 */
#define BOARD_BUTTON_PIN_LIST {13, 14, 15, 16}
/**
 * @brief Button polarity
 */
#define BOARD_BUTTON_ACTIVE_LOW true

/**
 * @brief This configuration tells that board contains DCDC converter to be
 *        used.
 */
#define BOARD_SUPPORT_DCDC

#endif /* _BOARD_NRF52_TEMPLATE_BOARD_H_ */
