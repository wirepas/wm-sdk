/* Copyright 2019 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

#include "mcu.h"
#include "board.h"

void Board_custom_init(void)
{
    // This board requires FEM driver, as there is no bypass mode. The stack
    // _must_ control it for the RF to work.
    Fem_init();
}

