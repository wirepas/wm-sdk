/* Copyright 2020 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/**
 * @file
 *
 * Board definition for the
 * <a href="https://www.nordicsemi.com/Software-and-tools/Development-Kits/nRF52833-DK">Nordic semiconductor PCA10100 evaluation board</a>
 */
#ifndef BOARD_PCA10100_BOARD_H_
#define BOARD_PCA10100_BOARD_H_

// NRF_GPIO is mapped to NRF_P0 , for pins P0.00 ... P0.31
// Use NRF_P1 for pins P1.00 ... P1.09
// With nrf_gpio.h, use SW_pin (logical pins, port-aware)

/**
NRF_P0  SW_pin  PCA10056         Notes (recommended usage)
-----------------------------------------------------------
P0.00   0       [XTAL 32k]
P0.01   1       [XTAL 32k]
P0.02   2       gpio/AIN0        (low freq)
P0.03   3       gpio/AIN1        (low freq)
P0.04   4       gpio/AIN2
P0.05   5       UART_RTS         (gpio/AIN3)
P0.06   6       UART_TX
P0.07   7       UART_CTS         (gpio/TRACECLK)
P0.08   8       UART_RX
P0.09   9       gpio/NFC1        (low freq)
P0.10   10      gpio/NFC2        (low freq)
P0.11   11      gpio/nBUTTON1
P0.12   12      gpio/nBUTTON2
P0.13   13      gpio/nLED1
P0.14   14      gpio/nLED2
P0.15   15      gpio/nLED3
P0.16   16      gpio/nLED4
P0.17   17      gpio
P0.18   18      nRESET
P0.19   19      gpio
P0.20   20      gpio
P0.21   21      gpio
P0.22   22      gpio
P0.23   23      gpio
P0.24   24      gpio/nBUTTON3
P0.25   25      gpio/nBUTTON4
P0.26   26      gpio
P0.27   27      gpio
P0.28   28      gpio/AIN4        (low freq)
P0.29   29      gpio/AIN5        (low freq)
P0.30   30      gpio/AIN6        (low freq)
P0.31   31      gpio/AIN7        (low freq)

NRF_P1:
P1.00   32      gpio/SWO
P1.01   33      gpio             (low freq)
P1.02   34      gpio             (low freq)
P1.03   35      gpio             (low freq)
P1.04   36      gpio             (low freq)
P1.05   37      gpio             (low freq)
P1.06   38      gpio             (low freq)
P1.07   39      gpio             (low freq)
P1.08   40      gpio
P1.09   41      gpio
*/

// Interrupt pin for dual mcu app, unread indication
#define BOARD_UART_IRQ_PIN              11

// Serial port pins
#define BOARD_USART_TX_PIN              6
#define BOARD_USART_RX_PIN              8
#define BOARD_USART_CTS_PIN             7  /* For USE_USART_HW_FLOW_CONTROL */
#define BOARD_USART_RTS_PIN             5  /* For USE_USART_HW_FLOW_CONTROL */

// Pwm output for pwm_driver app
#define BOARD_PWM_OUTPUT_GPIO           28

// List of GPIO pins for the LEDs on the board: LED 1 to LED 4
#define BOARD_LED_PIN_LIST              {13, 14, 15, 16}

// Active low polarity for LEDs
#define BOARD_LED_ACTIVE_LOW            true

// List of GPIO pins for buttons on the board: Button 1 to Button 4
#define BOARD_BUTTON_PIN_LIST           {11, 12, 24, 25}

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


#endif /* BOARD_PCA10100_BOARD_H_ */
