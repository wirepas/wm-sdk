/* Copyright 2023 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/**
 * @file
 *
 * Board definition for a board composed of a
 * <a href="https://www.silabs.com/documents/public/user-guides/ug524-brd2601b-user-guide.pdf"> </a>
 * <a href="https://www.silabs.com/documents/public/schematic-files/BRD2601B-A01-schematic.pdff"> </a>
 */
#ifndef BOARD_SILABS_BRD2610B_BOARD_H_
#define BOARD_SILABS_BRD2610B_BOARD_H_

// VCOM port only supports 115200 baudrate
// This speed will be used independently of UART_BAUDRATE flag value
#define BOARD_USART_FORCE_BAUDRATE      115200

#define BOARD_SPI                       USART0
#define BOARD_SPIROUTE                  GPIO->USARTROUTE[0]
#define BOARD_SPI_MOSI_PORT    GPIO_PORTC      // SPI_COPI name in schema
#define BOARD_SPI_MISO_PORT    GPIO_PORTC      // SPI_CIPO name in schema
#define BOARD_SPI_SCKL_PORT    GPIO_PORTC
#define BOARD_SPI_MOSI_PIN     3
#define BOARD_SPI_MISO_PIN     2
#define BOARD_SPI_SCKL_PIN     1



#define USE_I2C1            
#define BOARD_I2C_GPIO_PORT             GPIOC
#define BOARD_I2C_SDA_PIN               4
#define BOARD_I2C_SCL_PIN               5
#define BOARD_I2C_ROUTELOC_SDALOC I2C_ROUTELOC0_SDALOC_LOC17  
#define BOARD_I2C_ROUTELOC_SCLLOC I2C_ROUTELOC0_SCLLOC_LOC17  



// Serial port
#define BOARD_USART_ID                  0
#define BOARD_USART_TX_PORT             GPIO_PORTA
#define BOARD_USART_TX_PIN              5               // UART_TX VCOM & Mini Simplicity
#define BOARD_USART_RX_PORT             GPIO_PORTA
#define BOARD_USART_RX_PIN              6               // UART_RX VCOM & Mini Simplicity

// List of GPIO pins
#define BOARD_GPIO_PIN_LIST            {{GPIO_PORTD, 2}, /* PD02  Red LED */ \
                                        {GPIO_PORTA, 3}, /* PA04  Green LED */ \
                                        {GPIO_PORTB, 0}, /* PB00  BLue LED */\
                                        {GPIO_PORTB, 2}, /* PB01  Button0 */\
                                        {GPIO_PORTB, 3}, /* PB01  Button1 */ \
                                        {GPIO_PORTA, 7}, /* PA07. IMU CS, needs SENSOR_ENABLE to be set true to work  */\
                                        {GPIO_PORTA, 6}, /* PA06  USART wake-up */\
                                        {GPIO_PORTC, 0}, /* PD04. SPI_FLASH_CS  */ \
                                        {GPIO_PORTC, 8}, /* PC08. I2C_MICS_ENABLE  */ \
                                        {GPIO_PORTD, 5}, /* PD05. I2C_WSE  */ \
                                        {GPIO_PORTC, 9}, /* PC09. SENSOR_ENABLE  */ \
                                        {GPIO_PORTA, 0}} /* PA00. ADC_VREF_ENABLE  */ \




// User friendly name for GPIOs (IDs mapped to the BOARD_GPIO_PIN_LIST table)
#define BOARD_GPIO_ID_LED_RED           0  // mapped to pin PD02
#define BOARD_GPIO_ID_LED_GREEN         1  // mapped to pin PA04
#define BOARD_GPIO_ID_LED_BLUE          2  // mapped to pin PB00

#define BOARD_GPIO_ID_BUTTON0           3  // mapped to pin PB02
#define BOARD_GPIO_ID_BUTTON1           4  // mapped to pin PB03

#define BOARD_SPI_CS_IMU                5  // mapped to pin PA07 NOTE! Needs SENSOR_ENABLE to be set also
#define BOARD_GPIO_ID_USART_WAKEUP      6  // mapped to pin PA06
#define BOARD_SPI_FLASH_CS              7  // mapped to pin PC00 32 MBit serial Flash

#define BOARD_I2C_MICS_ENABLE           8  // mapped to pin PC08, both mics 1 = powered, 0 = not powered
#define BOARD_I2C_WSE                   9  // mapped to pin PD05, selects which mic is readed with i2c

#define BOARD_SENSOR_ENABLE             10 // mapped to pin PC09, Enables next sensors to i2c bus:
                                           // Temperature, Humidity, Hall effect, Ambient light, Barometric pressure.
                                           // Signal enables 6-Axis Inertia sensor to SPI bus

#define BOARD_ADC_VREF_ENABLE           11 // mapped to pin PA00, 1 = reference is enabled, 0 = reference is not enabled

// List of LED IDs
#define BOARD_LED_ID_LIST              {BOARD_GPIO_ID_LED_RED, BOARD_GPIO_ID_LED_GREEN, BOARD_GPIO_ID_LED_BLUE}

// Active high polarity for LEDs
#define BOARD_LED_ACTIVE_LOW false

// List of button IDs
#define BOARD_BUTTON_ID_LIST           {BOARD_GPIO_ID_BUTTON0, BOARD_GPIO_ID_BUTTON1}

// Active button connects signal to ground
#define BOARD_BUTTON_ACTIVE_LOW         true

// Board has external pull-up for buttons, internal pull-ups are not needed
#define BOARD_BUTTON_INTERNAL_PULL      false

#endif /* BOARD_SILABS_BRD2610B_BOARD_H_ */
