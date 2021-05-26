/* Copyright 2020 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */
#ifndef RADIO_H_
#define RADIO_H_

/**
 * @brief   Perform application level initialization of radio
 *
 * @note    This function is only called from low level board initialization
 *          functions. There is no need to call this explicitly if
 *          @ref board_folder "board" has been defined correctly.
 *
 */
extern void Radio_init() __attribute__((weak));

#endif /* RADIO_H_ */
