/* Copyright 2021 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */
#include "board.h"
#include "power.h"

#include "mcu.h"

void Power_enableDCDC()
{
#if BOARD_HW_DCDC
    NRF_REGULATORS->DCDCEN = REGULATORS_DCDCEN_DCDCEN_Enabled << REGULATORS_DCDCEN_DCDCEN_Pos;
#endif
}
