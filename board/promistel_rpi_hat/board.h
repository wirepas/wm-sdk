/* Copyright 2019 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/**
 * @file
 *
 * Board definition for the
 * <a href="https://promistel.com/en/iot.html">Promistel raspberry pi hat</a>
 */
#ifndef BOARD_PROMISTEL_RPI_HAT_BOARD_H_
#define BOARD_PROMISTEL_RPI_HAT_BOARD_H_

// Serial port pins
#define BOARD_USART_TX_PIN              45 /* P1.13 */
#define BOARD_USART_RX_PIN              29
#define BOARD_USART_CTS_PIN             7  /* For USE_USART_HW_FLOW_CONTROL */
#define BOARD_USART_RTS_PIN             5  /* For USE_USART_HW_FLOW_CONTROL */

// List of GPIO pins
#define BOARD_GPIO_PIN_LIST            {15, /* P0.15 */\
                                        16, /* P0.16 */\
                                        24, /* P0.24 */\
                                        14, /* P0.14 */\
                                        29, /* P0.29. required by the dual_mcu app. usart wakeup pin (= BOARD_USART_RX) */\
                                        23} /* P0.23. required by the dual_mcu app (indication signal) */

// User friendly name for GPIOs (IDs mapped to the BOARD_GPIO_PIN_LIST table)
#define BOARD_GPIO_ID_LED_R             0 // mapped to pin P0.15
#define BOARD_GPIO_ID_LED_G             1 // mapped to pin P0.16
#define BOARD_GPIO_ID_LED_B             2 // mapped to pin P0.24
#define BOARD_GPIO_ID_BUTTON0           3 // mapped to pin P0.14
#define BOARD_GPIO_ID_USART_WAKEUP      4 // mapped to pin P0.29
#define BOARD_GPIO_ID_UART_IRQ          5 // mapped to pin P0.23

// List of LED IDs
#define BOARD_LED_ID_LIST               {BOARD_GPIO_ID_LED_R, BOARD_GPIO_ID_LED_G, BOARD_GPIO_ID_LED_B}

// Active low polarity for LEDs
#define BOARD_LED_ACTIVE_LOW            true

// List of button IDs
#define BOARD_BUTTON_ID_LIST           {BOARD_GPIO_ID_BUTTON0}

// Active low polarity for buttons
#define BOARD_BUTTON_ACTIVE_LOW         true

// Active internal pull-up for buttons
#define BOARD_BUTTON_INTERNAL_PULL      true

// Define the SPI instance to use
#define USE_SPI0

// SPI Port pins
#define BOARD_SPI_SCK_PIN               19
#define BOARD_SPI_MOSI_PIN              20
#define BOARD_SPI_MISO_PIN              21

// SPI Chip Select pin
#define BOARD_SPI_CS_PIN                17

// The board supports DCDC (#define BOARD_SUPPORT_DCDC)
// Since SDK v1.2 (bootloader > v7) this option has been move to
// board/<board_name>/config.mk. Set board_hw_dcdc to yes to enable DCDC.
#ifdef BOARD_SUPPORT_DCDC
#error This option has been moved to board/<board_name>/config.mk
#endif

#endif /* BOARD_PROMISTEL_RPI_HAT_BOARD_H_ */
