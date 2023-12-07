/* Copyright 2023 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/**
 * @file
 * Board definition for the Thingy:9161.
 *
 * <a href="https://www.nordicsemi.com/products/nrf9161">nRF9161</a>
 */
#ifndef BOARD_PCA20049_BOARD_H_
#define BOARD_PCA20049_BOARD_H_

// Thingy91+ modem initialization AT commands to define antenna path.
// BOARD_AT_COMMANDS is list of AT commands, where
// each AT command is separated by null character ('\0'), and
// end of the list is indicated with double null characters ("\0\0")
#define BOARD_AT_COMMANDS "AT%XMIPIRFFEDEV=1,4,71,198,248\0" \
                          "AT%XMIPIRFFECTRL=1,0,1,28,248\0" \
                          "AT%XMIPIRFFECTRL=1,1,1,28,56,13,0,0,8,8,715,4,4,770,12,12,829,11,11,863,130,130,892,1,1,939,129,129,978,26,26,1042,8,8,1118,4,4,1270,12,12,1386,14,14,1523,130,130,2200\0" \
                          "AT%XMIPIRFFECTRL=1,2,1,28,184\0" \
                          "AT%XMIPIRFFECTRL=1,3,1,28,184\0" \
                          "AT+CFUN=0\0"

// NRF_GPIO is mapped to NRF_P0 , for pins P0.00 ... P0.31
// With nrf_gpio.h, use SW_pin (logical pins, port-aware)

/**
NRF_P0  SW_pin  PCA20049                Notes (recommended usage)
------------------------------------------------------------------------
P0.00   0       VCOM0_TXD
P0.01   1       VCOM0_RXD
P0.02   2       VCOM0_CTS
P0.03   3       VCOM0_RTS
P0.04   4       VCOM1_TXD
P0.05   5       VCOM1_RXD
P0.06   6       VCOM1_CTS
P0.07   7       VCOM1_RTS
P0.08   8       I2C-SCL
P0.09   9       I2C-SDA
P0.10   10      UART_IRQ
P0.11   11      ACC_INT
P0.12   12      FLASH_CS
P0.13   13      SPI_SCK
P0.14   14      SPI_MOSI
P0.15   15      SPI_MISO
P0.16   16      WIFI_IRQ
P0.17   17      WIFI_CS
P0.18   18      EXP_BOARD_PIN2
P0.19   19      EXP_BOARD_PIN1
P0.20   20      nRF53_RESET
P0.21   21      TRACE_CLK
P0.22   22      TRACE_DATA0
P0.23   23      TRACE_DATA1
P0.24   24      TRACE_DATA2
P0.25   25      TRACE_DATA3
P0.26   26      BUTTON1
P0.27   27      WIFI_VDDIO_EN
P0.28   28      WIFI_EN
P0.29   29      LED1_RED
P0.30   30      LED1_BLUE
P0.31   31      LED1_GREEN
*/

// Serial port pins
#define BOARD_USART_TX_PIN               0 // P0.00
#define BOARD_USART_RX_PIN               1 // P0.01
#define BOARD_USART_CTS_PIN              2 // P0.02. For USE_USART_HW_FLOW_CONTROL
#define BOARD_USART_RTS_PIN              3 // P0.03. For USE_USART_HW_FLOW_CONTROL

// List of GPIO pins
#define BOARD_GPIO_PIN_LIST            {29, /* P0.29 */\
                                        30, /* P0.30 */\
                                        31, /* P0.31 */\
                                        26, /* P0.26 */\
                                         1, /* P0.01. required by the dual_mcu app. usart wakeup pin (= BOARD_USART_RX) */\
                                        10} /* P0.10. required by the dual_mcu app (indication signal) */

// User friendly name for GPIOs (IDs mapped to the BOARD_GPIO_PIN_LIST table)
#define BOARD_GPIO_ID_LED1              0  // mapped to pin P0.29
#define BOARD_GPIO_ID_LED2              1  // mapped to pin P0.30
#define BOARD_GPIO_ID_LED3              2  // mapped to pin P0.31
#define BOARD_GPIO_ID_BUTTON1           3  // mapped to pin P0.26
#define BOARD_GPIO_ID_USART_WAKEUP      4  // mapped to usart rx pin
#define BOARD_GPIO_ID_UART_IRQ          5  // mapped to pin P0.10

// List of LED IDs mapped to GPIO IDs: LED 1 to LED 6
#define BOARD_LED_ID_LIST              {BOARD_GPIO_ID_LED1, BOARD_GPIO_ID_LED2, BOARD_GPIO_ID_LED3}

// List of button IDs mapped to GPIO IDs
#define BOARD_BUTTON_ID_LIST           {BOARD_GPIO_ID_BUTTON1}

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

// External Flash Memory using Wirepas designed flex-PCB
#define EXT_FLASH_SPI_SCK              13 // P0.13 TP34
#define EXT_FLASH_SPI_MOSI             14 // P0.14 TP35
#define EXT_FLASH_SPI_MISO             15 // P0.15 TP36
#define EXT_FLASH_CS                   12 // P0.12 TP33
#define EXT_FLASH_SPIM_P               NRF_SPIM1
// Enable external flash memory debugging using LEDs.
#define EXT_FLASH_DRIVER_DEBUG_LED

// I2C Port pin
#define USE_I2C2
#define BOARD_I2C_SCL_PIN              8 // P0.08
#define BOARD_I2C_SDA_PIN              9 // P0.09

// The I2C address for environment sensor
#define BME688_I2C_ADDRESS          0x76

// The I2C address for accelerometer
#define ADXL367_I2C_ADDRESS         0x1D

// Not defined as there are no free GPIO pins to use with Thingy91
//#define HAL_DBG_MCU_PIN                 ??      // P0.??
//#define HAL_DBG_TASK_MGMT_PIN           ??      // P0.??
//#define HAL_DBG_TASK_ROUTE_PIN          ??      // P0.??
//#define HAL_DBG_TASK_WAPS_PIN           ??      // P0.??
//#define HAL_DBG_TASK_MAC_PIN            ??      // P0.??
//#define HAL_DBG_IRQ_USART_PIN           ??      // P0.??
//#define HAL_DBG_RADIO_CRC_PIN           ??      // P0.??
//#define HAL_DBG_IRQ_RTC_PIN             ??      // P0.??
//#define HAL_DBG_IRQ_RADIO_PIN           ??      // P0.??
//#define HAL_DBG_RADIO_TX_PIN            ??      // P0.??
//#define HAL_DBG_RADIO_RX_PIN            ??      // P0.??
//#define HAL_DBG_ASSERT_PIN              ??      // P0.??

#endif /* BOARD_PCA20035_BOARD_H_ */
