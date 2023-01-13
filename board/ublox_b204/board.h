/* Copyright 2019 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/**
 * @file
 *
 * Board definition for the
 * <a href="https://www.u-blox.com/en/product/b204">Ublox b204 dongle</a>
 */
#ifndef BOARD_UBLOX_B204_H_
#define BOARD_UBLOX_B204_H_

// Serial port pins
#define BOARD_USART_TX_PIN              6
#define BOARD_USART_RX_PIN              5
#define BOARD_USART_CTS_PIN             7   /* For USE_USART_HW_FLOW_CONTROL */
#define BOARD_USART_RTS_PIN             31  /* For USE_USART_HW_FLOW_CONTROL */

/* List of GPIO pins
LED     B204    / nrf52832 pin
GREEN:  GPIO_7  / P0.16
RED:    GPIO_1  / P0.08
BLUE:   GPIO_8  / P0.18
*/
#define BOARD_GPIO_PIN_LIST            {16, /* P0.16 */\
                                        8,  /* P0.08 */\
                                        18, /* P0.18 */\
                                        5}  /* P0.05. required by the dual_mcu app. usart wakeup pin (= BOARD_USART_RX) */

// User friendly name for GPIOs (IDs mapped to the BOARD_GPIO_PIN_LIST table)
#define BOARD_GPIO_ID_LED_G             0 // mapped to pin P0.16
#define BOARD_GPIO_ID_LED_R             1 // mapped to pin P0.08
#define BOARD_GPIO_ID_LED_B             2 // mapped to pin P0.18
#define BOARD_GPIO_ID_USART_WAKEUP      3 // mapped to pin P0.05

// List of LED IDs mapped to GPIO IDs
#define BOARD_LED_ID_LIST               {BOARD_GPIO_ID_LED_G, BOARD_GPIO_ID_LED_R, BOARD_GPIO_ID_LED_B}

// The board supports DCDC (#define BOARD_SUPPORT_DCDC)
// Since SDK v1.2 (bootloader > v7) this option has been move to
// board/<board_name>/config.mk. Set board_hw_dcdc to yes to enable DCDC.
#ifdef BOARD_SUPPORT_DCDC
#error This option has been moved to board/<board_name>/config.mk
#endif

#endif /* BOARD_UBLOX_B204_H_ */
