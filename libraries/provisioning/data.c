/* Copyright 2019 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

#include "provisioning.h"
#include "provisioning_int.h"
#include "time.h"

#define DEBUG_LOG_MODULE_NAME "PROV DAT"
#define DEBUG_LOG_MAX_LEVEL LVL_INFO
#include "debug_log.h"


/** Size of the buffer to store strings from CBOR buffer. */
#define MAX_STRING_BUFFER_SIZE 94

/** Invalid values for the network parameters. */
#define INVALID_KEY 0xFF
#define INVALID_NET_ADDR 0
#define INVALID_NET_CHAN 0
#define INVALID_NODE_ADDR 0
#define INVALID_NODE_ROLE 0xFF

/** \brief Structure containing the Wirepas network parameters. */
static struct
{
    uint8_t enc_key[APP_LIB_SETTINGS_AES_KEY_NUM_BYTES];
    bool is_enc_key_set;
    uint8_t auth_key[APP_LIB_SETTINGS_AES_KEY_NUM_BYTES];
    bool is_auth_key_set;
    app_lib_settings_net_addr_t net_addr;
    app_lib_settings_net_channel_t net_chan;
    app_addr_t node_addr;
    app_lib_settings_role_t node_role;
} m_provisioning_data;

/**
 * \brief   Extract a byte array from a CBOR Value.
 * \param   value
 *          Pointer to a CBOR Value.
 * \param   buffer
 *          Pointer to where to store the data.
 * \param   buflen
 *          [In] Size of the buffer. [Out] Size of the extracted byte string.
 * \return  A CborError error code.
 */
static CborError extract_byte_string(CborValue * value,
                                     uint8_t *buffer,
                                     size_t *buflen)
{
    if (!cbor_value_is_byte_string(value))
    {
        return CborErrorIllegalType;
    }

    return cbor_value_copy_byte_string(value, buffer, buflen, NULL);
}

/**
 * \brief   Extract an unsigned int a CBOR Value.
 * \param   value
 *          Pointer to a CBOR Value.
 * \param   data
 *          Pointer to store the unsigned int.
 * \param   size
 *          Size in bytes of value pointed by data.
 * \note    This function does not support unsigned int bigger than 64 bits.
 * \return  A CborError error code.
 */
static CborError extract_unsigned_int(CborValue * value,
                                      void * data,
                                      size_t size)
{
    uint64_t val;
    CborError err;

    if (!cbor_value_is_unsigned_integer(value))
    {
        return CborErrorIllegalType;
    }

    err = cbor_value_get_uint64(value, &val);
    if (err != CborNoError)
    {
        return err;
    }

    if (size == 8)
    {

    }
    else if (size == 0)
    {
        return CborErrorDataTooLarge;
    }
    else if (size > 8 || val > (uint64_t)((2<<(size*8)) - 1))
    {
        return CborErrorDataTooLarge;
    }

    memcpy(data, &val, size);

    return CborNoError;
}

/**
 * \brief   Parse encryption key from CBOR buffer.
 * \param   value
 *          Pointer to a CBOR Value. Points to provisioning data encoded
 *          in a Cbor(Data of one Id:Data map entry).
 * \return  A CborError error code.
 */
static CborError parse_enc_key(CborValue * value)
{
    CborError err;
    size_t len = APP_LIB_SETTINGS_AES_KEY_NUM_BYTES;
    err = extract_byte_string(value,
                                m_provisioning_data.enc_key,
                                &len);
    if (err != CborNoError)
    {
        return err;
    }

    if (len != APP_LIB_SETTINGS_AES_KEY_NUM_BYTES)
    {
        return CborErrorImproperValue;
    }

    m_provisioning_data.is_enc_key_set = true;

    return err;
}

/**
 * \brief   Parse authentication key from CBOR buffer.
 * \param   value
 *          Pointer to a CBOR Value. Points to provisioning data encoded
 *          in a Cbor(Data of one Id:Data map entry).
 * \return  A CborError error code.
 */
static CborError parse_auth_key(CborValue * value)
{
    CborError err;
    size_t len = APP_LIB_SETTINGS_AES_KEY_NUM_BYTES;
    err = extract_byte_string(value, m_provisioning_data.auth_key, &len);
    if (err != CborNoError)
    {
        return err;
    }

    if (len != APP_LIB_SETTINGS_AES_KEY_NUM_BYTES)
    {
        return CborErrorImproperValue;
    }

    m_provisioning_data.is_auth_key_set = true;

    return err;
}

/**
 * \brief   Parse network address from CBOR buffer.
 * \param   value
 *          Pointer to a CBOR Value. Points to provisioning data encoded
 *          in a Cbor(Data of one Id:Data map entry).
 * \return  A CborError error code.
 */
