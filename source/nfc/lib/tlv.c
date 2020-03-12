/* Copyright 2018 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/**
 * \file tlv.c
 *
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "api.h"

#include "tlv.h"

/**
 * \brief   Encapsulate payload inside a TLV message
 * \param   msg
 *          Pointer to the payload
 * \param   tagData
 *          Pointer to the destination.
 *          (by exemple Type 2 Tag simulated user memory)
 * \param   msgLength
 *          Number of bytes in the payload
 */
void tlv_encode(uint8_t *msg, uint16_t msgLength, uint8_t *tagData)
{
    uint16_t offset = 0;

    tagData[offset++] = 0x03;

    if (msgLength < 255)
    {
        tagData[offset++] = (uint8_t)(msgLength);
    }
    else
    {
        tagData[offset++] = 0xFF;
        tagData[offset++] = (uint8_t)((msgLength >> 8) & 0x0FF);
        tagData[offset++] = (uint8_t)(msgLength & 0x0FF);
    }

    if (msg)
    {
        memcpy(&tagData[offset], msg, msgLength);
    }
    else
    {
        memset(&tagData[offset], 0x0, msgLength);
    }

    offset += msgLength;

    tagData[offset++] = (uint8_t)0xFE;
    tagData[offset++] = (uint8_t)0x00;
}

/**
 * \brief   Check a valid TLV block
 * \param   tlv
 *          Pointer to a structure that will return offset to
 *          tagData and len of a valid TLV block
 * \param   tagData
 *          Pointer to the Type 2 Tag simulated user memory.
 * \param   tagDataLength
 *          num of bytes until end of Type 2 Tag simulated  memory
 * \return  tlv_res_e
 */
tlv_res_e tlv_decode(uint8_t *tagData, uint16_t tagDataLength, tlv_msg_t *tlv)
{
    tlv_res_e ret = TLV_RES_ERR;
    uint8_t tag;

    uint16_t offset = 0;
    uint16_t len = 0;

    tag = tagData[offset++];
    len = tagData[offset++] & 0xFF;

    if (len == 0xFF)
    {
        len = ((tagData[offset++] & 0xFF) << 8);
        len |= (tagData[offset++] & 0xFF);
    }

    if (tag == (uint8_t)0x03) /* NDEF Message */
    {
         tlv->len = len;
         tlv->offset = offset;
         ret = TLV_RES_BLOCK;
    }
    else if (tag == (uint8_t)0xFE) /* Terminator */
    {
        tlv->len = 0;
        tlv->offset = offset;
        ret = TLV_RES_TERMINATOR;
    }
    return ret;
}
