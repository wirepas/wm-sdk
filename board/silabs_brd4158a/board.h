/* Copyright 2023 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/**
 * @file
 *
 * Board definition for a board composed of
 * <a href="https://www.silabs.com/documents/public/user-guides/ug282-brd4158a-user-guide.pdf">Silabs starter kit</a>
 * and <a href="https://www.silabs.com/documents/public/reference-manuals/brd4158a-rm.pdf">brd4158a radio module</a>
 */
#ifndef BOARD_SILABS_BRD4158A_BOARD_H_
#define BOARD_SILABS_BRD4158A_BOARD_H_

// By default VCOM port is configured to 115200
// This speed will be used independently of UART_BAUDRATE flag value
#define BOARD_USART_FORCE_BAUDRATE     115200

// Serial port
#define BOARD_USART_ID                 0
#define BOARD_USART_TX_PORT            GPIOA
#define BOARD_USART_TX_PIN             0
#define BOARD_USART_RX_PORT            GPIOA
#define BOARD_USART_RX_PIN             1

// ROUTELOC are dependent on GPIO defined above and mapping
// can be founded chip datasheet from Silabs
#define BOARD_USART_ROUTELOC_RXLOC     USART_ROUTELOC0_RXLOC_LOC0
#define BOARD_USART_ROUTELOC_TXLOC     USART_ROUTELOC0_TXLOC_LOC0

// List of GPIO pins
#define BOARD_GPIO_PIN_LIST            {{GPIOF, 4}, /* PF04 LED0 */\
                                        {GPIOF, 5}, /* PF05 LED1 */\
                                        {GPIOF, 6}, /* PF06 BUTTON0 */\
                                        {GPIOF, 7}, /* PF07 BUTTON1 */\
                                        {GPIOA, 1}, /* PA01. required by the dual_mcu app. usart wakeup pin (= BOARD_USART_RX) */\
                                        {GPIOA, 5}} /* PA05 (USART VCOM enable pin) */

// User friendly name for GPIOs (IDs mapped to the BOARD_GPIO_PIN_LIST table)
#define BOARD_GPIO_ID_LED0             0  // mapped to pin PF04
#define BOARD_GPIO_ID_LED1             1  // mapped to pin PF05
#define BOARD_GPIO_ID_BUTTON0          2  // mapped to pin PF06
#define BOARD_GPIO_ID_BUTTON1          3  // mapped to pin PF07
#define BOARD_GPIO_ID_USART_WAKEUP     4  // mapped to pin PA01
#define BOARD_GPIO_ID_VCOM_ENABLE      5  // mapped to pin PA05

// List of LED IDs
#define BOARD_LED_ID_LIST              {BOARD_GPIO_ID_LED0, BOARD_GPIO_ID_LED1}

// Active high polarity for LEDs
#define BOARD_LED_ACTIVE_LOW           false

// List of button IDs
#define BOARD_BUTTON_ID_LIST           {BOARD_GPIO_ID_BUTTON0, BOARD_GPIO_ID_BUTTON1}

// Active low polarity for buttons
#define BOARD_BUTTON_ACTIVE_LOW         true

// Handle button pull up/down in SW
#define BOARD_BUTTON_INTERNAL_PULL      false

#endif // BOARD_SILABS_BRD4158A_BOARD_H_
