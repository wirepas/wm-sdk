/* Copyright 2017 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

#ifndef WAPS_COMM_H_
#define WAPS_COMM_H_

#include <stdbool.h>
#include <stdint.h>

/** Callback for a valid looking frame */
typedef bool(*new_frame_cb_f)(void *, uint32_t);

#include "uart/waps_uart.h"

#endif /* WAPS_COMM_H_ */
