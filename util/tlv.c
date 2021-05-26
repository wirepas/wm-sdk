/**
    @file  tlv.c
    @brief Library to encode/decode buffers in TLV (Type Length Value) format.
    @copyright  Wirepas Oy 2019
*/
#include "tlv.h"
#include <string.h>
#include <stddef.h>
#include <stdbool.h>

/* Minimal size for a TLV on wire, a short type + len + at least one value byte */
#define MINIMAL_TLV_SIZE 3

/* Define size for extended headers and short header */
#define SIZE_EXTENDED_HEADER  3
#define SIZE_SHORT_HEADER   2

void Tlv_init(tlv_record * rcd, uint8_t * buffer, uint8_t length)
{
    rcd->buffer = buffer;
    rcd->length = length;
    rcd->index = 0;
}

tlv_res_e Tlv_Decode_getNextItem(tlv_record * rcd, tlv_item_t * item)
{
    uint8_t remaining_bytes;
    uint16_t type;
    uint8_t length;
    bool type_extended = false;

    remaining_bytes = rcd->length - rcd->index;
    if (remaining_bytes == 0)
    {
        /* End of buffer. */
        return TLV_RES_END;
    }

    /* Check length first */
    if (remaining_bytes < MINIMAL_TLV_SIZE)
    {
        /* 3 bytes needed at least*/
        return TLV_RES_ERROR;
    }

    /* Decode it */
    /* Load lower part of type */
    type = rcd->buffer[rcd->index] & 0xFF;
    /* Load length */
    length = rcd->buffer[rcd->index + 1] & 0x7f;
    /* Check if extended type */
    type_extended = ((rcd->buffer[rcd->index + 1] & 0x80) != 0);

    /* Move the index */
    rcd->index += 2;

    /* if it is an extended type, load the high part */
    if (type_extended)
    {
        type +=  (rcd->buffer[rcd->index] & 0xFF) << 8;
        rcd->index += 1;
    }

    /* Check if the remaining bytes is coherent */
    remaining_bytes = rcd->length - rcd->index;
    if (remaining_bytes < length)
    {
        /* Not enough bytes remaining */
        return TLV_RES_ERROR;
    }

    item->type = type;
    item->length = length;
    item->value = &rcd->buffer[rcd->index];

    rcd->index += length;

    return TLV_RES_OK;
}

tlv_res_e Tlv_Encode_addItem(tlv_record * rcd, tlv_item_t * item)
{
    uint8_t remaining_bytes = rcd->length - rcd->index;

    if (item->length > 127)
    {
        return TLV_RES_ERROR;
    }

    /* Check the size of type */
    if (item->type > UINT8_MAX)
    {
        if (remaining_bytes < SIZE_EXTENDED_HEADER + item->length)
        {
            return TLV_RES_ERROR;
        }
        rcd->buffer[rcd->index] = item->type & 0xff;
        rcd->buffer[rcd->index + 1] = 0x80 | item->length;
        rcd->buffer[rcd->index + 2] = (item->type >> 8) & 0xff;
        rcd->index += SIZE_EXTENDED_HEADER;
    }
    else
    {
        if (remaining_bytes < SIZE_SHORT_HEADER + item->length)
        {
            return TLV_RES_ERROR;
        }
        rcd->buffer[rcd->index] = item->type & 0xff;
        rcd->buffer[rcd->index + 1] = item->length;
        rcd->index += SIZE_SHORT_HEADER;
    }

    /* Copy value*/
    memcpy(&rcd->buffer[rcd->index], item->value, item->length);
    rcd->index += item->length;

    return TLV_RES_OK;
}

int Tlv_Encode_getBufferSize(tlv_record * rcd)
{
    return rcd->index;
}
