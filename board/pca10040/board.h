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

// Serial port pins
#define BOARD_USART_TX_PIN              6
#define BOARD_USART_RX_PIN              8
#define BOARD_USART_CTS_PIN             7  /* For USE_USART_HW_FLOW_CONTROL */
#define BOARD_USART_RTS_PIN             5  /* For USE_USART_HW_FLOW_CONTROL */

// List of GPIO pins
#define BOARD_GPIO_PIN_LIST            {17, /* P0.17 */\
                                        18, /* P0.18 */\
                                        19, /* P0.19 */\
                                        20, /* P0.20 */\
                                        13, /* P0.13 */\
                                        14, /* P0.14 */\
                                        15, /* P0.15 */\
                                        16, /* P0.16 */\
                                        8,  /* P0.08. required by the dual_mcu app. usart wakeup pin (= BOARD_USART_RX) */\
                                        11} /* P0.11. required by the dual_mcu app (indication signal) */

// User friendly name for GPIOs (IDs mapped to the BOARD_GPIO_PIN_LIST table)
#define BOARD_GPIO_ID_LED0              0  // mapped to pin P0.17
#define BOARD_GPIO_ID_LED1              1  // mapped to pin P0.18
#define BOARD_GPIO_ID_LED2              2  // mapped to pin P0.19
#define BOARD_GPIO_ID_LED3              3  // mapped to pin P0.20
#define BOARD_GPIO_ID_BUTTON0           4  // mapped to pin P0.13
#define BOARD_GPIO_ID_BUTTON1           5  // mapped to pin P0.14
#define BOARD_GPIO_ID_BUTTON2           6  // mapped to pin P0.15
#define BOARD_GPIO_ID_BUTTON3           7  // mapped to pin P0.16
#define BOARD_GPIO_ID_USART_WAKEUP      8  // mapped to pin P0.08
#define BOARD_GPIO_ID_UART_IRQ          9  // mapped to pin P0.11

// List of LED IDs
#define BOARD_LED_ID_LIST               {BOARD_GPIO_ID_LED0, BOARD_GPIO_ID_LED1, BOARD_GPIO_ID_LED2, BOARD_GPIO_ID_LED3}

// Active low polarity for LEDs
#define BOARD_LED_ACTIVE_LOW            true

// List of button IDs
#define BOARD_BUTTON_ID_LIST           {BOARD_GPIO_ID_BUTTON0, BOARD_GPIO_ID_BUTTON1, BOARD_GPIO_ID_BUTTON2, BOARD_GPIO_ID_BUTTON3}

// Active low polarity for buttons
#define BOARD_BUTTON_ACTIVE_LOW         true

// Active internal pull-up for buttons
#define BOARD_BUTTON_INTERNAL_PULL      true

// The board supports DCDC (#define BOARD_SUPPORT_DCDC)
// Since SDK v1.2 (bootloader > v7) this option has been move to
// board/<board_name>/config.mk. Set board_hw_dcdc to yes to enable DCDC.
#ifdef BOARD_SUPPORT_DCDC
#error This option has been moved to board/<board_name>/config.mk
#endif


#endif /* BOARD_PCA10040_BOARD_H_ */
