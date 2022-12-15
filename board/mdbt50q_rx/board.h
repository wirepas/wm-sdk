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

// List of GPIO pins
#define BOARD_GPIO_PIN_LIST             {45, /* P1.13 */\
                                         43, /* P1.11 */\
                                         15} /* P0.15 */

// User friendly name for GPIOs (IDs mapped to the BOARD_GPIO_PIN_LIST table)
#define BOARD_GPIO_ID_LED0              0  // mapped to pin P1.13
#define BOARD_GPIO_ID_LED1              1  // mapped to pin P1.11
#define BOARD_GPIO_ID_BUTTON0           2  // mapped to pin P0.15

// List of LED IDs
#define BOARD_LED_ID_LIST              {BOARD_GPIO_ID_LED0, BOARD_GPIO_ID_LED1}

// Active low polarity for LEDs
#define BOARD_LED_ACTIVE_LOW            true

// List of button IDs
#define BOARD_BUTTON_ID_LIST            {BOARD_GPIO_ID_BUTTON0}

// Active low polarity for buttons
#define BOARD_BUTTON_ACTIVE_LOW         true

// Active internal pull-up for buttons
#define BOARD_BUTTON_INTERNAL_PULL      true

// The board supports DCDC (#define BOARD_SUPPORT_DCDC)
// Since SDK v1.2 (stack v5.1.x) this option has been move to
// board/<board_name>/config.mk. Set board_hw_dcdc to yes to enable DCDC.
#ifdef BOARD_SUPPORT_DCDC
#error This option has been moved to board/<board_name>/config.mk
#endif

#endif /* BOARD_MDBT50Q_RX_H_ */
