/* Copyright 2020 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "mcu.h"
#include "flash.h"

/* Driver specifics */
#define FLASH_BASE_BAND_CFG     CMU_AUXHFRCOCTRL_BAND_14MHZ
#define FLASH_BASE_BAND_HZ      14000000ul

/**
 * \brief   Enable writing to flash.
 *          Not necessary with EFR32
 * \return  none
 */
void Flash_enableWrite(void)
{
    // Not necessary with EFM32
}

/**
 * \brief   Enable flash erase.
 *          Not necessary with EFR32
 * \return  none
 */
void Flash_enableErase(void)
{
    // Not necessary with EFM32
}


/* Low level access for hardware */
__STATIC_INLINE void Flash_waitBusy(void)
{
    /* Wait for the write to complete */
    while ((MSC->STATUS & MSC_STATUS_BUSY))
    {
    }
}

__STATIC_INLINE void Flash_waitWrite(void)
{
    while ((MSC->STATUS & MSC_STATUS_WDATAREADY) == 0)
    {
    }
}

__STATIC_INLINE void Flash_setBaseAddress(uint32_t addr)
{
    MSC->ADDRB = addr;
#if defined(_SILICON_LABS_32B_SERIES_1)
    MSC->WRITECMD = MSC_WRITECMD_LADDRIM;
#endif
}

__STATIC_INLINE void Flash_startErase(void)
{
    MSC->WRITECMD = MSC_WRITECMD_ERASEPAGE;
}

/**
 * \brief   Enable write and erase access of the flash.
 * Internal function not in flash.h
 * \return  none
 */
void Flash_enableAccess(void)
{
    /* Enable writing to the MSC */
    MSC->WRITECTRL |= MSC_WRITECTRL_WREN;
}


/**
 * \brief   Disables write and erase access of the flash.
 * \return  none
 */
void Flash_disableAccess(void)
{
    /* Disable writing to the MSC */
    MSC->WRITECTRL &= ~MSC_WRITECTRL_WREN;
}

/**
 * \brief   To initialize flash HW.
 * In EFR32 only the MSC registers need to be initialized
 * \return  none
 */
void Flash_init(void)
{
    /* Unlock the MSC */
    MSC->LOCK = MSC_LOCK_LOCKKEY_UNLOCK;

    /* Disable writing to the flash */
    Flash_disableAccess();
}

/* Access for hardware (FLASH MSC) */
void __attribute__ ((section(".ramtext")))
Flash_write(void * to, const void * from, size_t amount)
{
    uint32_t p_words, written;
    uint32_t * p_base = to;
 #if defined(_SILICON_LABS_32B_PLATFORM_2_GEN_1)
    uint32_t *to_ = (uint32_t *) p_base;
 #endif
    const uint32_t * p_data = from;
    /* Amount is bytes, convert to words */
    amount /= sizeof(uint32_t);
    /* Enable access to flash */
    Flash_enableAccess();
    for(written = 0; written < amount;)
    {
        /* Load base address */
        Flash_setBaseAddress((uint32_t)(p_base + written));
        /* How many words until PAGE boundary */
        p_words = (FLASH_PAGE_SIZE -
                  ((uint32_t) (p_base + written)) %
                  FLASH_PAGE_SIZE) / sizeof(uint32_t);
        /* Increment written */
        if(p_words > amount - written)
        {
            p_words = amount - written;
        }
        written += p_words;
        /* Write words until amount or page boundary */
        while(p_words--)
        {
            uint32_t tmp;
            // From might not be aligned, but memcpy handles this
            memcpy(&tmp, p_data, sizeof(uint32_t));
            Flash_waitWrite();
            /* Set data shift register */
            MSC->WDATA = tmp;
#if defined(_SILICON_LABS_32B_SERIES_1)
            /* Trigger write once */
            MSC->WRITECMD = MSC_WRITECMD_WRITEONCE;
#endif
            p_data++;
            /* Wait for the write to complete */
            Flash_waitBusy();

#if defined(_SILICON_LABS_32B_PLATFORM_2_GEN_1)
            // Errata FLASH_E201 – Potential Program Failure after Power On ??
           if( *to_ != tmp){
                Flash_setBaseAddress( (uint32_t)to_ );
                /* Set data shift register */
                MSC->WDATA = tmp;
                /* Trigger write once */
                MSC->WRITECMD = MSC_WRITECMD_WRITEONCE;
                /* Wait for the write to complete */
                Flash_waitBusy();
            }
            to_++;
#endif

        }
    }
    Flash_disableAccess();
}

void __attribute__ ((section(".ramtext")))
Flash_erasePage(size_t page_base)
{
    /* Enable writing to the MSC */
    Flash_enableAccess();
    /* Load address */
    Flash_setBaseAddress(page_base);
    /* Send erase page command */
    Flash_startErase();
    /* Wait for the erase to complete */
    Flash_waitBusy();
    /* Disable writing to the MSC */
    Flash_disableAccess();
}
