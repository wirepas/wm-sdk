/* Copyright 2021 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/**
 * @file voltage.h
 *
 * @brief Voltage measurement *examples* for various processors.
 *
 * @note  Voltage measurements are only *examples*. It should be verified
 * whether given measurement mechanism applies to hardware design or not.
 */

#ifndef _HAL_VOLTAGE_H_
#define _HAL_VOLTAGE_H_

#include <stdint.h>

/**
 * @brief   Initialize voltage measurement mechanisms. Need to do once before
 *          measurements.
 */
void Mcu_voltageInit(void);

/**
 * @brief   Measure processor voltage. What is measured depends on processor
 *          type
 *
 * @return  Voltage in millivolts
 *
 * @note  Voltage measurements are only *examples*. It should be verified
 * whether given measurement mechanism applies to hardware design or not.
 *
 */
uint16_t Mcu_voltageGet(void);

#endif /* _HAL_VOLTAGE_H_ */