static CborError parse_net_address(CborValue * value)
{
    CborError err;

    err = extract_unsigned_int(value,
                               &m_provisioning_data.net_addr,
                               sizeof(app_lib_settings_net_addr_t));

    if (err != CborNoError)
    {
        return err;
    }

    if (!lib_settings->isValidNetworkAddress(m_provisioning_data.net_addr))
    {
        m_provisioning_data.net_addr = INVALID_NET_ADDR;
        return CborErrorImproperValue;
    }

    return err;
}

/**
 * \brief   Parse network channel from CBOR buffer.
 * \param   value
 *          Pointer to a CBOR Value. Points to provisioning data encoded
 *          in a Cbor(Data of one Id:Data map entry).
 * \return  A CborError error code.
 */
static CborError parse_net_channel(CborValue * value)
{
    CborError err;

    err = extract_unsigned_int(value,
                               &m_provisioning_data.net_chan,
                               sizeof(app_lib_settings_net_channel_t));

    if (err != CborNoError)
    {
        return err;
    }

    if (!lib_settings->isValidNetworkChannel(m_provisioning_data.net_chan))
    {
        m_provisioning_data.net_chan = INVALID_NET_CHAN;
        return CborErrorImproperValue;
    }

    return err;
}

/**
 * \brief   Parse node address from CBOR buffer.
 * \param   value
 *          Pointer to a CBOR Value. Points to provisioning data encoded
 *          in a Cbor(Data of one Id:Data map entry).
 * \return  A CborError error code.
 */
static CborError parse_node_address(CborValue * value)
{
    CborError err;

    err = extract_unsigned_int(value,
                               &m_provisioning_data.node_addr,
                               sizeof(app_addr_t));

    if (err != CborNoError)
    {
        return err;
    }

    if (!lib_settings->isValidNodeAddress(m_provisioning_data.node_addr))
    {
        m_provisioning_data.node_addr = INVALID_NODE_ADDR;
        return CborErrorImproperValue;
    }

    return err;
}

/**
 * \brief   Parse node role from CBOR buffer.
 * \param   value
 *          Pointer to a CBOR Value. Points to provisioning data encoded
 *          in a Cbor(Data of one Id:Data map entry).
 * \return  A CborError error code.
 */
static CborError parse_node_role(CborValue * value)
{
    CborError err;
    size_t len = sizeof(app_lib_settings_role_t);

    err = extract_byte_string(value, &m_provisioning_data.node_role, &len);

    if (err != CborNoError)
    {
        return err;
    }

    if (len != sizeof(app_lib_settings_role_t) ||
        !lib_settings->isValidNodeRole(m_provisioning_data.node_role))
    {
        m_provisioning_data.node_role = INVALID_NODE_ROLE;
        return CborErrorImproperValue;
    }

    return err;
}

/**
 * \brief   Parse one CBOR Id of wirepas data.
 * \param   value
 *          Pointer to a CBOR Value. Points to provisioning data encoded
 *          in a Cbor(Data of one Id:Data map entry).
 * \param   id
 *          Id corresponding to the data
 * \return  A CborError error code.
 */
static CborError parse_wirepas_data(CborValue * value,
                                    int id)
{
    CborError err;

    switch(id)
    {
        case PROV_DATA_ID_ENC_KEY:
        {
            err = parse_enc_key(value);
            break;
        }
        case PROV_DATA_ID_AUTH_KEY:
        {
            err = parse_auth_key(value);
            break;
        }
        case PROV_DATA_ID_NET_ADDR:
        {
            err = parse_net_address(value);
            break;
        }
        case PROV_DATA_ID_NET_CHAN:
        {
            err = parse_net_channel(value);
            break;
        }
        case PROV_DATA_ID_NODE_ADDR:
        {
            err = parse_node_address(value);
            break;
        }
        case PROV_DATA_ID_NODE_ROLE:
        {
            err = parse_node_role(value);
            break;
        }
        default:
        {
            /* This should not happen as id is tested before calling this
             * function.
             */
            err = CborErrorImproperValue;
        }
    }

    return err;
}

/**
 * \brief   Parse one CBOR Id of User data.
 * \param   value
 *          Pointer to a CBOR Value. Points to provisioning data encoded
 *          in a Cbor(Data of one Id:Data map entry).
 * \param   id
 *          Id corresponding to the data
 * \param   cb
 *          Callback to call for each User specific Id found.
 *          It is called only if the whole provisioning data is valid.
 * \return  A CborError error code.
 */
