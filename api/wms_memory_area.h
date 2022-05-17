/* Copyright 2018 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/**
 * \file wms_memory_area.h
 *
 * This library gives read, write and erase access to User defined or
 * Application memory areas. The areas can be located in internal flash or on
 * an external flash chipset. See document WP-RM-131 - Wirepas Mesh bootloader
 * development guide for more information about bootloader external flash
 * support.
 *
 * Library services are accessed via @ref app_lib_memory_area_t
 * "lib_memory_area" handle.
 *
 */
#ifndef APP_LIB_MEMORY_AREA_H_
#define APP_LIB_MEMORY_AREA_H_

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "wms_app.h"

/** \brief Library symbolic name  */
#define APP_LIB_MEMORY_AREA_NAME 0x01fd3b49

/** \brief Maximum supported library version */
#define APP_LIB_MEMORY_AREA_VERSION 0x200

/** \brief  memory area operations result */
typedef enum
{
    /** Operation is successful */
    APP_LIB_MEM_AREA_RES_OK        = 0,
    /** Error during operation. Mostly bus communication (I2C/SPI) error, or
     * external flash chipset is not responding. */
    APP_LIB_MEM_AREA_RES_ERROR     = 1,
    /** Underneath flash driver is busy */
    APP_LIB_MEM_AREA_RES_BUSY      = 2,
    /** There is no external flash driver implemented in the bootloader. */
    APP_LIB_MEM_AREA_RES_NODRIVER  = 3,
    /** Invalid parameters */
    APP_LIB_MEM_AREA_RES_PARAM     = 4,
    /** Area doesn't exist */
    APP_LIB_MEM_AREA_RES_INVALID_AREA  = 5,
} app_lib_mem_area_res_e;

/**
 * \brief  Maximum number of areas that can defined in the bootloader.
 *
 * Used as a absolute maximum value for @ref app_lib_mem_area_id_t.
 */
#define APP_LIB_MEM_AREA_MAX_AREAS    8

/** \brief  Memory Area id definition */
typedef uint32_t app_lib_mem_area_id_t;

/** \brief  Lists the types of areas accessible from application side
 *
 * Used when querying @ref app_lib_mem_area_info_t.type "type" of the memory
 * area with service @ref app_lib_mem_area_getAreaInfo_f
 * "lib_memory_area->getAreaInfo()".
 */
typedef enum
{
    /** Application area */
    APP_LIB_MEM_AREA_TYPE_APPLICATION = 0,
    /** User defined area */
    APP_LIB_MEM_AREA_TYPE_USER        = 1,
} app_lib_mem_area_type_e;

/**
 * \brief   Information on flash peripheral
 *
 * The structure containing flash topology and timings returned by the
 * \ref app_lib_mem_area_getAreaInfo_f "lib_memory_area->getAreaInfo()"
 * function */
typedef struct
{
    /** Size of a write page */
    size_t write_page_size;
    /** Size of an erase sector */
    size_t erase_sector_size;
    /** Minimum write alignment supported by the flash hardware. Write address
     *  must be aligned on write_alignment and write amount must be a multiple
     *  of write_alignment bytes.
     */
    size_t write_alignment;
    /** Time taken by the Flash chipset to write one byte in uS. */
    /** \note For internal flash driver write (erase) time is 0. Due to hardware
     *  limitation the driver is fully synchronous so the write (erase) to flash
     *  is finished when \ref app_lib_mem_area_startWrite_f
     *  "lib_memory_area->startWrite()" (\ref app_lib_mem_area_startErase_f
     *  "lib_memory_area->startErase()") returns.
     */
    uint32_t byte_write_time;
    /** Time taken by the Flash chipset write one page in useconds */
    uint32_t page_write_time;
    /** Time taken by the Flash chipset to erase on sector in useconds */
    uint32_t sector_erase_time;
    /** Time taken by \ref app_lib_mem_area_startWrite_f
     * "lib_memory_area->startWrite()" call for one byte in useconds */
    uint32_t byte_write_call_time;
    /** Time taken by \ref app_lib_mem_area_startWrite_f
     * "lib_memory_area->startWrite()" call for one page in useconds */
    uint32_t page_write_call_time;
    /** Time taken by \ref app_lib_mem_area_startErase_f
     * "lib_memory_area->startErase()" call for one sector in useconds */
    uint32_t sector_erase_call_time;
    /** Time taken by \ref app_lib_mem_area_isBusy_f "lib_memory_area->isBusy()"
     * call in useconds */
    uint32_t is_busy_call_time;
} app_lib_mem_area_flash_info_t;

/**
 * @brief memory area information
 *
 * The structure containing memory area information returned by the \ref
 * app_lib_mem_area_getAreaInfo_f "lib_memory_area->getAreaInfo()" function
 */
