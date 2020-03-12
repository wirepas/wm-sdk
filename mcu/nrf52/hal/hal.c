/* Copyright 2017 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */
#include "hal_api.h"

/* For reading back cleared interrupt events in NRF52 */
__IO uint32_t                   EVENT_READBACK; /* declared in nrf.h */

/* Status of module */
static bool                     m_opened;

bool HAL_Open(void)
{
    if(!m_opened)
    {
        // Initialize deep sleep control module
        DS_Init();
        m_opened = true;
    }
    return true;
}
