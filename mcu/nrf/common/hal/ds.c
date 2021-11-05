/* Copyright 2017 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */


#include <stdint.h>
#include "ds.h"
#include "api.h"

// Bitmask holding different sleep control bits
static volatile uint32_t m_sleep_mask = 0;

void DS_Init(void)
{
    m_sleep_mask = 0;
}

void DS_Enable(uint32_t source)
{
    // Atomic operation
    Sys_enterCriticalSection();
    // Clear bit
    m_sleep_mask &= ~source;
    // If mask is clear, enable deep sleep
    if(m_sleep_mask == 0)
    {
        // Enable deep sleep
        Sys_disableDs(false);
    }
    Sys_exitCriticalSection();
}

void DS_Disable(uint32_t source)
{
    // Atomic operation
    Sys_enterCriticalSection();
    // Some sources are declared NIL on this platform
    if(source == DS_SOURCE_USART_POWER)
    {
        // Usart power-up procedure does not need ds disable as the system
        // is very fast to wake up from deep sleep
        goto exit_no_change;
    }
    // Set bit
    m_sleep_mask |= source;
    // Disable deep sleep
    Sys_disableDs(true);
exit_no_change:
    Sys_exitCriticalSection();
}
