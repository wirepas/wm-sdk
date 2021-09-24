/* Copyright 2021 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/**
 * @file
 * Board definition for the
 * <a href="https://infocenter.nordicsemi.com/pdf/nRF9160_DK_HW_User_Guide_v1.0.pdf</a>
 */
#ifndef BOARD_PCA10090_BOARD_H_
#define BOARD_PCA10090_BOARD_H_


// NRF_GPIO is mapped to NRF_P0 , for pins P0.00 ... P0.31
// With nrf_gpio.h, use SW_pin (logical pins, port-aware)

/**
NRF_P0  SW_pin  PCA10090                Notes (recommended usage)
------------------------------------------------------------------------
P0.00   0       gpio/UART2_RX
P0.01   1       gpio/UART2_TX
P0.02   2       gpio/LED-1
P0.03   3       gpio/LED-2
P0.04   4       gpio/LED-3
P0.05   5       gpio/LED-4
P0.06   6       gpio/BUTTON-1
P0.07   7       gpio/BUTTON-2
P0.08   8       gpio/SWITCH-1
P0.09   9       gpio/SWITCH-2
P0.10   10      gpio
P0.11   11      gpio/MOSI               external flash memory
P0.12   12      gpio/MOSO               external flash memory
P0.13   13      gpio/SCK/AIN0           external flash memory
P0.14   14      gpio/UART2_RTS/AIN1
P0.15   15      gpio/UART2_CTS/AIN2
P0.16   16      gpio/AIN3
P0.17   17      gpio/AIN4
P0.18   18      gpio/AIN5
P0.19   19      gpio/AIN6
P0.20   20      gpio/AIN7
P0.21   21      TRACECLK
P0.22   22      TRACEDATA[0]            Debug connector P18
P0.23   23      TRACEDATA[1]            Debug connector P18
P0.24   24      TRACEDATA[2]            Debug connector P18
P0.25   25      gpio/CS/TRACEDATA[3]    CS for external flash memory / P18
P0.26   26      UART1_CTS
P0.27   27      UART1_RTS
P0.28   28      UART1_RX
P0.29   29      UART1_TX
P0.30   30      gpio/SDA                I2C
P0.31   31      gpio/SCL                I2C
*/

// Interrupt pin for dual mcu app, unread indication
#define BOARD_UART_IRQ_PIN              11

// Serial port pins for UART1
#define BOARD_USART_TX_PIN              29
#define BOARD_USART_RX_PIN              28
#define BOARD_USART_CTS_PIN             26  /* For USE_USART_HW_FLOW_CONTROL */
#define BOARD_USART_RTS_PIN             27  /* For USE_USART_HW_FLOW_CONTROL */

// List of GPIO pins for the LEDs on the board: LED 1 to LED 4
#define BOARD_LED_PIN_LIST              {2, 3, 4, 5}

// List of GPIO pins for buttons on the board
#define BOARD_BUTTON_PIN_LIST           {6, 7}

// Pwm output for pwm_driver app
#define BOARD_PWM_OUTPUT_GPIO           28


// Active low polarity for LEDs
#define BOARD_LED_ACTIVE_LOW            true


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


#endif /* BOARD_PCA10056_BOARD_H_ */
