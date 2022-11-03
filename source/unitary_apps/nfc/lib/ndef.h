/* Copyright 2018 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/**
 * \file ndef.h
 *
 * Minimal NDEF Lib
 */

#ifndef __NDEF_H__
#define __NDEF_H__

/**
 * Structure to describe an NDEF Record
 */
 typedef struct
 {
    uint8_t* buffer;
    uint16_t length;

    uint8_t type_length;
    uint16_t type_offset;

    uint8_t id_length;
    uint16_t id_offset;

    uint32_t payload_length;
    uint16_t payload_offset;
} ndef_record_t;


/** Return codes of NDEF functions */
typedef enum
{
    NDEF_RES_OK,
    NDEF_RES_INVALID_CONFIG,
    NDEF_RES_LAST_MESSAGE
} ndef_res_e;

/*
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
*          if payload is NULL, destiantion will be filled with 0
*          payload_length
*          Lenght of payload
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
        size_t payload_length);


/**
 * \brief   Decode a NDEF record
 * \param   buffer
 *          Pointer to 1st byte of a NDEF record
 * \param   record
 *          Pointer to NDEF structure with the decoded message
 * \return  true if it's the last message, or if record or buffer is NULL.
 */
ndef_res_e ndef_parse(ndef_record_t* record, const uint8_t* buffer);

#endif /* __NDEF_H__ */
