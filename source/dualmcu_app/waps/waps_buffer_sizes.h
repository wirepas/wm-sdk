/* Copyright 2017 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

#ifndef WAPS_BUFFER_SIZES_H_
#define WAPS_BUFFER_SIZES_H_

#include "waps_frames.h" // For frame min/max length constants
#include "crc.h"

/** Worst case is _every_ WAPS UART frame character must be escaped */
#define WAPS_TX_BUFFER_SIZE     ((2 * WAPS_MAX_FRAME_LENGTH) + 2 * sizeof(crc_t) + 2)
#define WAPS_RX_BUFFER_SIZE     ((WAPS_MAX_FRAME_LENGTH + sizeof(crc_t) + 2))

#endif /* WAPS_BUFFER_SIZES_H_ */
