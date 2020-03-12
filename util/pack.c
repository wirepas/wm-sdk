/* Copyright 2019 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

#include "pack.h"

uint32_t Pack_unpackLe(const uint8_t * bytes, size_t num_bytes)
{
    const uint8_t * p = bytes + num_bytes;
    uint32_t        result = 0;

    for (; num_bytes > 0; num_bytes--)
    {
        result <<= 8;
        result |= *(--p);   // Little endian
    }

    return result;
}

void Pack_packLe(void * bytes, uint32_t value, size_t num_bytes)
{
    uint8_t * p = (uint8_t *)bytes;

    for (; num_bytes > 0; num_bytes--)
    {
        *(p++) = (uint8_t)(value & 0xff);   // Little endian
        value >>= 8;
    }
}
