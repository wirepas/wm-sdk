/* Copyright 2020 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

 #include <stddef.h>

/** \brief  Maximum number of memory areas */
//#define NUM_MEMORY_AREAS    8

/**
 * \brief Offset from start of area for \ref bl_info_header_t, in bytes
 * \note  This must match the actual offset of \ref __saved_bl_info_header__
 *        in scratchpad_header.s
 */
#define BL_INFO_HEADER_OFFSET       16

/**
 * \brief   Memory area information in .blconfig section
 */
typedef struct
{
    /** Start address of memory area */
    uint32_t address;
    /** Number of bytes in memory area */
    uint32_t length;
    /** ID of memory area */
    uint32_t id;
    /** Flags */
    uint32_t flags;
} memory_area_t;

/**
 * \brief  Bootloader information header
 *
 * Bootloader can be requested to store this header in an empty area in the
 * beginning of the firmware, at \ref BL_INFO_HEADER_OFFSET.
 */
typedef struct
{
    /** Number of bytes of scratchpad, not including header and tag */
    uint32_t length;
    /** CRC16-CCITT of scratchpad, not including any header or tag bytes */
    uint16_t crc;
    /** Sequence number of data in scratchpad: \ref BL_SCRATCHPAD_MIN_SEQ .. \ref BL_SCRATCHPAD_MAX_SEQ */
    uint8_t seq;
    /** Flags are used by the stack to determine if the scratchpad must be
     *  taken into use if the connection to the sink is lost for more than
     *  one hour (MANUAL or AUTO_PROCESS). These flags are don't care for
     *  the bootloader and are not defined here.
     */
    uint8_t flags;
    /** Memory area ID of firmware */
    uint32_t id;
    /** Firmware major version number component */
    uint8_t major;
    /** Firmware minor version number component */
    uint8_t minor;
    /** Firmware maintenance version number component */
    uint8_t maint;
    /** Firmware development version number component */
    uint8_t devel;
    /** Size of the data written in the area after decrompression */
    uint32_t written_size;
} bl_info_header_t;
