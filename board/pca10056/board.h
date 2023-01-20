/* Copyright 2018 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/**
 * @file
 *
 * Board definition for the
 * <a href="https://www.nordicsemi.com/Software-and-tools/Development-Kits/nRF52840-DK">Nordic semiconductor PCA10056 evaluation board</a>
 */
#ifndef BOARD_PCA10056_BOARD_H_
#define BOARD_PCA10056_BOARD_H_

// NRF_GPIO is mapped to NRF_P0 , for pins P0.00 ... P0.31
// Use NRF_P1 for pins P1.00 ... P1.15
// With nrf_gpio.h, use SW_pin (logical pins, port-aware)

/**
NRF_P0  SW_pin  PCA10056       PCA10059       Notes (recommended usage)
------------------------------------------------------------------------
P0.00   0       [XTAL 32k]     [XTAL 32k]
P0.01   1       [XTAL 32k]     [XTAL 32k]
P0.02   2       gpio/AIN0      0.02           (low freq)
P0.03   3       gpio/AIN1      -              (low freq)
P0.04   4       gpio/AIN2      0.04
P0.05   5       UART_RTS       -
P0.06   6       UART_TX        nLED1
P0.07   7       UART_CTS       -
P0.08   8       UART_RX        nLED2red
P0.09   9       gpio/NFC1      0.09           (low freq)
P0.10   10      gpio/NFC2      0.10           (low freq)
P0.11   11      gpio/nBUTTON1  0.11
P0.12   12      gpio/nBUTTON2  nLED2blue
P0.13   13      gpio/nLED1     0.13
P0.14   14      gpio/nLED2     0.14
P0.15   15      gpio/nLED3     0.15
P0.16   16      gpio/nLED4     -
P0.17   17      gpio/FLASH     0.17
P0.18   18      nRESET         nRESET
P0.19   19      gpio/FLASH     nRESET         (QSPI/SCK)
P0.20   20      gpio/FLASH     0.20
P0.21   21      gpio/FLASH     nRESET         (QSPI)
P0.22   22      gpio/FLASH     0.22           (QSPI)
P0.23   23      gpio/FLASH     nRESET         (QSPI)
P0.24   24      gpio/nBUTTON3  0.24
P0.25   25      gpio/nBUTTON4  nRESET
P0.26   26      gpio           0.26
P0.27   27      gpio           -
P0.28   28      gpio/AIN4      -              (low freq)
P0.29   29      gpio/AIN5      0.29           (low freq)
P0.30   30      gpio/AIN6      -              (low freq)
P0.31   31      gpio/AIN7      0.31           (low freq)

NRF_P1:
P1.00   32      gpio/SWO       1.00           (QSPI)
P1.01   33      gpio           1.01           (low freq)
P1.02   34      gpio           1.02           (low freq)
P1.03   35      gpio           -              (low freq)
P1.04   36      gpio           1.04           (low freq)
P1.05   37      gpio           -              (low freq)
P1.06   38      gpio           nSW1           (low freq)
P1.07   39      gpio           1.07           (low freq)
P1.08   40      gpio           -
P1.09   41      gpio           nLED2green
P1.10   42      gpio           1.10           (low freq)
P1.11   43      gpio           1.11           (low freq)
P1.12   44      gpio           -              (low freq)
P1.13   45      gpio           1.13           (low freq)
P1.14   46      gpio           -              (low freq)
P1.15   47      gpio           1.15           (low freq)
*/

// Serial port pins
#define BOARD_USART_TX_PIN              6
#define BOARD_USART_RX_PIN              8
#define BOARD_USART_CTS_PIN             7  /* For USE_USART_HW_FLOW_CONTROL */
#define BOARD_USART_RTS_PIN             5  /* For USE_USART_HW_FLOW_CONTROL */

// List of GPIO pins
#define BOARD_GPIO_PIN_LIST            {13, /* P0.13 */\
                                        14, /* P0.14 */\
                                        15, /* P0.15 */\
                                        16, /* P0.16 */\
                                        11, /* P0.11 */\
                                        12, /* P0.12 */\
                                        24, /* P0.24 */\
                                        25, /* P0.25 */\
                                        8,  /* P0.08. required by the dual_mcu app. usart wakeup pin (= BOARD_USART_RX) */\
                                        11} /* P0.11. required by the dual_mcu app (indication signal) */

// User friendly name for GPIOs (IDs mapped to the BOARD_GPIO_PIN_LIST table)
#define BOARD_GPIO_ID_LED0              0  // mapped to pin P0.13
#define BOARD_GPIO_ID_LED1              1  // mapped to pin P0.14
#define BOARD_GPIO_ID_LED2              2  // mapped to pin P0.15
#define BOARD_GPIO_ID_LED3              3  // mapped to pin P0.16
#define BOARD_GPIO_ID_BUTTON0           4  // mapped to pin P0.11
#define BOARD_GPIO_ID_BUTTON1           5  // mapped to pin P0.12
#define BOARD_GPIO_ID_BUTTON2           6  // mapped to pin P0.24
#define BOARD_GPIO_ID_BUTTON3           7  // mapped to pin P0.25
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


#endif /* BOARD_PCA10056_BOARD_H_ */
