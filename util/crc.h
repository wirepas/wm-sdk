/* Copyright 2017 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

#ifndef CRC_H
#define CRC_H

#include <stdint.h>

/**
 * \file crc.h
 *
 * This implementation follows the "bad CRC16-CCITT" algorithm,
 * as described in http://srecord.sourceforge.net/crc16-ccitt.html.
 */

/* Select only one of the alternative implementations below: */

/** Fast LUT based algorithm. */
#define CRC_CCITT_FAST_LUT

/**< Original CRC algorithm, ported 1:1 from PIC18F */
//#define CRC_CCITT_LEGACY

typedef union
{
    struct
    {
        uint8_t lsb;
        uint8_t msb;
    };
    uint16_t crc;
} crc_t;

/**
 * Startup value for CRC result
 */
#define Crc_initValue() 0xffff

/**
 * Add byte to CRC result
 * \param crc   Cumulative result of CRC calculation
 * \param byte  The byte to add to the result
 * \note        90.1 us for 50 bytes -> 1.80 us per byte for CRC_CCITT_FAST_LUT
 * \note        96.4 us for 50 bytes -> 1.93 us per byte for CRC_CCITT_LEGACY
 */
uint16_t Crc_addByte(uint16_t crc, uint8_t byte);

/**
 * Calculate CRC over a buffer
 * \param buf   Pointer to a buffer
 * \param len   Length of the buffer in bytes
 * \note        63.8 us for 50 bytes -> 1.28 us per byte for CRC_CCITT_FAST_LUT
 * \note        92.1 us for 50 bytes -> 1.84 us per byte for CRC_CCITT_LEGACY
 */
uint16_t Crc_fromBuffer(const uint8_t * buf, uint32_t len);

/**
 * Calculate CRC over a buffer
 * \param buf   Pointer to a buffer
 * \param len   Length of the buffer in words
 * \note        Approximately 65% execution time compared to uint8_t version
 *              when calculating checksums from ROM contents
 */
uint16_t Crc_fromBuffer32(const uint32_t * buf, uint32_t len);

#endif
