/* Copyright 2019 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */
#include "board_init.h"
#include "board.h"
#include "power.h"

#if defined(USE_FEM)
#include "fem_driver.h"
#endif

/* Declaration of weak a custom board initialization that
 * can be overwritten for each board under board/<board_name>/board_custom_init.c
 */
void Board_custom_init(void) __attribute__((weak));

void Board_init()
{
    Board_custom_init();

    Power_enableDCDC();

#if defined(USE_FEM)
    // Initialize external RF Front End Module (if any)
    Fem_init();
#endif
}

