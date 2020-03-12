/* Copyright 2019 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/**
 *
 * \file pack.h
 *
 * Bytes to native integers packing and unpacking
 */

#ifndef PACK_H_
#define PACK_H_

#include <stdlib.h>
#include <stdint.h>

/**
 * \brief   Convert 1 to 4 little endian bytes to a native unsigned integer
 * \param   bytes
 *          Pointer to bytes to unpack
 * \param   num_bytes
 *          Number of bytes to unpack, up to 4
 * \returns Unpacked little endian value
 */
uint32_t Pack_unpackLe(const uint8_t * bytes, size_t num_bytes);
/**
 * \brief   Convert native unsigned integer to 1 to 4 little endian bytes
 * \param   bytes
 *          Pointer to bytes to pack
 * \param   value
 *          Value to pack
 * \param   num_bytes
 *          Number of bytes to pack, up to 4
 */
void Pack_packLe(void * bytes, uint32_t value, size_t num_bytes);

#endif // PACK_H_
