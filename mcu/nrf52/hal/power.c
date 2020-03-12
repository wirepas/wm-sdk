/* Copyright 2018 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */
#include "board.h"
#include "power.h"

#include "mcu.h"

void Power_enableDCDC()
{
#if defined(BOARD_SUPPORT_DCDC)
    NRF_POWER->DCDCEN = POWER_DCDCEN_DCDCEN_Enabled << POWER_DCDCEN_DCDCEN_Pos;
#endif
}

