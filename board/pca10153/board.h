/* Copyright 2023 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/**
 * @file
 * Board definition for the
 * <a href="https://infocenter.nordicsemi.com/pdf/nRF9160_DK_HW_User_Guide_v1.0.pdf</a>   TODO !!! Update to nRF9161-DK when link to PDF is available
 */
#ifndef BOARD_PCA10153_BOARD_H_
#define BOARD_PCA10153_BOARD_H_



// NRF_GPIO is mapped to NRF_P0 , for pins P0.00 ... P0.31
// With nrf_gpio.h, use SW_pin (logical pins, port-aware)

/**
NRF_P0  SW_pin  PCA10153                Notes (recommended usage)
------------------------------------------------------------------------
P0.00    0      gpio/LED-1
P0.01    1      gpio/LED-2
P0.02    2      gpio
P0.03    3      gpio
P0.04    4      gpio/LED-3
P0.05    5      gpio/LED-4
P0.06    6      gpio
P0.07    7      gpio
P0.08    8      gpio/BUTTON-1
P0.09    9      gpio/BUTTON-2
P0.10   10      gpio                    dualmcu_app indication signal
P0.11   11      gpio/MOSI               external flash memory MOSI
P0.12   12      gpio/MISO               external flash memory MISO
P0.13   13      gpio/SCK/AIN0           external flash memory SCK
P0.14   14      gpio/UART1_RTS/AIN1
P0.15   15      gpio/UART1_CTS/AIN2
P0.16   16      gpio/UART2_RTS/AIN3
P0.17   17      gpio/UART2_CTS/AIN4
P0.18   18      gpio/BUTTON-3/AIN5
P0.19   19      gpio/BUTTON-4/AIN6
P0.20   20      gpio/CS/AIN7            external flash memory CS
P0.21   21      TRACECLK
P0.22   22      TRACEDATA[0]            Debug connector
P0.23   23      TRACEDATA[1]            Debug connector
P0.24   24      TRACEDATA[2]            Debug connector
P0.25   25      TRACEDATA[3]            Debug connector
P0.26   26      UART1_RX
P0.27   27      UART1_TX
P0.28   28      UART2_RX
P0.29   29      UART2_TX
P0.30   30      gpio/SDA                I2C
P0.31   31      gpio/SCL                I2C
*/

// Serial port pins for UART1
#define BOARD_USART_TX_PIN              27
#define BOARD_USART_RX_PIN              26
#define BOARD_USART_CTS_PIN             15  /* For USE_USART_HW_FLOW_CONTROL */
#define BOARD_USART_RTS_PIN             14  /* For USE_USART_HW_FLOW_CONTROL */

// List of GPIO pins
#define BOARD_GPIO_PIN_LIST            {0,  /* P0.00 LED-1 */\
                                        1,  /* P0.01 LED-2 */\
                                        4,  /* P0.04 LED-3 */\
                                        5,  /* P0.05 LED-4 */\
                                        8,  /* P0.08 BUTTON-1 */\
                                        9,  /* P0.09 BUTTON-2 */\
                                        18, /* P0.18 BUTTON-3 */\
                                        19, /* P0.19 BUTTON-4 */\
                                        26, /* P0.26 required by the dual_mcu app. usart wakeup pin (= BOARD_USART_RX) */\
                                        10} /* P0.10 required by the dual_mcu app (indication signal) */

// User friendly name for GPIOs (IDs mapped to the BOARD_GPIO_PIN_LIST table)
#define BOARD_GPIO_ID_LED1              0  // mapped to pin P0.00
#define BOARD_GPIO_ID_LED2              1  // mapped to pin P0.01
#define BOARD_GPIO_ID_LED3              2  // mapped to pin P0.04
#define BOARD_GPIO_ID_LED4              3  // mapped to pin P0.05
#define BOARD_GPIO_ID_BUTTON1           4  // mapped to pin P0.08
#define BOARD_GPIO_ID_BUTTON2           5  // mapped to pin P0.09
#define BOARD_GPIO_ID_BUTTON3           6  // mapped to pin P0.18
#define BOARD_GPIO_ID_BUTTON4           7  // mapped to pin P0.19
#define BOARD_GPIO_ID_USART_WAKEUP      8  // mapped to pin P0.26
#define BOARD_GPIO_ID_UART_IRQ          9  // mapped to pin P0.10

// List of LED IDs
#define BOARD_LED_ID_LIST              {BOARD_GPIO_ID_LED1, BOARD_GPIO_ID_LED2, BOARD_GPIO_ID_LED3, BOARD_GPIO_ID_LED4}

// List of button IDs mapped to GPIO IDs
#define BOARD_BUTTON_ID_LIST           {BOARD_GPIO_ID_BUTTON1, BOARD_GPIO_ID_BUTTON2, BOARD_GPIO_ID_BUTTON3, BOARD_GPIO_ID_BUTTON4}

// Active low polarity for LEDs
#define BOARD_LED_ACTIVE_LOW            false

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

// External Flasn Memory
#define EXT_FLASH_SPI_MOSI             11 // P0.11
#define EXT_FLASH_SPI_MISO             12 // P0.12
#define EXT_FLASH_SPI_SCK              13 // P0.13
#define EXT_FLASH_CS                   20 // P0.20
#define EXT_FLASH_SPIM_P               NRF_SPIM1
// Enable external flash memory debugging using LEDs.
#define EXT_FLASH_DRIVER_DEBUG_LED

#endif /* BOARD_PCA10153_BOARD_H_ */
