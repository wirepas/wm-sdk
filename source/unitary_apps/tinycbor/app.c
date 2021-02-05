/* Copyright 2019 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/*
 * \file    app.c
 * \brief   This file demonstrate the use of the tiny cbor library to decode
 *          a buffer.
 */

#include <stdio.h>

#include "api.h"
#include "node_configuration.h"
#include "cbor.h"

#define DEBUG_LOG_MODULE_NAME "CBOR APP"
#define DEBUG_LOG_MAX_LEVEL LVL_INFO
#include "debug_log.h"

static uint8_t m_cbor_buffer[] = {0xA5,       // map(5)
                                    0x00,     //Id 0
                                    0x50,     //16 bytes array
                                        0x00,
                                        0x01,
                                        0x02,
                                        0x03,
                                        0x04,
                                        0x05,
                                        0x06,
                                        0x07,
                                        0x08,
                                        0x09,
                                        0x0A,
                                        0x0B,
                                        0x0C,
                                        0x0D,
                                        0x0E,
                                        0x0F,
                                    0x01,     //Id 1
                                    0x6B,     //10 characters string
                                        'T',
                                        'e',
                                        's',
                                        't',
                                        ' ',
                                        'S',
                                        't',
                                        'r',
                                        'i',
                                        'n',
                                        'g',
                                    0x02,     //Id 2
                                    0x18,     //Unsigned int
                                        0x1F,
                                    0x03,     //Id 3
                                        0x20, //Signed int
                                    0x04,     //Id 4
                                    0x1A,     //Large unsigned int
                                        0x07,
                                        0x5B,
                                        0xCD,
                                        0x15};

static bool dumprecursive(CborValue *it, int nestingLevel)
{
    char indent[32];
    int idx = 0, lvl = nestingLevel;

    while (lvl--)
    {
        indent[idx++] = ' ';
        indent[idx++] = ' ';
    }
    indent[idx] = '\0';

    while (!cbor_value_at_end(it))
    {
        CborError err;
        CborType type = cbor_value_get_type(it);

        switch (type)
        {
        case CborArrayType:
        case CborMapType:
        {
            /* Recursive type. */
            CborValue recursed;
            LOG(LVL_INFO, "%s%s", indent,
                                  type == CborArrayType ? "Array[" : "Map[");
            err = cbor_value_enter_container(it, &recursed);
            if (err != CborNoError)
            {
                /* Parse error. */
                return err;
            }
            err = dumprecursive(&recursed, nestingLevel + 1);
            if (err != CborNoError)
            {
                /* Parse error. */
                return err;
            }
            err = cbor_value_leave_container(it, &recursed);
            if (err != CborNoError)
            {
                /* Parse error. */
                return err;
            }
            LOG(LVL_INFO, "%s]", indent);
            continue;
        }

        case CborIntegerType:
        {
            int val;
            err = cbor_value_get_int_checked(it, &val);
            if (err != CborNoError)
            {
                /* Parse error. */
                return err;
            }
            LOG(LVL_INFO, "%sInteger: %d", indent, val);
            break;
        }

        case CborByteStringType:
        {
            uint8_t data[64];
            size_t n = 64;
            err = cbor_value_copy_byte_string(it, data, &n, it);
            if (err != CborNoError)
            {
                /* Parse error. */
                return err;
            }
            LOG(LVL_INFO, "%sByteString: ", indent);
            LOG_BUFFER(LVL_INFO, data, n);

            continue;
        }

        case CborTextStringType:
        {
            char data[64];
            size_t n = 64;
            err = cbor_value_copy_text_string(it, data, &n, it);
            if (err != CborNoError)
            {
                /* Parse error. */
                return err;
            }
            LOG(LVL_INFO, "%sTextString: %s", indent, data);
            continue;
        }

        case CborTagType:
        {
            CborTag tag;
            cbor_value_get_tag(it, &tag); // can't fail
            LOG(LVL_INFO, "%sTag(%d)", indent, tag);
            break;
        }

        case CborSimpleType:
        {
            uint8_t type;
            cbor_value_get_simple_type(it, &type); // can't fail
            LOG(LVL_INFO, "%sSimple(%u)", indent, type);
            break;
        }

        case CborNullType:
        {
            LOG(LVL_INFO, "%sNull", indent);
            break;
        }

        case CborUndefinedType:
        {
            LOG(LVL_INFO, "%sUndefined", indent);
            break;
        }

        case CborBooleanType:
        {
            bool val;
            cbor_value_get_boolean(it, &val); // can't fail
            LOG(LVL_INFO, "%sBoolean: %s", indent, val ? "true" : "false");
            break;
        }

        case CborDoubleType:
        case CborFloatType:
        case CborHalfFloatType:
        {
            /* Not managed. */
            LOG(LVL_WARNING, "%sFloat: (not managed)", indent);
            break;
        }

        case CborInvalidType:
            /* Can't happen. */
            LOG(LVL_INFO, "%sInvalidType", indent);
            break;
        }

        err = cbor_value_advance_fixed(it);
        if (err != CborNoError)
        {
            /* Parse error. */
            return err;
        }
    LOG_FLUSH(LVL_INFO);
    }
    return CborNoError;
}

void App_init(const app_global_functions_t * functions)
{
    int len = sizeof(m_cbor_buffer);

    CborParser parser;
    CborValue value;
    CborError err;

    LOG_INIT();
    LOG(LVL_INFO, "TinyCBOR app started");

    err = cbor_parser_init(m_cbor_buffer, len, 0, &parser, &value);

    if (err == CborNoError)
    {
        err = cbor_value_validate(&value,CborValidateStrictMode);
    }

    if (err == CborNoError)
    {
        err = dumprecursive(&value, 0);
    }

    if (err != CborNoError)
    {
        LOG(LVL_ERROR, "CBOR parsing failure at offset %ld: %s",
                       value.ptr - m_cbor_buffer,
                       cbor_error_string(err));
    }
}
