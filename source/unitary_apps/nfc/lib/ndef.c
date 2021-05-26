/* Copyright 2018 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/**
 * \file ndef.c
 *
 * minimal NDEF lib
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "api.h"

#include "ndef.h"

/**
 * \brief   Create a NDEF record
 * \param   *record
 *          Pointer to destination buffer
 *          tnf
 *          NDEF tnf value (valid from 0 to 6)
 *          is_begin
 *          1st NDEF record in a TLV destination block
 *          is_end
 *          last NDEF record in TLV destination block
 *          * type
 *          NDEF type
 *          type_length
 *          Lenght of type
 *          *payload
 *          The payload that will be copied to TLV destination block
 *          if payload is NULL, destination will be filled with 0
 *          payload_length
 *          Length of payload
 * \param   record
 *          Pointer to NDEF structure with the decoded message
 * \return  Length of NDEF record created.
 */
uint16_t ndef_create(
        ndef_record_t* record,
        uint8_t tnf,
        bool is_begin,
        bool is_end,
        const char* type,
        size_t type_length,
        const uint8_t* payload,
        size_t payload_length)
{
    uint16_t i, j;

    bool is_short;
    i=0;

    if ((record == NULL) || (record->buffer == NULL))
    {
        goto error;
    }

    is_short = (payload_length > 1 << 8) ? false : true;

    record->length = 2 + (is_short ? 1 : 4) + type_length + payload_length;

    record->type_length = type_length;
    record->payload_length = payload_length;

    if (tnf > 0x06)
    {
        goto error;
    }

    record->buffer[i++] = tnf;

    if (is_begin)
    {
        record->buffer[0] |= 0x80;
    }

    if (is_end)
    {
        record->buffer[0] |= 0x40;
    }

    if (is_short)
    {
        record->buffer[0] |= 0x10;
    }

    record->buffer[i++] = type_length;

    if (is_short)
    {
        record->buffer[i++] = (uint8_t) payload_length;
    }
    else
    {
        for (j = 0; j < 4; ++j)
        {
            record->buffer[i++] = 0xFF & (payload_length >> ((3 - j) * 8));
        }
    }

    record->type_offset = i;
    record->id_offset = record->type_offset + record->type_length;
    record->payload_offset = record->id_offset;
    record->length = record->payload_offset + record->payload_length;

    memcpy(record->buffer + record->type_offset, type, type_length);

    if (payload)
    {
        memcpy(record->buffer + record->payload_offset, payload, payload_length);
    }
    else
    {
        memset(record->buffer + record->payload_offset, 0x0, payload_length);
    }

error:
    return record->length;
}

/**
 * \brief   Decode a NDEF record
 * \param   buffer
 *          Pointer to 1st byte of a NDEF record
 * \param   record
 *          Pointer to NDEF structure with the decoded message
 * \return  true if it's the last message, or if record or buffer is NULL.
 */
ndef_res_e ndef_parse(ndef_record_t* record, const uint8_t* buffer)
{
    uint16_t i, j;

    if ((record == NULL) || (&buffer[0] == NULL))
    {
        return NDEF_RES_INVALID_CONFIG;
    }

    record->buffer = (uint8_t*)buffer;

    i = 1;
    record->type_length = record->buffer[i++];
    if (record->buffer[0] & 0x10) /* Short Record */
    {
        record->payload_length = record->buffer[i++];
    }
    else
    {
        record->payload_length = 0;
        for (j = 0; j < 4; ++j)
        {
            record->payload_length |= ((uint32_t) record->buffer[i++]) << ((3 - j) * 8);
        }
    }

    if (record->buffer[0] & 0x08) /* IL */
    {
        record->id_length = buffer[i++];
    }
    else
    {
        record->id_length = 0;
    }

    record->type_offset = (size_t)i;
    record->id_offset = record->type_offset + record->type_length;
    record->payload_offset = record->id_offset + record->id_length;
    record->length = record->payload_offset + record->payload_length;

    if (record->buffer[0] & 0x40) /* Msg End */
    {
        return NDEF_RES_LAST_MESSAGE;
    }

    return NDEF_RES_OK;
}
