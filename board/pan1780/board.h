/* Copyright 2021 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/**
 * @file
 *
 * Board definition for the
 * <a href="https://industry.panasonic.eu/products/devices/wireless-connectivity/bluetooth-low-energy-modules/pan1780-nrf52840">Panasonic PAN1780 evaluation board</a>
 */
#ifndef BOARD_PAN1780_BOARD_H_
#define BOARD_PAN1780_BOARD_H_

// NRF_GPIO is mapped to NRF_P0 , for pins P0.00 ... P0.31
// Use NRF_P1 for pins P1.00 ... P1.15
// With nrf_gpio.h, use SW_pin (logical pins, port-aware)

/**
NRF_P0  SW_pin  PAN1780         Notes (recommended usage)
-----------------------------------------------------------------
P0.00   0       [XTAL 32k]
P0.01   1       [XTAL 32k]
P0.02   2       gpio/AIN0          (low freq)
P0.03   3       gpio/AIN1          (low freq)
P0.04   4       gpio/AIN2
P0.05   5       UART_RTS
P0.06   6       UART_TX
P0.07   7       UART_CTS
P0.08   8       UART_RX
P0.09   9       gpio/NFC1          (low freq)
P0.10   10      gpio/NFC2          (low freq)
P0.11   11      nBUTTON1
P0.12   12      nBUTTON2/SPI1 nSS
P0.13   13      nLED1/SPI1 MOSI
P0.14   14      nLED2/SPI1 MISO
P0.15   15      nLED3/SPI1 SCK
P0.16   16      nLED4
P0.17   17      IRQ
P0.18   18      RESET
P0.19   19
P0.20   20
P0.21   21
P0.22   22
P0.23   23
P0.24   24      nBUTTON3
P0.25   25      nBUTTON4
P0.26   26      I2C SDA
P0.27   27      I2C SCL
P0.28   28      gpio/AIN4         (low freq)
P0.29   29      gpio/AIN5         (low freq)
P0.30   30      gpio/AIN6         (low freq)
P0.31   31      gpio/AIN7         (low freq)

NRF_P1:
P1.00   32      P1.00             (QSPI)
P1.01   33      UART4 Tx          (low freq)
P1.02   34      UART4 Rx          (low freq)
P1.03   35      gpio              (low freq)
P1.04   36      gpio              (low freq)
P1.05   37      gpio              (low freq)
P1.06   38      gpio              (low freq)
P1.07   39      gpio              (low freq)
P1.08   40      gpio
P1.09   41      gpio
P1.10   42      gpio              (low freq)
P1.11   43      gpio              (low freq)
P1.12   44      gpio              (low freq)
P1.13   45      gpio              (low freq)
P1.14   46      gpio              (low freq)
P1.15   47      gpio              (low freq)
*/

// List of GPIO pins
#define BOARD_GPIO_PIN_LIST            {13, /* P0.13 */\
                                        14, /* P0.14 */\
                                        15, /* P0.15 */\
                                        16, /* P0.16 */\
                                        11, /* P0.11 */\
                                        12, /* P0.12 */\
                                        24, /* P0.24 */\
                                        25, /* P0.25 */\
                                        6,  /* P0.06. usart tx pin */\
                                        8,  /* P0.08. usart rx pin */\
                                        7,  /* P0.07. usart cts pin */\
                                        5,  /* P0.05. usart rts pin */\
                                        17} /* P0.17. required by the dual_mcu app (indication signal) */

// User friendly name for GPIOs (IDs mapped to the BOARD_GPIO_PIN_LIST table)
#define BOARD_GPIO_ID_LED0              0  // mapped to pin P0.13
#define BOARD_GPIO_ID_LED1              1  // mapped to pin P0.14
#define BOARD_GPIO_ID_LED2              2  // mapped to pin P0.15
#define BOARD_GPIO_ID_LED3              3  // mapped to pin P0.16
#define BOARD_GPIO_ID_BUTTON0           4  // mapped to pin P0.11
#define BOARD_GPIO_ID_BUTTON1           5  // mapped to pin P0.12
#define BOARD_GPIO_ID_BUTTON2           6  // mapped to pin P0.24
#define BOARD_GPIO_ID_BUTTON3           7  // mapped to pin P0.25
#define BOARD_GPIO_ID_USART_TX          8  // mapped to pin P0.06
#define BOARD_GPIO_ID_USART_RX          9  // mapped to pin P0.08
#define BOARD_GPIO_ID_USART_CTS         10 // mapped to pin P0.07
#define BOARD_GPIO_ID_USART_RTS         11 // mapped to pin P0.05
#define BOARD_GPIO_ID_UART_IRQ          12 // mapped to pin P0.17

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


#endif /* BOARD_PAN1780_BOARD_H_ */
