/* Copyright 2021 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/**
 * @file
 *
 * Board definition for the
 * <a href="https://industry.panasonic.eu/products/devices/wireless-connectivity/bluetooth-low-energy-modules/pan1780-nrf52840">Panasonic PAN1780 evaluation board</a>
 */
#ifndef BOARD_PAN1780_BOARD_H_
#define BOARD_PAN1780_BOARD_H_

// NRF_GPIO is mapped to NRF_P0 , for pins P0.00 ... P0.31
// Use NRF_P1 for pins P1.00 ... P1.15
// With nrf_gpio.h, use SW_pin (logical pins, port-aware)

/**
NRF_P0  SW_pin  PAN1780         Notes (recommended usage)
-----------------------------------------------------------------
P0.00   0       [XTAL 32k]    
P0.01   1       [XTAL 32k]    
P0.02   2       gpio/AIN0          (low freq)
P0.03   3       gpio/AIN1          (low freq)
P0.04   4       gpio/AIN2     
P0.05   5       UART_RTS      
P0.06   6       UART_TX       
P0.07   7       UART_CTS      
P0.08   8       UART_RX       
P0.09   9       gpio/NFC1          (low freq)
P0.10   10      gpio/NFC2          (low freq)
P0.11   11      nBUTTON1
P0.12   12      nBUTTON2/SPI1 nSS 
P0.13   13      nLED1/SPI1 MOSI    
P0.14   14      nLED2/SPI1 MISO    
P0.15   15      nLED3/SPI1 SCK    
P0.16   16      nLED4    
P0.17   17      IRQ    
P0.18   18      RESET       
P0.19   19             
P0.20   20           
P0.21   21      
P0.22   22      
P0.23   23      
P0.24   24      nBUTTON3
P0.25   25      nBUTTON4
P0.26   26      I2C SDA         
P0.27   27      I2C SCL        
P0.28   28      gpio/AIN4         (low freq)
P0.29   29      gpio/AIN5         (low freq)
P0.30   30      gpio/AIN6         (low freq)
P0.31   31      gpio/AIN7         (low freq)

NRF_P1:
P1.00   32      P1.00             (QSPI)
P1.01   33      UART4 Tx          (low freq)
P1.02   34      UART4 Rx          (low freq)
P1.03   35      gpio              (low freq)
P1.04   36      gpio              (low freq)
P1.05   37      gpio              (low freq)
P1.06   38      gpio              (low freq)
P1.07   39      gpio              (low freq)
P1.08   40      gpio         
P1.09   41      gpio         
P1.10   42      gpio              (low freq)
P1.11   43      gpio              (low freq)
P1.12   44      gpio              (low freq)
P1.13   45      gpio              (low freq)
P1.14   46      gpio              (low freq)
P1.15   47      gpio              (low freq)
*/

// Interrupt pin for dual mcu app, unread indication
#define BOARD_UART_IRQ_PIN              17

// Serial port pins
#define BOARD_USART_TX_PIN              6
#define BOARD_USART_RX_PIN              8
#define BOARD_USART_CTS_PIN             7  /* For USE_USART_HW_FLOW_CONTROL */
#define BOARD_USART_RTS_PIN             5  /* For USE_USART_HW_FLOW_CONTROL */

// List of GPIO pins for the LEDs on the board: LED 1 to LED 4
#define BOARD_LED_PIN_LIST              {13, 14, 15, 16}

// Active low polarity for LEDs
#define BOARD_LED_ACTIVE_LOW            true

// List of GPIO pins for buttons on the board: Button 1 to Button 4
#define BOARD_BUTTON_PIN_LIST           {11, 12, 24, 25}

// Active low polarity for buttons
#define BOARD_BUTTON_ACTIVE_LOW         true

// Active internal pull-up for buttons
#define BOARD_BUTTON_INTERNAL_PULL      true


#endif /* BOARD_PAN1780_BOARD_H_ */
