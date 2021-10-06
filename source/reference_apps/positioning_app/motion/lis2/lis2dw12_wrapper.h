/**
 * @file    lis2dw12_wrapper.h
 * @brief   Wrapper on top of LIS2DW12 driver from STMicroelectronics GitHub
 * This file is the wrapper to expose a simple API for getting the acceleration
 * from lis2dw12 device. It makes the glue between the ST driver and Wirepas
 * i2c driver.
 *
 * @copyright   Wirepas Ltd 2021
 */
#ifndef _LIS2DW12_WRAPPER_H_
#define _LIS2DW12_WRAPPER_H_
#include "board.h"

/** LIS2DW12 interrupt pin for motion monitoring, @note Only one pin shall be defined in board.h */
#ifdef BOARD_LIS2DW12_INT1_PIN
#define LIS2DW12_MOTION_USE_INT1
#define MOTION_MON_INT_PIN BOARD_LIS2DW12_INT1_PIN
#elif defined BOARD_LIS2DW12_INT2_PIN
#define LIS2DW12_MOTION_USE_INT2
#define MOTION_MON_INT_PIN BOARD_LIS2DW12_INT2_PIN
#endif

/** LIS2DW12 montion monitoring sampling rate */
#define LIS2DW12_INT_DURATION LIS2DW12_XL_ODR_12Hz5

/** LIS2DW12 motion monitoring sampling precision */
#define LIS2DW12_INT_OPPERATING_MODE LIS2DW12_CONT_LOW_PWR_12bit


/** Time to get first measure from POWER DOWN @1Hz, with 50% margin. */
#define LIS2DW12_WAKE_UP_TIME_MS   (7*1.5)

/** Time to get the measurement if motion monitoring is activated */
#define LIS2DW12_WAKE_UP_TIME_MON_MS   (1)

#endif /* _LIS2DW12_WRAPPER_H_ */
