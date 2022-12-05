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

/**
 * @brief   GPIO definitions
 *
 * If board needs GPIOs, they are defined here. If not defined, a dummy
 * GPIO driver is compiled, so that applications can use the GPIO driver
 * unconditionally.
 *
 * For Nordic nRF52 family, GPIOs are defined as list of GPIO pin numbers
 *
 * @note in order for application to use GPIOs, see @ref source_makefile_hal_gpio
 * "here".
 */
#define BOARD_GPIO_PIN_LIST            {17, /* P0.17 */\
                                        18, /* P0.18 */\
                                        19, /* P0.19 */\
                                        20, /* P0.20 */\
                                        13, /* P0.13 */\
                                        14, /* P0.14 */\
                                        15, /* P0.15 */\
                                        16, /* P0.16 */\
                                        6,  /* P0.06. usart tx pin  */\
                                        8,  /* P0.08. usart rx pin  */\
                                        7,  /* P0.07. usart cts pin  */\
                                        5,  /* P0.05. usart rts pin */\
                                        11} /* P0.11. required by the dual_mcu app (indication signal) */

/**
 * @brief   GPIO IDs
 *
 * User friendly name for GPIOs (IDs mapped to the BOARD_GPIO_PIN_LIST table)
 *
 */
#define BOARD_GPIO_ID_LED0              0  // mapped to pin P0.17
#define BOARD_GPIO_ID_LED1              1  // mapped to pin P0.18
#define BOARD_GPIO_ID_LED2              2  // mapped to pin P0.19
#define BOARD_GPIO_ID_LED3              3  // mapped to pin P0.20
#define BOARD_GPIO_ID_BUTTON0           4  // mapped to pin P0.13
#define BOARD_GPIO_ID_BUTTON1           5  // mapped to pin P0.14
#define BOARD_GPIO_ID_BUTTON2           6  // mapped to pin P0.15
#define BOARD_GPIO_ID_BUTTON3           7  // mapped to pin P0.16
#define BOARD_GPIO_ID_USART_TX          8  // mapped to P0.06
#define BOARD_GPIO_ID_USART_RX          9  // mapped to P0.08
#define BOARD_GPIO_ID_USART_CTS         10 // mapped to P0.07
#define BOARD_GPIO_ID_USART_RTS         11 // mapped to P0.05
#define BOARD_GPIO_ID_UART_IRQ          12 // mapped to P0.11
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
#define BOARD_LED_ID_LIST               {BOARD_GPIO_ID_LED0, BOARD_GPIO_ID_LED1, BOARD_GPIO_ID_LED2, BOARD_GPIO_ID_LED3}

/**
 * @brief   LED GPIO polarity
 *
 * If LEDs turn on when the GPIO pin is driven low, this setting is true. This
 * is the case for many nRF52 boards, such as the PCA10040 and PCA10056.
 * Otherwise, if a LED is lit when the the GPIO pin is driven high, this
 * setting should be set to false.
 */
#define BOARD_LED_ACTIVE_LOW            true

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
#define BOARD_BUTTON_ID_LIST           {BOARD_GPIO_ID_BUTTON0, BOARD_GPIO_ID_BUTTON1, BOARD_GPIO_ID_BUTTON2, BOARD_GPIO_ID_BUTTON3}

/**
 * @brief   Button GPIO polarity
 *
 * If a button press pulls the GPIO pin low, this setting is true. This is the
 * case for many nRF52 boards, such as the PCA10040 and PCA10056. Otherwise, if
 * a button press pulls the GPIO pin high, this setting should be set to false.
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

#endif /* _BOARD_NRF52_TEMPLATE_BOARD_H_ */