static CborError parse_user_data(CborValue * value,
                                 int id,
                                 provisioning_user_data_cb_f cb)
{
    CborError err;

    size_t len;
    /* Force alignement as buffer can contain int64 values. */
    uint8_t val[MAX_STRING_BUFFER_SIZE]  __attribute__ ((aligned (8)));
    void * data = &val;
    CborType type = cbor_value_get_type(value);

    switch (type)
    {
        case CborIntegerType:
        {
            len = sizeof(int64_t);
            err = cbor_value_get_int64_checked(value,
                                                (int64_t *)data);
            break;
        }

        case CborByteStringType:
        {
            len = MAX_STRING_BUFFER_SIZE;
            err = cbor_value_copy_byte_string(value,
                                                (uint8_t *)data,
                                                &len,
                                                NULL);
            break;
        }

        case CborTextStringType:
        {
            len = MAX_STRING_BUFFER_SIZE;
            err = cbor_value_copy_text_string(value,
                                                (char *)data,
                                                &len,
                                                NULL);
            break;
        }

        case CborSimpleType:
        {
            len = sizeof(uint8_t);
            err = cbor_value_get_simple_type(value,
                                                (uint8_t *)data);
            break;
        }

        case CborBooleanType:
        {
            len = sizeof(bool);
            err = cbor_value_get_boolean(value, (bool *)data);
            break;
        }

        case CborDoubleType:
        {
            len = sizeof(double);
            err = cbor_value_get_double(value,
                                        (double *)data);
            break;
        }

        case CborFloatType:
        {
            len = sizeof(float);
            err = cbor_value_get_float(value,
                                        (float *)data);
            break;
        }
        case CborHalfFloatType:
        {
            len = sizeof(uint16_t);
            err = cbor_value_get_half_float(value,
                                            (uint16_t *)data);
            break;
        }

        default:
            return CborErrorUnknownType;
    }

    if (err != CborNoError)
    {
        return err;
    }

    if (cb != NULL)
    {
        LOG(LVL_DEBUG, "User data (id : %d, type : %d, len : %d).",
                        id,
                        type,
                        len);
        cb(id, type, data, len);
    }

    return err;
}

/**
 * \brief   Parse the CBOR encoded provisioning data.
 * \param   value
 *          Pointer to a CBOR Value. Points to provisioning data encoded
 *          in a Cbor map (Id:Data).
 * \param   cb
 *          Callback to call for each User specific Id found.
 *          It is called only if the whole provisioning data is valid.
 * \return  A CborError error code.
 */
static CborError parse_map(CborValue * value, provisioning_user_data_cb_f cb)
{
    CborError err;
    int id;

    /* Sets provisioning data structure to invalid values. */
    m_provisioning_data.is_auth_key_set = false;
    m_provisioning_data.is_enc_key_set = false;
    m_provisioning_data.net_addr = INVALID_NET_ADDR;
    m_provisioning_data.net_chan = INVALID_NET_CHAN;
    m_provisioning_data.node_addr = INVALID_NODE_ADDR;
    m_provisioning_data.node_role = INVALID_NODE_ROLE;


    while (!cbor_value_at_end(value))
    {
        /* Get the Id. */
        if (!cbor_value_is_unsigned_integer(value))
        {
            return CborErrorIllegalType;
        }

        err = cbor_value_get_int_checked(value, &id);
        if (err != CborNoError)
        {
            return err;
        }

        /* Get the data. */
        err = cbor_value_advance_fixed(value);
        if (err != CborNoError)
        {
            return err;
        }

        /* Match Wirepas Ids. Id > 0 already checked above. */
        if (id <= PROV_DATA_ID_NODE_ROLE)
        {
            err = parse_wirepas_data(value, id);
        }
        /* Match User Ids. */
        else if (id >= PROV_DATA_MIN_USER_ID && id <= PROV_DATA_MAX_USER_ID)
        {
            err = parse_user_data(value, id, cb);
        }
        else
        {
            return CborErrorImproperValue;
        }

        if (err != CborNoError)
        {
            return err;
        }

        err = cbor_value_advance(value);
        if (err != CborNoError)
        {
            return err;
        }
    }

    /* Encryption and Authentatication keys, network address and channel
     * are mandatory in the provisioning data packet.
     */
    if (!m_provisioning_data.is_enc_key_set ||
        !m_provisioning_data.is_auth_key_set ||
        m_provisioning_data.net_addr == INVALID_NET_ADDR ||
        m_provisioning_data.net_chan == INVALID_NET_CHAN)
    {
        return CborErrorTooFewItems;
    }

    return CborNoError;
}

/**
 * \brief   Apply received network parameters.
 */
