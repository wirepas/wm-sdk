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
#define BOARD_USART_CTS_PIN             7  /* For USE_USART_HW_FLOW_CONTROL */
#define BOARD_USART_RTS_PIN             31  /* For USE_USART_HW_FLOW_CONTROL */

/* List of GPIO pins for the LEDs on the B204 board:
LED     B204    / nrf52832 pin
GREEN:  GPIO_7  / P0.16
RED:    GPIO_1  / P0.08
BLUE:   GPIO_8  / P0.18
*/
#define BOARD_LED_PIN_LIST              {16, 8, 18}

// The board supports DCDC (#define BOARD_SUPPORT_DCDC)
// Since SDK v1.2 (bootloader > v7) this option has been move to
// board/<board_name>/config.mk. Set board_hw_dcdc to yes to enable DCDC.
#ifdef BOARD_SUPPORT_DCDC
#error This option has been moved to board/<board_name>/config.mk
#endif


#endif /* BOARD_UBLOX_B204_H_ */
