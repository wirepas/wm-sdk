/* Copyright 2020 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

#ifndef HARDWARE_H_
#define HARDWARE_H_

#include <stdbool.h>

/** \brief  Hardware features that can be installed on a board. */
typedef struct
{
    /** True if 32kHz crystal is present; default:true
     *  (introduced in bootloader v7).
     */
    bool crystal_32k;
    /** True if DCDC converter is enabled; default:true
     * (introduced in bootloader v7).
     */
    bool dcdc;
} hardware_capabilities_t;

/**
 * \brief   Returns board hardware capabilities.
 * \return  Return a structure \ref bl_hardware_capabilities_t with
 *          hardware features installed on the board.
 * \note    There is no need to define this function, it is integrated in the
 *          SDK and is controlled from each board/<board_name>/config.mk.
 */
const hardware_capabilities_t * hardware_getCapabilities(void);

#endif //HARDWARE_H_
