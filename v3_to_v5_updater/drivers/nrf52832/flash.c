/* Copyright 2018 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

#include <string.h>
#include "mcu.h"
#include "flash.h"

volatile uint32_t EVENT_READBACK;

void Flash_init(void)
{
}

/* Low level access for hardware */
static void Flash_waitBusy(void)
{
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy)
    {
    }
}

void Flash_enableWrite(void)
{
    NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Wen;
    Flash_waitBusy();
}

void Flash_disableAccess(void)
{
    NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Ren;
    Flash_waitBusy();
}

void Flash_enableErase(void)
{
    NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Een;
    Flash_waitBusy();
}

void Flash_erase(size_t page)
{
    NRF_NVMC->ERASEPAGE = (uint32_t)page;
    Flash_waitBusy();
    // Fix for NRF52 errata 44 (Flash read fails after erase):
    EVENT_READBACK = *((uint32_t *)0x10000FFC);
}

void Flash_write(void * to, const void * from, size_t amount)
{
    // NOTE: This variable is necessary, the compiler produces code that fails
    // if assignment done via *(uint32_t *)to++ = tmp;
    uint32_t * to_ = (uint32_t *)to;
    // Writing can be done only in words, so must check alignments
    /* Enable WRITE */
    Flash_enableWrite();
    /* Write data */
    for(uint32_t tmp = 0;
        amount > 0;
        amount -= sizeof(uint32_t),
        from += sizeof(uint32_t))
    {
        // From might not be aligned, but memcpy handles this
        memcpy(&tmp, from, sizeof(uint32_t));
        *to_++ = tmp;
        Flash_waitBusy();
    }
    /* Disable WRITE */
    Flash_disableAccess();
}

void Flash_erasePage(size_t page_base)
{
    /* Enable ERASE */
    Flash_enableErase();
    /* Start ERASE */
    Flash_erase(page_base);
    /* Disable ERASE */
    Flash_disableAccess();
}
