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
 * @brief   Interrupt pin for dual mcu app, unread indication
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
 * If board contains LEDs, they are defined here. If not defined, a dummy
 * LED driver is compiled, so that applications can use the LED driver
 * unconditionally.
 *
 * For Silabs EFR32 family, the list contains GPIO port/pin number pairs.
 *
 * @note in order for application to use LEDs, see @ref source_makefile_hal_led
 * "here".
 */
#define BOARD_LED_PIN_LIST              {{GPIOF, 4}, {GPIOF, 5}}

/**
 * @brief   LED GPIO polarity
 *
 * If LEDs turn on when the GPIO pin is driven low, this setting is true. Many
 * EFR32 boards, such as the Thunderboard Sense 2 and the BRD4001 Evaluation
 * Board have active high LEDs, so this setting should remain false.
 */
#define BOARD_LED_ACTIVE_LOW            false

/**
 * @brief   Button definitions
 *
 * Any buttons present on the board are defined here. If not defined, a dummy
 * button driver is compiled, so that applications can use the button driver
 * unconditionally.
 *
 * For Silabs EFR32 family, the list contains GPIO external interrupt numbers,
 * GPIO ports and pins. See \ref BOARD_BUTTON_USE_EVEN_INT below for extra
 * considerations when selecting external interrupt numbers.
 *
 * @note in order for application to use buttons, see @ref
 * source_makefile_hal_button "here".
 */
#define BOARD_BUTTON_PIN_LIST           {{4, GPIOF, 6}, {6, GPIOF, 7}}

/**
 * @brief   Button GPIO polarity
 *
 * If a button press pulls the GPIO pin low, this setting is true. This is the
 * case for many EFR32 boards, such as the Thunderboard Sense 2 and the BRD4001
 * Evaluation Board. Otherwise, if a button press pulls the GPIO pin high, this
 * setting should be set to false.
 */
#define BOARD_BUTTON_ACTIVE_LOW         true

/**
 * @brief   Button GPIO internal pull up/down
 *
 * Some buttons don't have any pull-up or pull-down resistor installed on the
 * board. They need it to be setup in software. Set
 * \ref BOARD_BUTTON_INTERNAL_PULL to true to enable internal pull-up(down).
 * Pull-up(down) is enabled when \ref BOARD_BUTTON_ACTIVE_LOW is true(false).
 */
#define BOARD_BUTTON_INTERNAL_PULL true

/**
 * @brief   Button GPIO interrupt even/odd selection
 *
 * The EFR32 GPIO block has 16 configurable external interrupt sources. Even
 * and odd numbered interrupt sources are routed to separate interrupt vectors
 * in the processor. If this setting is true, the button interrupts use the
 * even interrupt vector GPIO_EVEN_IRQn, otherwise GPIO_ODD_IRQn.
 *
 * Not all GPIO pins can be mapped to all even or odd external interrupt
 * sources. Please see the GPIO_EXTIPINSELL and GPIO_EXTIPINSELH register
 * documentation in the EFR32xG12 Wireless Gecko Reference Manual.
 *
 * The external interrupt source in \ref BOARD_BUTTON_PIN_LIST above should
 * match this definition, otherwise the buttons won't work.
 */
#define BOARD_BUTTON_USE_EVEN_INT       true

#endif /* _BOARD_EFR32_TEMPLATE_BOARD_H_ */
