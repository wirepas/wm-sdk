/* Copyright 2018 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/**
 * @file
 *
 * Board definition for the
 * <a href="https://www.silabs.com/products/development-tools/thunderboard/thunderboard-sense-two-kit">Silabs Thunderboard Sense 2</a>
 */
#ifndef BOARD_TBSENSE2_BOARD_H_
#define BOARD_TBSENSE2_BOARD_H_

// Waps usart defines
#if defined USE_FTDI
#define BOARD_USART_ID                  1
#define BOARD_USART_GPIO_PORT           GPIOF
#define BOARD_USART_TX_PIN              3
#define BOARD_USART_RX_PIN              4
// ROUTELOC are dependent on GPIO defined above and mapping
// can be founded chip datasheet from Silabs
#define BOARD_USART_ROUTELOC_RXLOC      USART_ROUTELOC0_RXLOC_LOC27
#define BOARD_USART_ROUTELOC_TXLOC      USART_ROUTELOC0_TXLOC_LOC27

#else
#define BOARD_USART_ID                  0
#define BOARD_USART_GPIO_PORT           GPIOA
#define BOARD_USART_TX_PIN              0
#define BOARD_USART_RX_PIN              1

// VCOM port only supports 115200 baudrate
// This speed will be used independently of UART_BAUDRATE flag value
#define BOARD_USART_FORCE_BAUDRATE      115200

// ROUTELOC are dependent on GPIO defined above and mapping
// can be founded chip datasheet from Silabs
#define BOARD_USART_ROUTELOC_RXLOC      USART_ROUTELOC0_RXLOC_LOC0
#define BOARD_USART_ROUTELOC_TXLOC      USART_ROUTELOC0_TXLOC_LOC0
#endif

// Interrupt pin for dual mcu app, unread indication
#define BOARD_UART_INT_PIN              6
#define BOARD_UART_INT_PORT             GPIOF


#endif /* BOARD_TBSENSE2_BOARD_H_ */
