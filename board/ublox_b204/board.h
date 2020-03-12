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

// The board supports DCDC
#define BOARD_SUPPORT_DCDC


#endif /* BOARD_UBLOX_B204_H_ */
