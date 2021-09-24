/**
 * @file  lis2_dev.h
 * @brief Defines the interface for STMems device.
 * @copyright  Wirepas Ltd 2021
 */
#ifndef _LIS2_DEV_H_
#define _LIS2_DEV_H_

#ifdef LIS2DH12
#include "lis2dh12_reg.h"

#ifdef LIS2DH12_I2C

#elif defined LIS2DH12_SPI
#define BOARD_SPI_LIS2_CS_PIN BOARD_SPI_LIS2DH12_CS_PIN
#endif
#endif

#ifdef LIS2DW12
#include "lis2dw12_reg.h"

#ifdef LIS2DW12_I2C

#elif defined LIS2DW12_SPI
#define BOARD_SPI_LIS2_CS_PIN BOARD_SPI_LIS2DW12_CS_PIN
#endif
#endif

void LIS2_dev_init(stmdev_ctx_t * dev);

#endif /* _LIS2_DEV_H_ */