/**
 * @file  lis2_dev.h
 * @brief Defines the interface for STMems device.
 * @copyright  Wirepas Ltd 2021
 */
#ifndef _LIS2_DEV_H_
#define _LIS2_DEV_H_
#include "lis2dh12_reg.h"

#ifdef LIS2DH12_I2C

#elif defined LIS2DH12_SPI

#endif


void LIS2_dev_init(stmdev_ctx_t * dev);

#endif /* _LIS2_DEV_H_ */