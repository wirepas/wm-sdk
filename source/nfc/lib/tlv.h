/* Copyright 2018 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/**
 * \file tlv.h
 *
 */

#ifndef __TLV_H__
#define __TLV_H__

/**
 * Structure to describe a TLV Message
 */
typedef struct
{
    uint16_t len;
    uint16_t offset;
} tlv_msg_t;

/** Return codes of TLV functions */
typedef enum
{
    TLV_RES_ERR = 0,       /* Not a TLV block */
    TLV_RES_BLOCK = 1,     /* TLV Block */
    TLV_RES_TERMINATOR = 2 /* TLV Terminator */
} tlv_res_e;

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
void tlv_encode(uint8_t *msg, uint16_t msgLength, uint8_t *tagData);

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
tlv_res_e tlv_decode(uint8_t *tagData, uint16_t tagDataLength, tlv_msg_t *tlv);

#endif /* __TLV_H__ */
