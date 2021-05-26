/* Copyright 2018 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/**
 * \file bl_interface.h
 *
 *  Interface for interacting with bootloader
 */

#ifndef BL_INTERFACE_H_
#define BL_INTERFACE_H_

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

/** \brief  Bootloader interface operations result */
typedef enum
{
    BL_RES_OK            = 0, /** Operation is successful */
    BL_RES_ERROR         = 1, /** Error during operation */
    BL_RES_BUSY          = 2, /** Underneath flash driver is busy */
    BL_RES_NODRIVER      = 3, /** There is no external flash driver */
    BL_RES_PARAM         = 4, /** Invalid parameters */
    BL_RES_INVALID_AREA  = 5, /** Area doesn't exists */
    BL_RES_TIMEOUT       = 6, /** Synchronous call timeout */
    BL_RES_INVALID_STATE = 7, /** Write ongoing or scratchpad not valid */
} bl_interface_res_e;

/** \brief  Maximum number of memory areas */
#define BL_MEMORY_AREA_MAX_AREAS    8

/** \brief  Value of an area that doesn't exists */
#define BL_MEMORY_AREA_UNDEFINED 0xFFFFFFFF

/** \brief  Memory Area id definition */
typedef uint32_t bl_memory_area_id_t;

/** \brief  Types of Memory Areas */
typedef enum
{
    BL_MEM_AREA_TYPE_BOOTLOADER  = 0,  /** Bootloader area */
    BL_MEM_AREA_TYPE_STACK       = 1,  /** Stack area */
    BL_MEM_AREA_TYPE_APPLICATION = 2,  /** Application area */
    BL_MEM_AREA_TYPE_PERSISTENT  = 3,  /** Persistent memory area */
    BL_MEM_AREA_TYPE_SCRATCHPAD  = 4,  /** Dedicated scratchpad area */
    BL_MEM_AREA_TYPE_USER        = 5   /** User defined area */
} bl_memory_area_type_e;

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
    /** Time taken by the Flash chipset to write one byte in uS. */
    /** \note For internal Flash driver write (erase) time is 0. Duz to hardware
     *  limitation the driver is fully synchronous so the write (erase) to Flash
     *  is finished when startWrite (startErase) returns.
     */
    uint32_t byte_write_time;
    /** Time taken by the flash chipset write one page in uS */
    uint32_t page_write_time;
    /** Time taken by the flash chipset to erase on sector in uS */
    uint32_t sector_erase_time;
    /** Time taken by startWrite() call for one byte in uS */
    uint32_t byte_write_call_time;
    /** Time taken by startWrite() call for one page in uS */
    uint32_t page_write_call_time;
    /** Time taken by startErase() call for one sector in uS */
    uint32_t sector_erase_call_time;
    /** Time taken by isBusy() call in uS */
    uint32_t is_busy_call_time;
} bl_flash_info_t;

/** \brief  Memory Area info definition */
typedef struct
{
    /** Id of the area */
    bl_memory_area_id_t area_id;
    /** Size in bytes of the area */
    size_t area_size;
    /** Area physical address in flash */
    uint32_t area_physical_address;
    /** Description of the flash memory where the area is located */
    bl_flash_info_t flash;
    /** true if area is located in external flash */
    bool  external_flash;
    /** true if bl_info_header_t is stored at the beginning of area */
    bool  has_header;
    /** Type of memory area */
    bl_memory_area_type_e type;
} bl_memory_area_info_t;

/**
 * \brief  Informations contained in the header that the bootloader can store
 * in the beginning of an area (only used by stack and application for now).
 */
typedef struct
{
    /** Number of bytes of scratchpad, not including header and tag */
    uint32_t length;
    /** CRC16-CCITT of scratchpad, not including any header or tag bytes */
    uint16_t crc;
    /** Sequence number of data in scratchpad: \ref BL_SCRATCHPAD_MIN_SEQ .. \ref BL_SCRATCHPAD_MAX_SEQ */
    uint8_t seq;
    /** Firmware major version number component */
    uint8_t major;
    /** Firmware minor version number component */
    uint8_t minor;
    /** Firmware maintenance version number component */
    uint8_t maint;
    /** Firmware development version number component */
    uint8_t devel;
} bl_memory_area_header_t;

