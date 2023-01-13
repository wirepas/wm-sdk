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
 * @brief   Transmission pin port
 */
#define BOARD_USART_TX_PORT              GPIOA
/**
 * @brief   Transmission pin number
 */
#define BOARD_USART_TX_PIN              0
/**
 * @brief   Reception pin port
 */
#define BOARD_USART_RX_PORT              GPIOA
/**
 * @brief Reception pin number
 */
#define BOARD_USART_RX_PIN              1

/**
 * @brief   GPIO definitions
 *
 * If board needs GPIOs, they are defined here. If not defined, a dummy
 * GPIO driver is compiled, so that applications can use the GPIO driver
 * unconditionally.
 *
 * For Silabs EFR32 family, the list contains GPIO port/pin number pairs.
 *
 * @note in order for application to use GPIOs, see @ref source_makefile_hal_gpio
 * "here".
 */
#define BOARD_GPIO_PIN_LIST            {{GPIOF, 4}, /* PF04 */\
                                        {GPIOF, 5}, /* PF05 */\
                                        {GPIOF, 6}, /* PF06 */\
                                        {GPIOF, 7}, /* PF07 */\
                                        {GPIOA, 1}, /* PA01. required by the dual_mcu app. usart wakeup pin (= BOARD_USART_RX) */\
                                        {GPIOD, 8}} /* PD08. required by the dual_mcu app (indication signal) */

/**
 * @brief   GPIO IDs
 *
 * User friendly name for GPIOs (IDs mapped to the BOARD_GPIO_PIN_LIST table)
 *
 */
#define BOARD_GPIO_ID_LED0              0  // mapped to pin PF04
#define BOARD_GPIO_ID_LED1              1  // mapped to pin PF05
#define BOARD_GPIO_ID_BUTTON0           2  // mapped to pin PF06
#define BOARD_GPIO_ID_BUTTON1           3  // mapped to pin PF07

/**
 * @brief    pin for dual mcu app, usart wakeup
 *
 * This only used in @ref source/dualmcu_app.c "dualmcu_app" application to
 * wake up the usart driver when detecting a transition on the usart RX pin.
 *
 * It is optional definition.
 */
#define BOARD_GPIO_ID_USART_WAKEUP          4  // mapped to pin PA01

/**
 * @brief   Interrupt pin for dual mcu app, unread indication
 *
 * This only used in @ref source/dualmcu_app.c "dualmcu_app" application to
 * announce with GPIO pin that there is incoming indication to be read from
 * device.
 *
 * It is optional definition.
 */
#define BOARD_GPIO_ID_UART_IRQ           5  // mapped to pin PD08

/**
 * @brief   LED definitions
 *
 * If board contains LEDs, The LED IDs list is defined here. The LED IDs are mapped to GPIO IDs.
 * If not defined, a dummy LED driver is compiled, so that applications can use the LED driver
 * unconditionally.
 *
 * @note in order for application to use LEDs, see @ref source_makefile_hal_led
 * "here".
 */
#define BOARD_LED_PIN_LIST              {BOARD_GPIO_ID_LED0, BOARD_GPIO_ID_LED1}

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
 * If board contains buttons, The button IDs list is defined here. The button IDs are mapped to GPIO IDs.
 * If not defined, a dummy button driver is compiled, so that applications can use the button driver
 * unconditionally.
 *
 * @note in order for application to use buttons, see @ref
 * source_makefile_hal_button "here".
 */
#define BOARD_BUTTON_ID_LIST            {BOARD_GPIO_ID_BUTTON0, BOARD_GPIO_ID_BUTTON1}

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
#define BOARD_BUTTON_INTERNAL_PULL      true

#endif /* _BOARD_EFR32_TEMPLATE_BOARD_H_ */
