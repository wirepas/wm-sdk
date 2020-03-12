/* Copyright 2018 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/**
 * \brief   Generic power codes.
 */
#ifndef POWER_H_
#define POWER_H_

/**
 * \brief   Enable DCDC converter.
 *
 * @note    This function is only called from low level board initialization
 *          functions. There is no need to call this explicitly if
 *          @ref board_folder "board" has been defined correctly.
 *
 * @note    This function is used only on nRF52 architectures.
 */
extern void Power_enableDCDC() __attribute__((weak));

#endif /* POWER_H_ */