/** \brief Scratchpad type enum */
typedef enum
{
    /** No valid scratchpad stored */
    BL_SCRAT_TYPE_BLANK = 0,
    /** Valid scratchpad stored */
    BL_SCRAT_TYPE_PRESENT = 1,
    /** Valid scratchpad stored and marked to be processed by the bootloader */
    BL_SCRAT_TYPE_PROCESS = 2,
} bl_scrat_type_e;

/** \brief Scratchpad write status */
typedef enum
{
    /** Write completed successfully */
    BL_SCRAT_WRITE_STATUS_OK = 0,
    /** All data received and CRC is correct */
    BL_SCRAT_WRITE_STATUS_COMPLETED_OK = 1,
    /** All data received but CRC is incorrect */
    BL_SCRAT_WRITE_STATUS_COMPLETED_ERROR = 2,
    /** No writes have been started */
    BL_SCRAT_WRITE_STATUS_NOT_ONGOING = 3,
    /** Byte offset is invalid */
    BL_SCRAT_WRITE_STATUS_INVALID_START = 4,
    /** Number of bytes is invalid */
    BL_SCRAT_WRITE_STATUS_INVALID_NUM_BYTES = 5,
    /** Header does not seem to be correct */
    BL_SCRAT_WRITE_STATUS_INVALID_HEADER = 6,
    /** Bytes is NULL */
    BL_SCRAT_WRITE_STATUS_INVALID_NULL_BYTES = 7,
    /** Flash driver returned an error */
    BL_SCRAT_WRITE_STATUS_FLASH_ERROR = 8,
} bl_scrat_write_status_e;

/** \brief Scratchpad validity */
typedef enum
{
    /** Scratchpad validity has not been determined, yet */
    BL_SCRAT_IS_UNKNOWN,
    /** Scratchpad memory has been cleared, completely */
    BL_SCRAT_IS_CLEAR,
    /** Scratchpad tag not found */
    BL_SCRAT_IS_NOTAG,
    /** Scratchpad found but header is not valid */
    BL_SCRAT_IS_INVALID_HEADER,
    /** Scratchpad found but CRC doesn't match */
    BL_SCRAT_IS_INVALID_CRC,
    /** default error code, write ongoing, flash read error */
    BL_SCRAT_IS_INVALID,
    /** Scratchpad header present and CRC is correct */
    BL_SCRAT_IS_VALID,
} bl_scrat_valid_e;

/** \brief  Scratchpad info definition */
typedef struct
{
    /** Maximum possible length for the scratchpad (including header) */
    uint32_t area_length;
    /** theorical erase time of the whole scratchpad area */
    uint32_t erase_time;
    /** Number of bytes (including header) */
    uint32_t length;
    /** CRC16-CCITT, not including any header bytes */
    uint16_t crc;
    /** Sequence number of data in scratchpad: \ref bl_scratchpad_seq_t,
     *  we don't want dependancies with the bootloader here.
     *  \ref BL_SCRATCHPAD_MIN_SEQ .. \ref BL_SCRATCHPAD_MAX_SEQ */
    uint8_t seq;
    /** Flags used by the stack to determine if the scratchpad must be taken
     *  into use if the connection to the sink is lost for more than one hour.
     */
    uint8_t flags;
    /** Scratchpad type information for bootloader: \ref bl_header_type_e */
    uint32_t type;
    /** Status code from bootloader: \ref bl_header_status_e */
    uint32_t status;
    /** true if scratchpad has is own dedicated area */
    bool dedicated;
} bl_scrat_info_t;

/** \brief  Hardware features that can be installed on a board. */
typedef struct
{
    /** True if 32kHz crystal is present; default:true
     *  (introduced in booloader v7).
     */
    bool crystal_32k;
    /** True if DCDC converter is enabled; default:true
     * (introduced in bootloader v7).
     */
    bool dcdc;
} bl_hardware_capabilities_t;

/**
 * \brief  Read bytes from a memory area.
 * \param  id
 *         Id of the memory area to read from.
 * \param  to
 *         Pointer in RAM to store read data.
 * \param  from
 *         Address in memory area to read data from.
 * \param  amount
 *         Number of bytes to read.
 * \return Result code, \ref BL_RES_OK if successful.
 *         See \ref bl_interface_res_e for other result codes.
 */
typedef bl_interface_res_e
    (*bl_memory_area_startRead_f)(bl_memory_area_id_t id,
                                  void * to,
                                  uint32_t from,
                                  size_t amount);

