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

// List of GPIO pins
#define BOARD_GPIO_PIN_LIST            {23, /* P0.23 */\
                                        22, /* P0.22 */\
                                        24, /* P0.24 */\
                                        20, /* P0.20. usart tx pin */\
                                        19} /* P0.19. usart rx pin */

// User friendly name for GPIOs (IDs mapped to the BOARD_GPIO_PIN_LIST table)
#define BOARD_GPIO_ID_LED_R             0  // mapped to pin P0.23
#define BOARD_GPIO_ID_LED_G             1  // mapped to pin P0.22
#define BOARD_GPIO_ID_LED_B             2  // mapped to pin P0.24
#define BOARD_GPIO_ID_USART_TX          3  // mapped to pin P0.20
#define BOARD_GPIO_ID_USART_RX          4  // mapped to pin P0.19

// List of LED IDs
#define BOARD_LED_ID_LIST               {BOARD_GPIO_ID_LED_R, BOARD_GPIO_ID_LED_G, BOARD_GPIO_ID_LED_B}

// Active low polarity for LEDs
#define BOARD_LED_ACTIVE_LOW            true

// The board supports DCDC (#define BOARD_SUPPORT_DCDC)
// Since SDK v1.2 (bootloader > v7) this option has been move to
// board/<board_name>/config.mk.
#ifdef BOARD_SUPPORT_DCDC
#error This option has been moved to board/<board_name>/config.mk
#endif

#endif /* _BOARD_NRF52832_MDK_V2_BOARD_H_ */