typedef struct
{
    /** Id of the area */
    app_lib_mem_area_id_t area_id;
    /** Size in bytes of the area */
    size_t area_size;
    /** Description of the flash memory where the area is located */
    app_lib_mem_area_flash_info_t flash;
    /** true if area is located in external flash */
    bool external_flash;
    /** Type of memory area */
    app_lib_mem_area_type_e type;
} app_lib_mem_area_info_t;


/**
 * @brief  Block read from a memory area
 *
 * Reading is asynchronous and \ref app_lib_mem_area_isBusy_f
 * "lib_memory_area->isBusy()" must be checked for operation completion.
 *
 * \param  id
 *         Id of the memory area to read from.
 * \param  to
 *         Pointer in RAM to store read data.
 * \param  from
 *         Address in memory area to read data from.
 * \param  amount
 *         Number of bytes to read.
 * \return Result code, \ref APP_LIB_MEM_AREA_RES_OK if successful.
 *         See \ref app_lib_mem_area_res_e for other result codes.
 */
typedef app_lib_mem_area_res_e
    (*app_lib_mem_area_startRead_f)(app_lib_mem_area_id_t id,
                                    void * to,
                                    uint32_t from,
                                    size_t amount);

/**
 * @brief  Block write to a memory area.
 *
 * Writing to a flash memory takes time (5ms typical for a page). The call to
 * the function must be asynchronous, meaning that the write to the memory area
 * is accepted and will succeed but MAY not be finished. \ref
 * app_lib_mem_area_isBusy_f "lib_memory_area->isBusy()" must be used to check
 * if the write operation has terminated.
 *
 * \param  id
 *         Id of the memory area to write to.
 * \param  to
 *         Address in memory area to write data to.
 * \param  from
 *         Pointer in RAM to the data to be written.
 * \param  amount
 *         Number of bytes to write.
 * \note   Depending on flash chipset 1 byte write are not possible. Parameters
 *         \p to and \p amount must be aligned on write_alignement field of
 *         \ref app_lib_mem_area_flash_info_t.
 * \return Result code, \ref APP_LIB_MEM_AREA_RES_OK if successful.
 *         See \ref app_lib_mem_area_res_e for other result codes.
 */
typedef app_lib_mem_area_res_e
    (*app_lib_mem_area_startWrite_f)(app_lib_mem_area_id_t id,
                                     uint32_t to,
                                     const void * from,
                                     size_t amount);

/**
 * @brief   Erase one or multiple sectors in the memory area.
 *
 * Erasing is asynchronous and \ref app_lib_mem_area_isBusy_f
 * "lib_memory_area->isBusy()" must be checked for operation completion.
 *
 * \param  id
 *         Id of the memory area to erase to.
 * \param  sector_base
 *         pointer to the base address of the sector to be erased. If the flash
 *         driver cannot erase all requested sector, return the base address of
 *         the next sector to be erased.
 * \param  number_of_sector
 *         Pointer to number of sector to erase.
 *         Returns the number of remaining sector to erase.
 * \return Result code, \ref APP_LIB_MEM_AREA_RES_OK if successful.
 *         See \ref app_lib_mem_area_res_e for other result codes.
 */
typedef app_lib_mem_area_res_e
    (*app_lib_mem_area_startErase_f)(app_lib_mem_area_id_t id,
                                     uint32_t * sector_base,
                                     size_t * number_of_sector);

/**
 * \brief  Checks if underlying flash driver is busy.
 * \param  id
 *         Id of the memory area to check.
 * \return true: driver is busy, false otherwise.
 */
typedef bool
    (*app_lib_mem_area_isBusy_f)(app_lib_mem_area_id_t id);

/**
 * \brief  Fills a structure with info about memory area and flash timings and
 *         topology.
 * \param  id
 *         Id of the memory area to get info from.
 * \param  info
 *         pointer to an \ref app_lib_mem_area_info_t structure.
 * \return Result code, \ref APP_LIB_MEM_AREA_RES_OK if successful.
 *         See \ref app_lib_mem_area_res_e for other result codes.
 */
typedef app_lib_mem_area_res_e
    (*app_lib_mem_area_getAreaInfo_f)(app_lib_mem_area_id_t id,
                                      app_lib_mem_area_info_t * info);

/**
* \brief  Returns the list of areas defined in the bootloader and accessible
*         from the application.
* \param  list
*         pointer to an array of areas.
* \param  num_areas
*         In: size of the array list. Out: number of defined areas.
* \return none
*/
typedef void
  (*app_lib_mem_area_getAreaList_f)(app_lib_mem_area_id_t * list,
                                    uint8_t * num_areas);

/**
 * \brief       List of library functions
 */
 typedef struct
 {
     app_lib_mem_area_startRead_f      startRead;
     app_lib_mem_area_startWrite_f     startWrite;
     app_lib_mem_area_startErase_f     startErase;
     app_lib_mem_area_isBusy_f         isBusy;
     app_lib_mem_area_getAreaInfo_f    getAreaInfo;
     app_lib_mem_area_getAreaList_f    getAreaList;
} app_lib_memory_area_t;

#endif /* APP_LIB_MEMORY_AREA_H_ */
