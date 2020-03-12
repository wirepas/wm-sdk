/* Copyright 2018 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

#include "external_flash.h"

/* Dummy external flash driver that does nothing.
 * It is defined as weak symbol and can be overwritten in the board specific
 * folder (e.g board/pca10040/bootloader/).
 */


extFlash_res_e __attribute__((weak)) externalFlash_init(void)
{
    /* external flash driver is optional. So return OK even if there
     * is no driver.
     */
    return EXTFLASH_RES_OK;
}

extFlash_res_e __attribute__((weak))
            externalFlash_startRead(void * to, const void * from, size_t amount)
{
    (void)to;
    (void)from;
    (void)amount;

    return EXTFLASH_RES_NODRIVER;
}

extFlash_res_e __attribute__((weak))
        externalFlash_startWrite(void * to, const void * from, size_t amount)
{
    (void)to;
    (void)from;
    (void)amount;

    return EXTFLASH_RES_NODRIVER;
}

extFlash_res_e __attribute__((weak))
    externalFlash_startErase(size_t * sector_base, size_t * number_of_sector)
{
    (void)sector_base;
    (void)number_of_sector;

    return EXTFLASH_RES_NODRIVER;
}

bool __attribute__((weak)) externalFlash_isBusy(void)
{
    return false;
}

extFlash_res_e __attribute__((weak)) externalFlash_getInfo(flash_info_t * info)
{
    (void)info;

    return EXTFLASH_RES_NODRIVER;
}
