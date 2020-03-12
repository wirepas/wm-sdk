/* Copyright 2019 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */
#include "hal_api.h"

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
