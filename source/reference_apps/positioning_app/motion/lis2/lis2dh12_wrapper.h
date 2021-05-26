/**
 * @file    lis2dh12_wrapper.h
 * @brief   Wrapper on top of LIS2DH12 driver from STMicroelectronics GitHub
 * This file is the wrapper to expose a simple API for getting the acceleration
 * from lis2dh12 device. It makes the glue between the ST driver and Wirepas
 * i2c driver.
 *
 * @copyright   Wirepas Ltd 2021
 */
#ifndef _LIS2DH12_WRAPPER_H_
#define _LIS2DH12_WRAPPER_H_
#include "board.h"

/** LIS2DH12 interrupt pin for motion monitoring, @note Only one pin shall be defined in board.h */
#ifdef BOARD_LIS2DH12_INT1_PIN
#define LIS2DH12_MOTION_USE_INT1
#define MOTION_MON_INT_PIN BOARD_LIS2DH12_INT1_PIN
#elif defined BOARD_LIS2DH12_INT2_PIN
#define LIS2DH12_MOTION_USE_INT2
#define MOTION_MON_INT_PIN BOARD_LIS2DH12_INT2_PIN
#endif

/** LIS2DH12 montion monitoring sampling rate */
#define LIS2DH12_INT_DURATION LIS2DH12_ODR_10Hz

/** LIS2DH12 motion monitoring sampling precision */
#define LIS2DH12_INT_OPPERATING_MODE LIS2DH12_LP_8bit


/** Time to get first measure from POWER DOWN @1Hz, with 50% margin. */
#define LIS2DH12_WAKE_UP_TIME_MS   (7*1.5)

/** Time to get the measurement if motion monitoring is activated */
#define LIS2DH12_WAKE_UP_TIME_MON_MS   (1)

#endif /* _LIS2DH12_WRAPPER_H_ */
