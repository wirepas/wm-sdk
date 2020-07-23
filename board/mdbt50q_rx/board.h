/* Copyright 2018 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/**
 * @file
 *
 * Board definition for the
 * <a href="https://www.raytac.com/product/ins.php?index_id=89">Raytac MDBT50Q-RX dongle</a>
 */
#ifndef BOARD_MDBT50Q_RX_H_
#define BOARD_MDBT50Q_RX_H_


// Serial port
// There is no serial port but a USB connection


// List of GPIO pins for the LEDs on the board: LED 1 P1.13 to LED 2 P1.11
#define BOARD_LED_PIN_LIST              {45, 43}

// Active low polarity for LEDs
#define BOARD_LED_ACTIVE_LOW            true

// List of GPIO pins for buttons on the board: Button 1 to Button 4
#define BOARD_BUTTON_PIN_LIST           {15}

// Active low polarity for buttons
#define BOARD_BUTTON_ACTIVE_LOW         true

// The board supports DCDC
#define BOARD_SUPPORT_DCDC

#endif /* BOARD_MDBT50Q_RX_H_ */
