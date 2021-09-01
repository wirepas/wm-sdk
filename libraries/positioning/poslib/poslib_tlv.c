/**
 * @file       poslib_tlv.c
 * @brief      PosLib specific TLV decoding.
 *             In the new PosLib the configuration command is encoded as
 *             a compact TLV where the fields Type & Length are combined into
 *             a single byte (called TL byte) as:
 *             Length:
 *             -will be encoded on the 3 bit MSB of the TL byte
 *             -value 0 indicates that the actual length will be on the next
 *             byte i.e. same with old format
 *             -value 1...7 indicates the number of bytes expected in the value
 *             Type:
 *             will be encoded into the 5 LSB bits of the TL byte with allowed
 *             values from 1...31. (0 is reserved)
 * @copyright  Wirepas Ltd 2021
 */

#define DEBUG_LOG_MODULE_NAME "POSLIB TLV"
#ifdef DEBUG_POSLIB_LOG_MAX_LEVEL
#define DEBUG_LOG_MAX_LEVEL DEBUG_POSLIB_LOG_MAX_LEVEL
#else
#define DEBUG_LOG_MAX_LEVEL LVL_NOLOG
#endif
#include "debug_log.h"
#include <string.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include "poslib_tlv.h"

/* Minimal size for a TLV on wire, a short type + len + at least one value byte */
#define MINIMAL_POSLIB_TLV_SIZE             2

void PosLibTlv_init(poslib_tlv_record * rcd, uint8_t * buffer, uint8_t length)
{
    rcd->buffer = buffer;
    rcd->length = length;
    rcd->index = 0;
}

poslib_tlv_res_e PosLibTlv_Decode_getNextItem(poslib_tlv_record * rcd,
                                              poslib_tlv_item_t * item)
{
    uint8_t type;
    uint8_t length;

    if (rcd->index >= rcd->length)
    {
         return POSLIB_TLV_RES_END;
    }

    /** Check length first */
    if (rcd->length - rcd->index < MINIMAL_POSLIB_TLV_SIZE)
    {
         LOG(LVL_ERROR, "TLV_RES_ERROR: bytes left %d",  rcd->length - rcd->index);
         return POSLIB_TLV_RES_ERROR;
    }

    type = rcd->buffer[rcd->index];

    /** If type 3 MSB bits are zero length is encoded on the next byte*/
    if ((type & 0xE0) == 0)
    {
         rcd->index += 1;
         length = rcd->buffer[rcd->index];
    }
    else
    {
        /** length encoded on the 3 MSB bits */
        length = type >> 5;
        /** type encoded on the 5 LSB bits */
        type  &= 0x1F;
    }

    if (rcd->length - rcd->index >= length)
    {
         item->type = type;
         item->length = length;
         rcd->index += 1;
         item->value = &rcd->buffer[rcd->index];
         rcd->index += length;
         return POSLIB_TLV_RES_OK;
    }
    else
    {
         LOG(LVL_ERROR, "TLV_RES_ERROR: length %d", length);
         rcd->index = rcd->length;
         return POSLIB_TLV_RES_ERROR;
    }
}

poslib_tlv_res_e PosLibTlv_Encode_addItem(poslib_tlv_record * rcd,
                                             poslib_tlv_item_t * item)
{
     uint8_t bytes_left;
     
     if (rcd->index >= rcd->length || item->length == 0 || 
          rcd->buffer == NULL || item->value == NULL || item->type > 31)
     {
          LOG(LVL_ERROR, "Invalid input parameters");
          return POSLIB_TLV_RES_ERROR;
     }
     
     bytes_left = rcd->length - rcd->index;

     // for value smaller than 8 bytes type & length are encoded into one byte
     if (item->length <=7 && bytes_left >= (item->length + 1))
     {
          uint8_t tl = 0;
          tl = (item->length << 5) | (item->type & 0x1F);
          rcd->buffer[rcd->index++] = tl;
          memcpy(&rcd->buffer[rcd->index], item->value, item->length); 
          rcd->index += item->length;
     } 
      // for value larger than 7 bytes type & length are encoded on separate bytes
     else if (item->length > 7 && bytes_left >= (item->length + 2)) 
     {
          rcd->buffer[rcd->index++] = item->type & 0x1F;
          rcd->buffer[rcd->index++] = item->length;
          memcpy(&rcd->buffer[rcd->index], item->value, item->length); 
          rcd->index += item->length;
     }
     else
     {
          LOG(LVL_ERROR, "Cannot encode. type: %u, length: %u", item->type, item->length);
          return POSLIB_TLV_RES_ERROR;
     }
      return POSLIB_TLV_RES_OK;
} 