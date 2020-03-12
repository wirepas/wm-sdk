/* Copyright 2018 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/**
 * @file
 *
 * Template to be used for EFR32 board definitions
 */

#ifndef _BOARD_EFR32_TEMPLATE_BOARD_H_
#define _BOARD_EFR32_TEMPLATE_BOARD_H_

/**
 * @brief   USART ID
 *
 * Valid values: 0, 1, ... (to match <code>BOARD_USART</code>)
 *
 * @note in order for application to use USART, see @ref
 * source_makefile_hal_uart "here".
 *
 */
#define BOARD_USART_ID                  0

/**
 * @brief   GPIO port used
 *
 * Valid values: <code>GPIOA, GPIOB, ...</code>
 */
#define BOARD_USART_GPIO_PORT           GPIOA
/**
 * @brief   RX routeloc definition
 *
 * Valid values: <code>USART_ROUTELOC0_RXLOC_LOC0, USART_ROUTELOC0_RXLOC_LOC1,
 * ...</code>
 */
#define BOARD_USART_ROUTELOC_RXLOC      USART_ROUTELOC0_RXLOC_LOC0
/**
 * @brief   TX routeloc definition
 *
 * Valid values: <code>USART_ROUTELOC0_TXLOC_LOC0, USART_ROUTELOC0_TXLOC_LOC1,
 * ...</code>
 */
#define BOARD_USART_ROUTELOC_TXLOC      USART_ROUTELOC0_TXLOC_LOC0

/**
 * @brief   Transmission pin number
 */
#define BOARD_USART_TX_PIN              0
/**
 * @brief Reception pin number
 */
#define BOARD_USART_RX_PIN              1

/**
 * @brief Interrupt pin for dual mcu app, unread indication
 *
 * This only used in @ref source/dualmcu_app.c "dualmcu_app" application to
 * announce with GPIO pin that there is incoming indication to be read from
 * device.
 *
 * It is optional definition. If not present, no irq pin is present
 */
#define BOARD_UART_INT_PIN              8
#define BOARD_UART_INT_PORT             GPIOD

/**
 * @brief   LED definitions
 *
 * If board contains LEDs, they are defined here. When defined, application
 * using LEDs may use the board.
 *
 * For Silabs EFR32 family, the list contains GPIO port/pin number pairs.
 *
 * @note in order for application to use LEDs, see @ref source_makefile_hal_led
 * "here".
 */
#define BOARD_LED_PIN_LIST {{GPIOF, 4}, {GPIOF, 5}}

#endif /* _BOARD_EFR32_TEMPLATE_BOARD_H_ */
