/* Copyright 2018 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/**
 * @file
 *
 * Board definition for the
 * <a href="https://www.nordicsemi.com/Products/Low-power-short-range-wireless/nRF21540">Nordic semiconductor PCA10112 </a>
 */
#ifndef BOARD_PCA10112_BOARD_H_
#define BOARD_PCA10112_BOARD_H_

// PCA10112 nRF52840 SoC + nRF21540 FEM
// NRF_GPIO is mapped to NRF_P0 , for pins P0.00 ... P0.31
// Use NRF_P1 for pins P1.00 ... P1.15
// With nrf_gpio.h, use SW_pin (logical pins, port-aware)

/**
NRF_P0  SW_pin  PCA10112       Notes (recommended usage)
------------------------------------------------------------------------
P0.00   0       [XTAL 32k]
P0.01   1       [XTAL 32k]
P0.02   2       gpio/AIN0      (low freq)
P0.03   3       gpio/AIN1      (low freq)
P0.04   4       gpio/AIN2
P0.05   5       UART_RTS
P0.06   6       UART_TX
P0.07   7       UART_CTS
P0.08   8       UART_RX
P0.09   9       gpio/NFC1      (low freq)
P0.10   10      gpio/NFC2      (low freq)
P0.11   11      gpio/nBUTTON1
P0.12   12      gpio/nBUTTON2
P0.13   13      gpio/nLED1
P0.14   14      gpio/nLED2
P0.15   15      gpio/nLED3
P0.16   16      gpio/nLED4
P0.17   17      gpio/FEM_MODE
P0.18   18      nRESET
P0.19   19      gpio/FEM_RXEN
P0.20   20      gpio/FEM_ANTSEL
P0.21   21      gpio/FEM_CS
P0.22   22      gpio/FEM_TXEN
P0.23   23      gpio/FEM_PDN
P0.24   24      gpio/nBUTTON3
P0.25   25      gpio/nBUTTON4
P0.26   26      gpio
P0.27   27      gpio
P0.28   28      gpio/AIN4       (low freq)
P0.29   29      gpio/AIN5       (low freq)
P0.30   30      gpio/AIN6       (low freq)
P0.31   31      gpio/AIN7       (low freq)

NRF_P1:
P1.00   32      gpio/SWO
P1.01   33      gpio            (low freq)
P1.02   34      gpio            (low freq)
P1.03   35      gpio            (low freq)
P1.04   36      gpio            (low freq)
P1.05   37      gpio            (low freq)
P1.06   38      gpio            (low freq)
P1.07   39      gpio            (low freq)
P1.08   40      gpio
P1.09   41      gpio
P1.10   42      gpio            (low freq)
P1.11   43      gpio            (low freq)
P1.12   44      gpio            (low freq)
P1.13   45      FEM_MISO        (low freq)
P1.14   46      FEM_MOSI        (low freq)
P1.15   47      FEM_CLK         (low freq)
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
                                        5}  /* P0.05. usart rts pin */

// User friendly name for GPIOs (IDs mapped to the BOARD_GPIO_PIN_LIST table)
#define BOARD_GPIO_ID_LED1              0  // mapped to pin P0.13
#define BOARD_GPIO_ID_LED2              1  // mapped to pin P0.14
#define BOARD_GPIO_ID_LED3              2  // mapped to pin P0.15
#define BOARD_GPIO_ID_LED4              3  // mapped to pin P0.16
#define BOARD_GPIO_ID_BUTTON1           4  // mapped to pin P0.11
#define BOARD_GPIO_ID_BUTTON2           5  // mapped to pin P0.12
#define BOARD_GPIO_ID_BUTTON3           6  // mapped to pin P0.24
#define BOARD_GPIO_ID_BUTTON4           7  // mapped to pin P0.25
#define BOARD_GPIO_ID_USART_TX          8  // mapped to pin P0.06
#define BOARD_GPIO_ID_USART_RX          9  // mapped to pin P0.08
#define BOARD_GPIO_ID_USART_CTS         10 // mapped to pin P0.07
#define BOARD_GPIO_ID_USART_RTS         11 // mapped to pin P0.05

// List of LED IDs
#define BOARD_LED_ID_LIST               {BOARD_GPIO_ID_LED1, BOARD_GPIO_ID_LED2, BOARD_GPIO_ID_LED3, BOARD_GPIO_ID_LED4}

// Active low polarity for LEDs
#define BOARD_LED_ACTIVE_LOW            true

// List of button IDs
#define BOARD_BUTTON_ID_LIST           {BOARD_GPIO_ID_BUTTON1, BOARD_GPIO_ID_BUTTON2, BOARD_GPIO_ID_BUTTON3, BOARD_GPIO_ID_BUTTON4}

// Active low polarity for buttons
#define BOARD_BUTTON_ACTIVE_LOW         true

// The board supports DCDC (#define BOARD_SUPPORT_DCDC)
// Since SDK v1.2 (bootloader > v7) this option has been move to
// board/<board_name>/config.mk. Set board_hw_dcdc to yes to enable DCDC.
#ifdef BOARD_SUPPORT_DCDC
#error This option has been moved to board/<board_name>/config.mk
#endif

// FEM control
#define BOARD_FEM_MODE_PIN              17      // P0.17
#define BOARD_FEM_RXEN_PIN              19      // P0.19
#define BOARD_FEM_ANTSEL_PIN            20      // P0.20
#define BOARD_FEM_CS_PIN                21      // P0.21
#define BOARD_FEM_TXEN_PIN              22      // P0.22
#define BOARD_FEM_PDN_PIN               23      // P0.23

// FEM SPI (not used for anything)
#define BOARD_FEM_SPI_MOSI_PIN          45      // P1.13
#define BOARD_FEM_SPI_MISO_PIN          46      // P1.14
#define BOARD_FEM_SPI_CLK_PIN           47      // P1.15

// Initialize FEM
void Fem_init(void);


#endif /* BOARD_PCA10112_BOARD_H_ */