/**
 * \brief  Write bytes to a memory area.
 * \param  id
 *         Id of the memory area to write to.
 * \param  to
 *         Address in memory area to write data to.
 * \param  from
 *         Pointer in RAM to the data to be written.
 * \param  amount
 *         Number of bytes to write.
 * \note   Writes to internal flash memory must be 4 bytes aligned.
 * \return Result code, \ref BL_RES_OK if successful.
 *         See \ref bl_interface_res_e for other result codes.
 */
typedef bl_interface_res_e
    (*bl_memory_area_startWrite_f)(bl_memory_area_id_t id,
                                   uint32_t to,
                                   const void * from,
                                   size_t amount);

/**
 * \brief  Erase a sector of a memory area.
 * \param  id
 *         Id of the memory area to erase to.
 * \param  sector_base
 *         pointer to the base address of the sector to be erased. If the flash
 *         driver cannot erase all requested sector, return the base address of
 *         the next sector to be erased.
 * \param  number_of_sector
 *         Pointer to number of sector to erase.
 *         Returns the number of remaining sector to erase.
 * \return Result code, \ref BL_RES_OK if successful.
 *         See \ref bl_interface_res_e for other result codes.
 */
typedef bl_interface_res_e
    (*bl_memory_area_startErase_f)(bl_memory_area_id_t id,
                                   uint32_t * sector_base,
                                   size_t * number_of_sector);

/**
 * \brief  Checks if underlying flash driver is busy.
 * \param  id
 *         Id of the memory area to check.
 * \return true: driver is busy, false otherwise.
 */
typedef bool
    (*bl_memory_area_isBusy_f)(bl_memory_area_id_t id);

/**
 * \brief  Fills a structure with info about memory area.
 * \param  id
 *         Id of the memory area to get info from.
 * \param  info
 *         pointer to an \ref memoryArea_info_t structure.
 * \return Result code, \ref BL_RES_OK if successful.
 *         See \ref bl_interface_res_e for other result codes.
 */
typedef bl_interface_res_e
    (*bl_memory_area_getAreaInfo_f)(bl_memory_area_id_t id,
                                    bl_memory_area_info_t * info);

/**
 * \brief  Return the list of areas defined in the bootloader.
 * \param  list
 *         pointer to an array of areas.
 * \param  num_areas
 *         In: size of the array list. Out: number of defined areas.
 * \return none
 */
typedef void
    (*bl_memory_area_getAreaList_f)(bl_memory_area_id_t * list,
                                    uint8_t * num_areas);

/**
 * \brief  Return the memory area id of the FIRST area with provided type.
 * \note   If there is more than one area with the same type. The first one is
 *         always returned.
 * \param  id
 *         Id of the area if found, \ref BL_MEMORY_AREA_UNDEFINED otherwise.
 * \param  type
 *         Type of the area area to find.
 * \return Result code, \ref BL_RES_OK if successful.
 *         See \ref bl_interface_res_e for other result codes.
 */
typedef bl_interface_res_e
    (*bl_memory_area_getIdfromType_f)(bl_memory_area_id_t * id,
                                      bl_memory_area_type_e type);

/**
 * \brief  Return a structure containing the information stored in the header
 *         at the beginning of the area.
 * \param  id
 *         Id of the memory area to get info from.
 * \param  header
 *         pointer to an \ref bl_memory_area_header_t structure.
 * \return Result code, \ref BL_RES_OK if successful. \ref BL_RES_ERROR if the
 *         area has no stored or invalid header. See \ref bl_interface_res_e
 *         for other result codes.
 */
typedef bl_interface_res_e
   (*bl_memory_area_getAreaHeader_f)(bl_memory_area_id_t id,
                                     bl_memory_area_header_t * header);

/**
 * \brief  Check if the scratchpad contains valid data
 * \note   Valid data isn't necessarily a firmware image
 * \param  validity
 *         Filled with scratchpad validity code.
 * \return Result code, \ref BL_RES_OK if validity field has
 *         successfully been updated (and not necessarily BL_SCRAT_IS_VALID).
 *         See \ref bl_interface_res_e for other result codes.
 */
typedef bl_interface_res_e
    (*bl_scrat_getValidity_f)(bl_scrat_valid_e * validity);