static void apply_network_parameters(void)
{
    LOG(LVL_DEBUG, "Network parameters :");
    LOG(LVL_DEBUG, " - Encryption key : * * * * * * * * * * * *"
                   " %02X %02X %02X %02X",
                   m_provisioning_data.enc_key[12],
                   m_provisioning_data.enc_key[13],
                   m_provisioning_data.enc_key[14],
                   m_provisioning_data.enc_key[15]);

    LOG(LVL_DEBUG, " - Authentication key : * * * * * * * * * * * *"
                   " %02X %02X %02X %02X",
                   m_provisioning_data.auth_key[12],
                   m_provisioning_data.auth_key[13],
                   m_provisioning_data.auth_key[14],
                   m_provisioning_data.auth_key[15]);

    lib_settings->setAuthenticationKey(m_provisioning_data.auth_key);
    lib_settings->setEncryptionKey(m_provisioning_data.enc_key);

    if (m_provisioning_data.net_addr != INVALID_NET_ADDR)
    {
        LOG(LVL_DEBUG, " - Network address : 0x%06X",
                       m_provisioning_data.net_addr);
        lib_settings->setNetworkAddress(m_provisioning_data.net_addr);
    }

    if (m_provisioning_data.net_chan != INVALID_NET_CHAN)
    {
        LOG(LVL_DEBUG, " - Network channel : %d",
                       m_provisioning_data.net_chan);
        lib_settings->setNetworkChannel(m_provisioning_data.net_chan);
    }

    if (m_provisioning_data.node_addr != INVALID_NODE_ADDR)
    {
        LOG(LVL_DEBUG, " - Node address : 0x%08X",
                       m_provisioning_data.node_addr);
        lib_settings->setNodeAddress(m_provisioning_data.node_addr);
    }

    if (m_provisioning_data.node_role != INVALID_NODE_ROLE)
    {
        LOG(LVL_DEBUG, " - Node role : 0x%02X", m_provisioning_data.node_role);
        lib_settings->setNodeRole(m_provisioning_data.node_role);
    }

    LOG(LVL_INFO, "Reboot.");

    /* Wait some time to print logs before rebooting. */
    LOG_FLUSH(LVL_INFO);
}

provisioning_ret_e Provisioning_Data_decode(provisioning_data_conf_t * conf, bool dry_run)
{
    CborParser parser;
    CborValue value;
    CborError err;

    if (conf == NULL || conf->buffer == NULL || conf->length == 0)
    {
        LOG(LVL_ERROR, "%s : PROV_RET_INVALID_PARAM.", __func__);
        return PROV_RET_INVALID_PARAM;
    }

    err = cbor_parser_init(conf->buffer, conf->length, 0, &parser, &value);

    if (err == CborNoError)
    {
        CborValue map;

        /* Begin parsing the received Cbor buffer. */
        if (!cbor_value_is_map(&value))
        {
            /* Data buffer must be organised as a map. */
            LOG(LVL_ERROR, "%s : PROV_RET_INVALID_DATA.", __func__);
            return PROV_RET_INVALID_DATA;
        }

        err = cbor_value_enter_container(&value, &map);
        if (err != CborNoError)
        {
            /* Error entering the map. */
            LOG(LVL_ERROR, "%s : PROV_RET_INVALID_DATA.", __func__);
            return PROV_RET_INVALID_DATA;
        }

        if(dry_run)
        {
            /* First to verify the data is valid but don't call the User
             * callback.
             */
            err = parse_map(&map, NULL);

            if (err != CborNoError)
            {
                /* Error when parsing the buffer. */
                LOG(LVL_ERROR, "%s : PROV_RET_INVALID_DATA (cBorError %d).",
                            __func__,
                            err);
                return PROV_RET_INVALID_DATA;
            }

            LOG(LVL_INFO, "Provisioning data is valid.");
            return PROV_RET_OK;
        }
        else
        {
            /* Parse the buffer. call user callback for customer data */
            err = parse_map(&map, conf->user_data_cb);

            /* This should not happen as the buffer is tested during dryrun. */
            if (err != CborNoError)
            {
                /* Error when parsing the buffer. */
                LOG(LVL_ERROR, "%s : PROV_RET_INVALID_DATA (cBorError %d).",
                            __func__,
                            err);
                return PROV_RET_INVALID_DATA;
            }

            if (conf->end_cb != NULL)
            {
                if (conf->end_cb(PROV_RES_SUCCESS))
                {
                    /* Stop the stack and apply new network parameters.
                     * This will trigger a reboot.
                     */
                    LOG(LVL_INFO, "Applying network parameters.");
                    lib_system->setShutdownCb(apply_network_parameters);
                    lib_state->stopStack(); /* Does not return. */
                }
            }
            return PROV_RET_OK;
        }
    }

    LOG(LVL_ERROR, "%s : PROV_RET_INVALID_DATA.", __func__);
    return PROV_RET_INVALID_DATA;
}