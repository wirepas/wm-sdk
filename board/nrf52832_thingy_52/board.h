/* Copyright 2020 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/**
 * @file
 *
 * Board definition for the
 * <a href="https://www.nordicsemi.com/Products/Development-hardware/Nordic-Thingy-52"> Nordic-Thingy-52</a>
 */

#ifndef _BOARD_NRF52832_THINGY_52_BOARD_H_
#define _BOARD_NRF52832_THINGY_52_BOARD_H_


// NRF_GPIO is mapped to NRF_P0 , for pins P0.00 ... P0.31
// With nrf_gpio.h, use SW_pin (logical pins, port-aware)

/**
NRF_P0  SW_pin  Thingy:52 board       
-------------------------------
P0.00   0       XL1             Low freq. crystal     
P0.01   1       XL2             Low freq. crystal      
P0.02   2       ANA/DIG0      
P0.03   3       ANA/DIG1      
P0.04   4       ANA/DIG2      
P0.05   5       SX_OSCIO        I/O expander oscillator input line
P0.06   6       MPU_INT         Motion sensor interrupt line
P0.07   7       SDA             I2C data line
P0.08   8       SCL             I2C clock line
P0.09   9       NFC1      
P0.10   10      NFC2      
P0.11   11      BUTTON  
P0.12   12      LIS_INT1        Low power accelerometer interrupt line
P0.13   13      USB_DETECT     
P0.14   14      SDA_EXT         External and low power accelerometer I2C data line
P0.15   15      SCL_EXT         External and low power accelerometer I2C clock line
P0.16   16      SX_RESET        I/O expander reset line
P0.17   17      BAT_CHG_STAT    Battery charge status
P0.18   18      MOS_1           Gate of N-MOS transistor externally available
P0.19   19      MOS_2           Gate of N-MOS transistor externally available
P0.20   20      MOS_3           Gate of N-MOS transistor externally available
P0.21   21      MOS_4           Gate of N-MOS transistor externally available
P0.22   22      CCS_INT         Gas sensor interrupt line
P0.23   23      LPS_INT         Pressure sensor interrupt line
P0.24   24      HTS_INT         Humidity sensor interrupt line
P0.25   25      MIC_DOUT        Microphone PDM data
P0.26   26      MIC_CLK         Microphone PDM clock 
P0.27   27      SPEAKER         Speaker PWM signal  
P0.28   28      BATTERY         Battery monitoring input
P0.29   29      SPK_PWR_CTRL    Speaker amplifier power control 
P0.30   30      VDD_PWD_CTRL    Power control for sensors, I/O expander, and LEDs
P0.31   31      BH_INT          Color sensor interrupt line

I/O expander pin map
SXIO0    IOEXT0                 gpio       
SXIO1    IOEXT1                 gpio           
SXIO2    IOEXT2                 gpio           
SXIO3    IOEXT3                 gpio          
SXIO4    BAT_MON_EN             Battery monitoring enable          
SXIO5    LIGHTWELL_G            Green color of the lightwell LEDs          
SXIO6    LIGHTWELL_B            Blue color of the lightwell LEDs           
SXIO7    LIGHTWELL_R            Red color of the lightwell LEDs           
SXIO8    MPU_PWR_CTRL           Motion sensor power control           
SXIO9    MIC_PWR_CTRL           Microphone power control          
SXIO10   CCS_PWR_CTRL           Gas sensor power control          
SXIO11   CCS_RESET              Gas sensor reset line           
SXIO12   CCS_WAKE               Gas sensor wake line             
SXIO13   SENSE_LED_R            Red color of the color sensor support LED           
SXIO14   SENSE_LED_G            Green color of the color sensor support LED           
SXIO15   SENSE_LED_B            Blue color of the color sensor support LED           
*/



// Serial port pins
#define BOARD_USART_TX_PIN              2
#define BOARD_USART_RX_PIN              4

// Active low polarity for LEDs
#define BOARD_LED_ACTIVE_LOW            true

// The board supports DCDC (#define BOARD_SUPPORT_DCDC)
// Since SDK v1.2 (bootloader > v7) this option has been move to
// board/<board_name>/config.mk.
#ifdef BOARD_SUPPORT_DCDC
#error This option has been moved to board/<board_name>/config.mk
#endif

#endif /* _BOARD_NRF52832_THINGY_52_BOARD_H_ */
