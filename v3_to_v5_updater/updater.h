/* Copyright 2018 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

 #include <stddef.h>

/** \brief  Maximum number of memory areas */
#define NUM_MEMORY_AREAS    8

/**
 * \brief Offset from start of area for \ref bl_info_header_t, in bytes
 * \note  This must match the actual offset of \ref __saved_bl_info_header__
 *        in scratchpad_header.s
 */
#define BL_INFO_HEADER_OFFSET       16

/** Byte offset of \ref APP_V2_TAG from the start of application memory area */
#define APP_V2_TAG_OFFSET           48

/** Length of \ref APP_V2_TAG, in bytes */
#define APP_V2_TAG_LENGTH           16

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

/**
 * \brief   Application information header
 *
 * If an application is compiled to support application API v2, this header
 * can found in the beginning of the application memory area. It is placed
 * right after the \ref APP_V2_TAG, which is at \ref APP_V2_TAG_OFFSET.
 */
typedef struct
{
    /** Expected API version of application */
    uint32_t api_version;
    /** Expected start address of application memory area, for sanity checks */
    uint32_t start_address;
    /** Total number of bytes used in the application memory area */
    uint32_t length;
    /** Padding, reserved for future use, must be 0 */
    uint32_t pad;
} app_information_header_t;
