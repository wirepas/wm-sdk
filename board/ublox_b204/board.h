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

/* List of GPIO pins
LED     B204    / nrf52832 pin
GREEN:  GPIO_7  / P0.16
RED:    GPIO_1  / P0.08
BLUE:   GPIO_8  / P0.18
*/
#define BOARD_GPIO_PIN_LIST            {16, /* P0.16 */\
                                        8,  /* P0.08 */\
                                        18, /* P0.18 */\
                                        6,  /* P0.06. usart tx pin */\
                                        5,  /* P0.05. usart rx pin */\
                                        7,  /* P0.07. usart cts pin */\
                                        31} /* P0.31. usart rts pin */

// User friendly name for GPIOs (IDs mapped to the BOARD_GPIO_PIN_LIST table)
#define BOARD_GPIO_ID_LED_G             0 // mapped to pin P0.16
#define BOARD_GPIO_ID_LED_R             1 // mapped to pin P0.08
#define BOARD_GPIO_ID_LED_B             2 // mapped to pin P0.18
#define BOARD_GPIO_ID_USART_TX          3 // mapped to pin P0.06
#define BOARD_GPIO_ID_USART_RX          4 // mapped to pin P0.05
#define BOARD_GPIO_ID_USART_CTS         5 // mapped to pin P0.07
#define BOARD_GPIO_ID_USART_RTS         6 // mapped to pin P0.31

// List of LED IDs mapped to GPIO IDs
#define BOARD_LED_ID_LIST               {BOARD_GPIO_ID_LED_G, BOARD_GPIO_ID_LED_R, BOARD_GPIO_ID_LED_B}

// The board supports DCDC (#define BOARD_SUPPORT_DCDC)
// Since SDK v1.2 (bootloader > v7) this option has been move to
// board/<board_name>/config.mk. Set board_hw_dcdc to yes to enable DCDC.
#ifdef BOARD_SUPPORT_DCDC
#error This option has been moved to board/<board_name>/config.mk
#endif

#endif /* BOARD_UBLOX_B204_H_ */
