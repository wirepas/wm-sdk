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
    // Set bit
    m_sleep_mask |= source;
    // Disable deep sleep
    Sys_disableDs(true);
    // End of atomic operation
    Sys_exitCriticalSection();
}
