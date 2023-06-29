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

// Serial port pins for UART1
#define BOARD_USART_TX_PIN              29
#define BOARD_USART_RX_PIN              28
#define BOARD_USART_CTS_PIN             26  /* For USE_USART_HW_FLOW_CONTROL */
#define BOARD_USART_RTS_PIN             27  /* For USE_USART_HW_FLOW_CONTROL */

// List of GPIO pins
#define BOARD_GPIO_PIN_LIST            {2,  /* P0.02 */\
                                        3,  /* P0.03 */\
                                        4,  /* P0.04 */\
                                        5,  /* P0.05 */\
                                        6,  /* P0.06 */\
                                        7,  /* P0.07 */\
                                        28, /* P0.28. required by the dual_mcu app. usart wakeup pin (= BOARD_USART_RX) */\
                                        11} /* P0.11. required by the dual_mcu app (indication signal) */


// User friendly name for GPIOs (IDs mapped to the BOARD_GPIO_PIN_LIST table)
#define BOARD_GPIO_ID_LED1              0  // mapped to pin P0.02
#define BOARD_GPIO_ID_LED2              1  // mapped to pin P0.03
#define BOARD_GPIO_ID_LED3              2  // mapped to pin P0.04
#define BOARD_GPIO_ID_LED4              3  // mapped to pin P0.05
#define BOARD_GPIO_ID_BUTTON1           4  // mapped to pin P0.06
#define BOARD_GPIO_ID_BUTTON2           5  // mapped to pin P0.07
#define BOARD_GPIO_ID_USART_WAKEUP      6  // mapped to pin P0.28
#define BOARD_GPIO_ID_UART_IRQ          7  // mapped to pin P0.11

// List of LED IDs
#define BOARD_LED_ID_LIST              {BOARD_GPIO_ID_LED1, BOARD_GPIO_ID_LED2, BOARD_GPIO_ID_LED3, BOARD_GPIO_ID_LED4}

// List of button IDs mapped to GPIO IDs
#define BOARD_BUTTON_ID_LIST           {BOARD_GPIO_ID_BUTTON1, BOARD_GPIO_ID_BUTTON2}

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
// Note! nRF52840 P0.19 needs to be set before
//       nRF9160 has access to external flash
#define EXT_FLASH_SPI_MOSI             11 // P0.11
#define EXT_FLASH_SPI_MISO             12 // P0.12
#define EXT_FLASH_SPI_SCK              13 // P0.13
#define EXT_FLASH_CS                   25 // P0.25
#define EXT_FLASH_SPIM_P               NRF_SPIM1
// Enable external flash memory debugging using LEDs.
#define EXT_FLASH_DRIVER_DEBUG_LED

// Debug traces (not available with all applications as pins are conflicted)
// TODO: Defined but not tested if they really works
#define HAL_DBG_MCU_PIN                 16      // P0.16  gpio/AIN3
#define HAL_DBG_TASK_MGMT_PIN           17      // P0.17  gpio/AIN4
#define HAL_DBG_TASK_ROUTE_PIN          18      // P0.18  gpio/AIN5
#define HAL_DBG_TASK_WAPS_PIN           19      // P0.19  gpio/AIN6
#define HAL_DBG_TASK_MAC_PIN            20      // P0.20  gpio/AIN7
#define HAL_DBG_IRQ_USART_PIN           10      // P0.10  gpio
#define HAL_DBG_RADIO_CRC_PIN           30      // P0.30  gpio/SDA
#define HAL_DBG_IRQ_RTC_PIN             31      // P0.31  gio/SCL
#define HAL_DBG_IRQ_RADIO_PIN           2       // P0.02 gpio/LED-1
#define HAL_DBG_RADIO_TX_PIN            3       // P0.03 gpio/LED-2
#define HAL_DBG_RADIO_RX_PIN            4       // P0.04 gpio/LED-3
#define HAL_DBG_ASSERT_PIN              5       // P0.05 gpio/LED-4

#endif /* BOARD_PCA10090_BOARD_H_ */
