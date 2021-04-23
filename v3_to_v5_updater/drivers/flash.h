/* Copyright 2018 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

#ifndef FLASH_H_
#define FLASH_H_

#include <stddef.h>
#include <stdbool.h>

// Page size of system flash from linker script
extern uint32_t                     __flash_page_size_bytes__;

// Macros for flash access
#define FLASH_PAGE_SIZE_BYTES       ((uint32_t)&__flash_page_size_bytes__)
#define FLASH_PAGE_MASK             (0xFFFFFFFF - (FLASH_PAGE_SIZE_BYTES - 1))
#define Flash_getPage(addr)         ((addr) & FLASH_PAGE_MASK)

// Macro for checking alignment / amount operands.
// Parameter should be unsigned
// This avoids using a potentially expensive modulo (division) operand
#define Flash_isAligned(param)      \
    (((uint32_t)(param) & (sizeof(uint32_t) - 1)) == 0)

/**
 * \brief   To initialize flash HW if required by platform.
 * \return  none
 */
void Flash_init(void);

/**
 * \brief   Enable writing to flash.
 *          (In NRF5x required also when writing to UICR/FICR registers)
 * \return  none
 */
void Flash_enableWrite(void);

/**
 * \brief   Enable flash erase.
 *          (In NRF5x required also when writing to UICR/FICR registers)
 * \return  none
 */
void Flash_enableErase(void);

/**
 * \brief   Disables write and erase access of the flash.
 * \return  none
 */
void Flash_disableAccess(void);

/**
 * \brief   Erase one flash page (low level).
 *          Erasing must be manually enabled/disabled.
 *          This primitive is not protected by critical sections.
 * \param   page
 *          Starting address of the flash page.
 * \return  none
 */
void Flash_erase(size_t page);

/**
 * \brief   Erase one flash page (high level).
 *          Handles flash erase enable/disable.
 *          This primitive is protected by critical sections.
 * \param   page_base
 *          Starting address of the flash page.
 * \return  none
 */
void Flash_erasePage(size_t page_base);

/**
 * \brief   Write data to flash.
 *          Handles write enable/disable
 * \param   to
 *          Flash address to write to
 * \param   from
 *          Address to read from
 * \param   amount
 *          Bytes to write. Full words i.e. multiple of 4 bytes.
 * \return  none
 */
void Flash_write(void * to, const void * from, size_t amount);

#endif /* FLASH_H_ */
