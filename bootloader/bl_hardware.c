/* Copyright 2020 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

#include "bl_hardware.h"

static const hardware_capabilities_t m_hw =
{
    .crystal_32k = BOARD_HW_CRYSTAL_32K,
    .dcdc = BOARD_HW_DCDC
};

const hardware_capabilities_t * Hardware_getCapabilities(void)
{
    return &m_hw;
}
