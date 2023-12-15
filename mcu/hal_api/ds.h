/* Copyright 2017 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

#ifndef SOURCE_WAPS_APP_DRIVERS_DS_H_
#define SOURCE_WAPS_APP_DRIVERS_DS_H_

/**
 * \brief   Initialize deep sleep control module
 */
void DS_Init(void);

/**
 * \brief   Enable deep sleep
 */
void DS_Enable(uint32_t source);

/**
 * \brief   Disable deep sleep
 */
void DS_Disable(uint32_t source);

// Bitmasks for deep sleep disable bits
#define DS_SOURCE_USART         0x00000001
#define DS_SOURCE_DEBUG         0x00000002
#define DS_SOURCE_INIT          0x00000004

#endif /* SOURCE_WAPS_APP_DRIVERS_DS_H_ */
