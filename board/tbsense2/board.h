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

// ROUTELOC are dependent on GPIO defined above and mapping
// can be founded chip datasheet from Silabs
#define BOARD_USART_ROUTELOC_RXLOC      USART_ROUTELOC0_RXLOC_LOC27
#define BOARD_USART_ROUTELOC_TXLOC      USART_ROUTELOC0_TXLOC_LOC27

// List of GPIO pins
#define BOARD_GPIO_PIN_LIST     	   {{GPIOD, 8},  /* PD08 */\
                                        {GPIOD, 9},  /* PD09 */\
                                        {GPIOD, 14}, /* PD14 */\
                                        {GPIOD, 15}, /* PD15 */\
                                        {GPIOF, 3},  /* PF03. usart tx pin */\
                                        {GPIOF, 4},  /* PF04. usart rx pin */\
                                        {GPIOF, 6}}  /* PF06. required by the dual_mcu app (indication signal) */
#else

#define BOARD_USART_ID                  0

// VCOM port only supports 115200 baudrate
// This speed will be used independently of UART_BAUDRATE flag value
#define BOARD_USART_FORCE_BAUDRATE      115200

// ROUTELOC are dependent on GPIO defined above and mapping
// can be founded chip datasheet from Silabs
#define BOARD_USART_ROUTELOC_RXLOC      USART_ROUTELOC0_RXLOC_LOC0
#define BOARD_USART_ROUTELOC_TXLOC      USART_ROUTELOC0_TXLOC_LOC0

// List of GPIO pins
#define BOARD_GPIO_PIN_LIST     	   {{GPIOD, 8},  /* PD08 */\
                                        {GPIOD, 9},  /* PD09 */\
                                        {GPIOD, 14}, /* PD14 */\
                                        {GPIOD, 15}, /* PD15 */\
                                        {GPIOA, 0},  /* PA00. usart tx pin */\
                                        {GPIOA, 1},  /* PA01. usart rx pin */\
                                        {GPIOF, 6}}  /* PF06. required by the dual_mcu app (indication signal) */
#endif

// User friendly name for GPIOs (IDs mapped to the BOARD_GPIO_PIN_LIST table)
#define BOARD_GPIO_ID_LED_R             0  // mapped to pin PD08
#define BOARD_GPIO_ID_LED_G             1  // mapped to pin PD09
#define BOARD_GPIO_ID_BUTTON0           2  // mapped to pin PD14
#define BOARD_GPIO_ID_BUTTON1           3  // mapped to pin PD15
#define BOARD_GPIO_ID_USART_TX          4  // mapped to BOARD_USART_TX_PIN
#define BOARD_GPIO_ID_USART_RX          5  // mapped to BOARD_USART_RX_PIN
#define BOARD_GPIO_ID_UART_IRQ          6  // mapped to pin PF06

// I2C configuration: SDA on PC4, SCL on PC5 (ENV_I2C on Thunderboard Sense 2)
#define USE_I2C1
#define BOARD_I2C_GPIO_PORT             GPIOC
#define BOARD_I2C_SDA_PIN               4
#define BOARD_I2C_SCL_PIN               5
#define BOARD_I2C_ROUTELOC_SDALOC I2C_ROUTELOC0_SDALOC_LOC17
#define BOARD_I2C_ROUTELOC_SCLLOC I2C_ROUTELOC0_SCLLOC_LOC17

// List of LED IDs
#define BOARD_LED_ID_LIST               {BOARD_GPIO_ID_LED_R, BOARD_GPIO_ID_LED_G}

// Active high polarity for LEDs
#define BOARD_LED_ACTIVE_LOW            false

// List of button IDs
#define BOARD_BUTTON_ID_LIST            {BOARD_GPIO_ID_BUTTON0, BOARD_GPIO_ID_BUTTON1}

// Active low polarity for buttons
#define BOARD_BUTTON_ACTIVE_LOW         true

// Board has external pull-up for buttons
#define BOARD_BUTTON_INTERNAL_PULL false


#endif /* BOARD_TBSENSE2_BOARD_H_ */
