/**
    @file  tlv.c
    @brief Library to encode/decode buffers in TLV (Type Length Value) format.
    @copyright  Wirepas Oy 2019
*/
#include "tlv.h"
#include <string.h>
#include <stddef.h>

void Tlv_init(tlv_record * rcd, uint8_t * buffer, uint8_t length)
{
    rcd->buffer = buffer;
    rcd->length = length;
    rcd->index = 0;
}

tlv_res_e Tlv_Decode_getNextItem(tlv_record * rcd, tlv_item ** item)
{
    *item = (tlv_item*)&rcd->buffer[rcd->index];

    if (rcd->index == rcd->length)
    {
        /* End of buffer. */
        *item = NULL;
        return TLV_RES_END;
    }
    else if ((*item)->length > 0 &&
             ((*item)->length + rcd->index + offsetof(tlv_item,value)) <=
                                                                    rcd->length)
    {
        /* Valid item found. */
        rcd->index += (*item)->length + offsetof(tlv_item,value);
        return TLV_RES_OK;
    }
    else
    {
        /* Invalid item. */
        *item = NULL;
        return TLV_RES_ERROR;
    }
}

tlv_res_e Tlv_Encode_addItem(tlv_record * rcd, uint8_t type,
                                               uint8_t length,
                                               void * value)
{
    tlv_res_e ret = TLV_RES_ERROR;

    if (length > 0 &&
        (length + rcd->index + offsetof(tlv_item,value)) <= rcd->length)
    {
        rcd->buffer[rcd->index] = type;
        rcd->buffer[rcd->index + 1] = length;
        memcpy(&rcd->buffer[rcd->index + offsetof(tlv_item,value)],
               value,
               length);
        rcd->index += offsetof(tlv_item,value) + length;
        ret = TLV_RES_OK;
    }

    return ret;
}

int Tlv_Encode_getBufferSize(tlv_record * rcd)
{
    return rcd->index;
}
