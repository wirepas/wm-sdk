/* Copyright 2018 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

#ifndef EXTERNAL_FLASH_H_
#define EXTERNAL_FLASH_H_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>


/** \brief  External flash operations result */
typedef enum
{
    EXTFLASH_RES_OK       = 0, /* Operation is successful */
    EXTFLASH_RES_ERROR    = 1, /* Error during operation */
    EXTFLASH_RES_BUSY     = 2, /* Flash driver is busy */
    EXTFLASH_RES_NODRIVER = 3, /* Flash driver not implemented */
    EXTFLASH_RES_PARAM    = 4  /* Invalid parameters */
} extFlash_res_e;

/** \brief  Flash memory info definition */
typedef struct
{
    /** Size in bytes of the flash */
    size_t flash_size;
    /** Size of a write page */
    size_t write_page_size;
    /** Size of an erase sector */
    size_t erase_sector_size;
    /** Minimum write alignment supported by the flash hardware. Write address
     *  must be aligned on write_alignment and write amount must be a multiple
     *  of write_alignment bytes.
     */
    size_t write_alignment;
    /** Time taken by the Flash chipset to write one byte in uS */
    /** \note For internal flash driver write (erase) time is 0. Du to hardware
     *  limitation the driver is fully synchronous so the write (erase) to flash
     *  is finished when startWrite (startErase) returns.
     */
    uint32_t byte_write_time;
    /** Time taken by the Flash chipset write one page in uS */
    uint32_t page_write_time;
    /** Time taken by the Flash chipset to erase on sector in uS */
    uint32_t sector_erase_time;
    /** Time taken by startWrite() call for one byte in uS */
    uint32_t byte_write_call_time;
    /** Time taken by startWrite() call for one page in uS */
    uint32_t page_write_call_time;
    /** Time taken by startErase() call for one sector in uS */
    uint32_t sector_erase_call_time;
    /** Time taken by isBusy() call in uS */
    uint32_t is_busy_call_time;
} flash_info_t;


/**
 * \brief  Initialize the external flash driver.
 * \return Result code, \ref EXTFLASH_RES_OK if successful.
 *         See \ref extFlash_res_e for other result codes.
 */
extFlash_res_e externalFlash_init(void);

/**
 * \brief  Read bytes from external flash.
 * \param  to
 *         Pointer in RAM to store read data.
 * \param  from
 *         Address in flash to read data from.
 * \param  amount
 *         Number of bytes to read.
 * \return Result code, \ref EXTFLASH_RES_OK if successful.
 *         See \ref extFlash_res_e for other result codes.
 */
extFlash_res_e externalFlash_startRead(void * to, const void * from,
                                       size_t amount);
/**
 * \brief  Write bytes to flash.
 * \param  to
 *         Address in flash to write data to.
 * \param  from
 *         Pointer in RAM to the data to be written.
 * \param  amount
 *         Number of bytes to write.
 * \return Result code, \ref EXTFLASH_RES_OK if successful.
 *         See \ref extFlash_res_e for other result codes.
 */
extFlash_res_e externalFlash_startWrite(void * to, const void * from,
                                        size_t amount);

/**
 * \brief  Erase a sector of flash.
 * \param  sector_base
 *         pointer to the base address of the sector to be erased. If the flash
 *         driver can’t erase all requested sector, return the base address of
 *         the next sector to be erased.
 * \param  number_of_sector
 *         Pointer to number of sector to erase.
 *         Returns the number of remaining sector to erase.
 * \return Result code, \ref EXTFLASH_RES_OK if successful.
 *         See \ref extFlash_res_e for other result codes.
 */
extFlash_res_e externalFlash_startErase(size_t * sector_base,
                                        size_t * number_of_sector);

/**
 * \brief  Checks if flash driver is busy.
 * \return true: driver is busy, false otherwise.
 */
bool externalFlash_isBusy(void);

/**
 * \brief  Fills a structure with info about flash.
 * \param  info
 *         pointer to an \ref flash_info_t structure.
 * \return Result code, \ref EXTFLASH_RES_OK if successful.
 *         See \ref extFlash_res_e for other result codes.
 */
 extFlash_res_e externalFlash_getInfo(flash_info_t * info);

#endif //EXTERNAL_FLASH_H__
