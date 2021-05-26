/* Copyright 2020 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/**
 * @file
 *
 * Board definition for the
 * <a href="https://www.seeedstudio.com/nRF52832-MDK-V2-IoT-Micro-Development-Kit-p-3049.html"> nrf52832-MDK V2 kit</a>
 */

#ifndef _BOARD_NRF52832_MDK_V2_BOARD_H_
#define _BOARD_NRF52832_MDK_V2_BOARD_H_

// Serial port pins
#define BOARD_USART_TX_PIN              20
#define BOARD_USART_RX_PIN              19

// List of GPIO pins for the LEDs on the board: LED R, G, B
#define BOARD_LED_PIN_LIST              {23, 22, 24}

// Active low polarity for LEDs
#define BOARD_LED_ACTIVE_LOW            true

// The board supports DCDC (#define BOARD_SUPPORT_DCDC)
// Since SDK v1.2 (bootloader > v7) this option has been move to
// board/<board_name>/config.mk.
#ifdef BOARD_SUPPORT_DCDC
#error This option has been moved to board/<board_name>/config.mk
#endif

#endif /* _BOARD_NRF52832_MDK_V2_BOARD_H_ */