/**
 * \brief  Read bytes from a scratchpad file.
 * \param  to
 *         Pointer in RAM to store read data.
 * \param  from
 *         Address in scratchpad file to read data from.
 * \param  amount
 *         Number of bytes to read.
 * \return Result code, \ref BL_RES_OK if successful.
 *         Returns \ref BL_RES_ERROR if scratchpad is invalid.
 *         See \ref bl_interface_res_e for other result codes.
 */
typedef bl_interface_res_e
    (*bl_scrat_read_f)(void * to, uint32_t from, size_t amount);

/**
 * \brief  Erase the scratchpad area.
 * \return Result code, \ref BL_RES_OK if successful.
 *         See \ref bl_interface_res_e for other result codes.
 */
typedef bl_interface_res_e (*bl_scrat_clear_f)(void (*clear_wdt)(void));

/**
 * \brief   Prepare scratchpad memory for storing new data
 * \param   num_bytes
 *          Total number of bytes to write
 * \param   seq
 *          Scratchpad sequence number
 * \return  Result code, \ref BL_RES_OK if successful.
 *          See \ref bl_interface_res_e for other result codes.
 * \note    Scratchpad memory is implicitly cleared if this call succeeds
 */
typedef bl_interface_res_e
    (*bl_scrat_begin_f)(uint32_t num_bytes,
                        uint8_t seq,
                        void (*clear_wdt)(void));

/**
 * \brief  Write bytes to scratchpad area.
 * \param  to
 *         Address in scratchpad to write data to.
 * \param  from
 *         Pointer in RAM to the data to be written.
 * \param  amount
 *         Number of bytes to write.
 * \note   Writes must be 4 bytes aligned.
 * \param  status
 *         The result of the write operation state machine.
 * \return Result code, \ref BL_RES_OK if successful.
 *         See \ref bl_interface_res_e for other result codes.
 */
typedef bl_interface_res_e
    (*bl_scrat_write_f)(uint32_t to,
                        const void * from,
                        size_t amount,
                        bl_scrat_write_status_e * status);

/**
 * \brief  Fills a structure with info about the scratchpad.
 * \param  info
 *         pointer to an \ref scratchpad_info_t structure.
 * \note   doesn't check if header/scratchpad is valid. some data may be false.
 * \return Result code, \ref BL_RES_OK if successful.
 *         See \ref bl_interface_res_e for other result codes.
 */
typedef bl_interface_res_e
    (*bl_scrat_getInfo_f)(bl_scrat_info_t * info);

/**
 * \brief   Set scratchpad as bootable
 * \note    Does nothing if scratchpad was already set to be bootable
 * \return  Result code, \ref BL_RES_OK if successful.
 *          See \ref bl_interface_res_e for other result codes.
 */
typedef bl_interface_res_e
    (*bl_scrat_setBootable_f)(void);

/**
 * \brief   Returns board hardware capabilities.
 * \return  Return a bit field \ref bl_hardware_capabilities_e with
 *          hardware features installed on the board.
 */
typedef const bl_hardware_capabilities_t * (*bl_hardware_getCapabilities_f)(void);

typedef struct
{
    bl_memory_area_startRead_f      startRead;
    bl_memory_area_startWrite_f     startWrite;
    bl_memory_area_startErase_f     startErase;
    bl_memory_area_isBusy_f         isBusy;
    bl_memory_area_getAreaInfo_f    getAreaInfo;
    bl_memory_area_getAreaList_f    getAreaList;
    bl_memory_area_getIdfromType_f  getIdfromType;
    bl_memory_area_getAreaHeader_f  getAreaHeader;
} memory_area_services_t;

typedef struct
{
    bl_scrat_getValidity_f          getValidity;
    bl_scrat_read_f                 read;
    bl_scrat_clear_f                clear;
    bl_scrat_begin_f                begin;
    bl_scrat_write_f                write;
    bl_scrat_getInfo_f              getInfo;
    bl_scrat_setBootable_f          setBootable;
} scratchpad_services_t;

typedef struct
{
    bl_hardware_getCapabilities_f   getCapabilities;
} hardware_services_t;

/**
 * \brief   Global interface entry point with a version id
 */
typedef struct
{
    uint32_t version;
    const memory_area_services_t * memory_area_services_p;
    const scratchpad_services_t * scratchpad_services_p;
    const hardware_services_t * hardware_services_p;
} bl_interface_t;

#endif /* BL_INTERFACE_H_ */
