/* Copyright 2019 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

#include <stdbool.h>
#include <stdint.h>

#include "hal_api.h"
#include "board.h"

/* Map some registers constant to the USART selected */
#if BOARD_USART_ID == 0
#define BOARD_USART_CMU_BIT CMU_HFPERCLKEN0_USART0
#elif BOARD_USART_ID == 1
#define BOARD_USART_CMU_BIT CMU_HFPERCLKEN0_USART1
#elif BOARD_USART_ID == 2
#define BOARD_USART_CMU_BIT CMU_HFPERCLKEN0_USART2
#elif BOARD_USART_ID == 3
#define BOARD_USART_CMU_BIT CMU_HFPERCLKEN0_USART3
#else
#error USART ID must be 0, 1, 2 or 3
#endif

uint32_t getHFPERCLK()
{
    uint32_t div;
    div = 1U + ((CMU->HFPERPRESC & _CMU_HFPERPRESC_PRESC_MASK) >> _CMU_HFPERPRESC_PRESC_SHIFT);
    return 38400000U / div;
}

void enableHFPERCLK(bool enable)
{
    if(enable)
    {
        CMU->HFPERCLKEN0 |= BOARD_USART_CMU_BIT;
    }
    else
    {
        CMU->HFPERCLKEN0 &= ~(BOARD_USART_CMU_BIT);
    }
}
