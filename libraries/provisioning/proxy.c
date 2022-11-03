/* Copyright 2019 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

#include <string.h>

#include "provisioning.h"
#include "provisioning_int.h"
#include "shared_data.h"
#include "random.h"
#include "node_configuration.h"
#include "api.h"
#include "aessw.h"

#define DEBUG_LOG_MODULE_NAME "PROXY LIB"
#define DEBUG_LOG_MAX_LEVEL LVL_INFO
#include "debug_log.h"

/** Copy of the configuration passed during initialization. */
static provisioning_proxy_conf_t m_conf;
/** Placeholder for the network parameters sent to the new node. */
static provisioning_proxy_net_param_t m_net_param;
/** Joining proxy library is startet. */
static bool m_started;
/** Joining proxy library is initialized. */
static bool m_init = false;
/** Provisioning packet received filter and callback. */
static shared_data_item_t m_ptk_received_item;
/** The counter used in AES encryption. */
static uint16_t m_counter;

/**
 * \brief   Sends a NACK packet to the existing node.
 * \param   type
 *          Reason why the NACK is sent.
 * \param   data
 *          The received packet data and metadata.
 *          (contains the sender address, START packet).
 */
void send_nack(prov_nack_type_e type, const app_lib_data_received_t * data)
{
    app_lib_data_send_res_e res;
    pdu_prov_start_t * pdu = (pdu_prov_start_t *) data;
    pdu_prov_nack_t nack_pdu;

    /* Generate NACK packet. */
    nack_pdu.pdu_header.type = PROV_PACKET_TYPE_DATA;
    nack_pdu.pdu_header.address = pdu->pdu_header.address;
    nack_pdu.pdu_header.session_id = pdu->pdu_header.session_id;
    nack_pdu.nack_type = type;

    /* Send NACK packet. */
    app_lib_data_to_send_t data_to_send =
    {
        .bytes = (uint8_t *)&nack_pdu,
        .num_bytes = sizeof(pdu_prov_nack_t),
        .dest_address = data->src_address,
        .delay = 0,
        .qos = APP_LIB_DATA_QOS_HIGH,
        .flags = APP_LIB_DATA_SEND_FLAG_NONE,
        .src_endpoint = PROV_DOWNLINK_EP,
        .dest_endpoint = PROV_UPLINK_EP
    };

    LOG(LVL_INFO, "Send NACK packet (type:%d).", type);

    res = Shared_Data_sendData(&data_to_send, NULL);
    if (res != APP_LIB_DATA_SEND_RES_SUCCESS)
    {
        LOG(LVL_WARNING, "Error sending NACK (res:%d).", res);
    }
}

/**
 * \brief   Encode the provisioning data in a CBOR buffer.
 * \param   buffer
 *          A pointer to store the encoded data.
 * \param   len
 *          [In] The size of the buffer, [Out] The size of the encoded data.
 * \return  A CborError error code.
 */
CborError encode_cbor_map(uint8_t * buffer, uint8_t  * len)
{
    CborError err;
    CborEncoder encoder, mapEncoder;

    cbor_encoder_init(&encoder, buffer, *len, 0);
    err = cbor_encoder_create_map(&encoder, &mapEncoder, 4);
    if (err != CborNoError)
    {
        return err;
    }

    /* [0]: Encryption key */
    err = cbor_encode_uint(&mapEncoder, PROV_DATA_ID_ENC_KEY);
    if (err != CborNoError)
    {
        return err;
    }
    err = cbor_encode_byte_string(&mapEncoder,
                                  m_net_param.enc_key,
                                  APP_LIB_SETTINGS_AES_KEY_NUM_BYTES);
    if (err != CborNoError)
    {
        return err;
    }

    /* [1]: Authentication key */
    err = cbor_encode_uint(&mapEncoder, PROV_DATA_ID_AUTH_KEY);
    if (err != CborNoError)
    {
        return err;
    }
    err = cbor_encode_byte_string(&mapEncoder,
                                  m_net_param.auth_key,
                                  APP_LIB_SETTINGS_AES_KEY_NUM_BYTES);
    if (err != CborNoError)
    {
        return err;
    }

    /* [2]: Network address */
    err = cbor_encode_uint(&mapEncoder, PROV_DATA_ID_NET_ADDR);
    if (err != CborNoError)
    {
        return err;
    }
    err = cbor_encode_uint(&mapEncoder, m_net_param.net_addr);
    if (err != CborNoError)
    {
        return err;
    }

    /* [3]: Network channel */
    err = cbor_encode_uint(&mapEncoder, PROV_DATA_ID_NET_CHAN);
    if (err != CborNoError)
    {
        return err;
    }
    err = cbor_encode_uint(&mapEncoder, m_net_param.net_chan);
    if (err != CborNoError)
    {
        return err;
    }

    err = cbor_encoder_close_container(&encoder, &mapEncoder);
    if (err != CborNoError)
    {
        return err;
    }

    *len = cbor_encoder_get_buffer_size(&encoder, buffer);

    return err;
}

/**
 * \brief   Encrypts/Authenticates a Provisioning DATA packet.
 * \param   data
 *          A pointer to the DATA packet.
 * \param   data_len
 *          The size of the provisioning data (not the whole packet).
 * \param   iv
 *          A pointer to the IV received in the START packet.
 */
void encrypt_data(pdu_prov_data_t * data_pdu, uint8_t data_len, uint8_t * iv)
{
    aes_data_stream_t data_stream;
    aes_omac1_state_t omac1_state;
    uint32_t icb[AES_128_KEY_BLOCK_SIZE/4];
    uint32_t sum;
    uint8_t mic[PROV_MIC_SIZE];

    m_counter++;
    data_pdu->key_index = 1;
    data_pdu->counter = m_counter;

    /* Initialize counter. */
    memcpy(icb, iv, AES_128_KEY_BLOCK_SIZE);
    sum = icb[0] + m_counter;

    if (sum < icb[0])
    {
        if (++(icb[1]) == 0)
        {
            if (++(icb[2]) == 0)
            {
                ++(icb[3]);
            }
        }
    }
    icb[0] = sum;

    LOG(LVL_DEBUG, "Encrypt data - Ctr: %d, - IV:", m_counter);
    LOG_BUFFER(LVL_DEBUG, iv, AES_128_KEY_BLOCK_SIZE);
    LOG(LVL_DEBUG, "Encrypt data - ICB:");
    LOG_BUFFER(LVL_DEBUG, ((uint8_t*)icb), AES_128_KEY_BLOCK_SIZE);

    /* Authenticate whole packet (Hdr + Key idx + Ctr + Data). */
    aes_initOmac1(&omac1_state, m_conf.key);
    aes_omac1(&omac1_state,
              mic,
              PROV_MIC_SIZE,
              (uint8_t *)data_pdu,
              data_len + PROV_DATA_OFFSET);

    /* Copy MIC after the data. */
    memcpy(&data_pdu->data[data_len], mic, PROV_MIC_SIZE);

    /* Encrypt Data + MIC. */
    aes_setupStream(&data_stream,
                    &m_conf.key[ENC_KEY_OFFSET],
                    (uint8_t*)icb);
    aes_crypto128Ctr(&data_stream,
                     data_pdu->data,
                     data_pdu->data, // overwrite input buffer
                     data_len + PROV_MIC_SIZE);
}

/**
 * \brief   Provisioning packet received callback. Handles START packets from
 *          new nodes.
 * \param   item
 *          Packet filter that generated this callback.
 * \param   data
 *          The packet data and metadata.
 * \return  Result code, @ref app_lib_data_receive_res_e
 */
static app_lib_data_receive_res_e pkt_received_cb(
                                        const shared_data_item_t * item,
                                        const app_lib_data_received_t * data)
{
    int8_t uid_len;
    uint8_t data_len = PROV_PDU_SIZE - PROV_DATA_OFFSET - PROV_MIC_SIZE;
    app_lib_data_send_res_e res;
    uint8_t data_buffer[PROV_PDU_SIZE];
    pdu_prov_data_t * data_pdu = (pdu_prov_data_t *) &data_buffer;
    pdu_prov_start_t * pdu = (pdu_prov_start_t *) data->bytes;

    LOG(LVL_DEBUG, "Packet received.");
    LOG_BUFFER(LVL_DEBUG, data->bytes, data->num_bytes);

    /* Check if it is a ACK packet. */
    if (pdu->pdu_header.type != PROV_PACKET_TYPE_START)
    {
        LOG(LVL_INFO, "ACK received from %08X.", data->src_address);
        return APP_LIB_DATA_RECEIVE_RES_HANDLED;
    }

    /* Check if it is a START packet. */
    if (pdu->pdu_header.type != PROV_PACKET_TYPE_START)
    {
        LOG(LVL_ERROR, "Not provisioning START packet.");
        return APP_LIB_DATA_RECEIVE_RES_HANDLED;
    }

    /* Check length. (-1 for method). */
    uid_len = data->num_bytes - sizeof(pdu_prov_hdr_t)
                              - 1
                              - AES_128_KEY_BLOCK_SIZE;
    if (uid_len <= 0)
    {
        LOG(LVL_ERROR, "START packet received, Invalid length.");
        return APP_LIB_DATA_RECEIVE_RES_HANDLED;
    }
    else
    {
        LOG(LVL_INFO, "START packet received, method: %d, UID:", pdu->method);
        LOG_BUFFER(LVL_INFO, pdu->uid, uid_len);
    }

    /* Check method. */
    switch(pdu->method)
    {
        case PROV_METHOD_UNSECURED:
        {
            if (!m_conf.is_local_unsec_allowed)
            {
                send_nack(PROV_NACK_TYPE_METHOD_NOT_SUPPORTED, data);
                LOG(LVL_ERROR, "Unsecured method is not supported.");
                return APP_LIB_DATA_RECEIVE_RES_HANDLED;
            }
            break;
        }
        case PROV_METHOD_SECURED:
        {
            if (!m_conf.is_local_sec_allowed)
            {
                send_nack(PROV_NACK_TYPE_METHOD_NOT_SUPPORTED, data);
                LOG(LVL_ERROR, "Secured method is not supported.");
                return APP_LIB_DATA_RECEIVE_RES_HANDLED;
            }
            break;
        }
        default:
        {
            LOG(LVL_ERROR, "Invalid method.");
            return APP_LIB_DATA_RECEIVE_RES_HANDLED;
            break;
        }
    }

    /* Call start callback. */
    if (m_conf.start_cb != NULL)
    {
        if (!m_conf.start_cb(pdu->uid, uid_len, pdu->method, &m_net_param))
        {
            send_nack(PROV_NACK_TYPE_NOT_AUTHORIZED, data);
            LOG(LVL_INFO, "Node rejected by app.");
            return APP_LIB_DATA_RECEIVE_RES_HANDLED;
        }
    }

    /* Generate DATA packet. */
    data_pdu->pdu_header.type = PROV_PACKET_TYPE_DATA;
    data_pdu->pdu_header.address = pdu->pdu_header.address;
    data_pdu->pdu_header.session_id = pdu->pdu_header.session_id;
    data_pdu->key_index = 0;
    data_pdu->counter = 0x0000;

    if (encode_cbor_map(data_pdu->data, &data_len) != CborNoError)
    {
        LOG(LVL_ERROR, "Error encoding CBOR.");
        return APP_LIB_DATA_RECEIVE_RES_HANDLED;
    }

    /* Encrypt if SECURED method. */
    if (pdu->method == PROV_METHOD_SECURED)
    {
        encrypt_data(data_pdu, data_len, pdu->iv);
        data_len += PROV_MIC_SIZE;
    }

    /* Send DATA packet. */
    app_lib_data_to_send_t data_to_send =
    {
        .bytes = data_buffer,
        .num_bytes = PROV_DATA_OFFSET + data_len,
        .dest_address = data->src_address,
        .delay = 0,
        .qos = APP_LIB_DATA_QOS_HIGH,
        .flags = APP_LIB_DATA_SEND_FLAG_NONE,
        .src_endpoint = PROV_DOWNLINK_EP,
        .dest_endpoint = PROV_UPLINK_EP
    };

    LOG(LVL_INFO, "Send DATA packet.");
    LOG_BUFFER(LVL_DEBUG, data_buffer, PROV_DATA_OFFSET + data_len);

    res = Shared_Data_sendData(&data_to_send, NULL);
    if (res != APP_LIB_DATA_SEND_RES_SUCCESS)
    {
        LOG(LVL_WARNING, "Error sending DATA (res:%d).", res);
    }

    return APP_LIB_DATA_RECEIVE_RES_HANDLED;
}

provisioning_ret_e Provisioning_Proxy_init(provisioning_proxy_conf_t * conf)
{
    bool is_local_provisioning = conf->is_local_sec_allowed ||
                                 conf->is_local_unsec_allowed;

    LOG(LVL_DEBUG, "%s, Configuration:",__func__);
    LOG(LVL_DEBUG, " - Tx Power (dBm) : %d", conf->tx_power);
    LOG(LVL_DEBUG, " - Payload :");
    LOG_BUFFER(LVL_DEBUG, conf->payload, conf->num_bytes);
    if (is_local_provisioning)
    {
        LOG(LVL_DEBUG, " - is_local_unsec_allowed: %d",
                       conf->is_local_unsec_allowed);
        LOG(LVL_DEBUG, " - is_local_sec_allowed: %d",
                       conf->is_local_sec_allowed);
        if (conf->is_local_sec_allowed)
        {
            LOG(LVL_DEBUG, " - Key:");
            LOG_BUFFER(LVL_DEBUG, conf->key, conf->key_len);
        }
    }

    /* (re)Initialization is possible only if joining beacons are disabled. */
    if (m_init && m_started)
    {
        LOG(LVL_ERROR, "%s : PROV_RET_INVALID_STATE.", __func__);
        return PROV_RET_INVALID_STATE;
    }

    /* Parameters check. */
    if ((conf->payload == NULL && conf->num_bytes != 0) ||
        (conf->payload != NULL && conf->num_bytes == 0))
    {
        LOG(LVL_ERROR, "Init, PROV_RET_INVALID_PARAM");
        return PROV_RET_INVALID_PARAM;
    }

    if (is_local_provisioning)
    {
        if ((conf->is_local_sec_allowed && conf->key_len == 0) ||
            conf->start_cb == NULL)
        {
            LOG(LVL_ERROR, "Init, PROV_RET_INVALID_PARAM");
            return PROV_RET_INVALID_PARAM;
        }

        m_ptk_received_item.filter.mode = SHARED_DATA_NET_MODE_UNICAST;
        m_ptk_received_item.filter.src_endpoint = PROV_UPLINK_EP;
        m_ptk_received_item.filter.dest_endpoint = PROV_DOWNLINK_EP;
        m_ptk_received_item.cb = pkt_received_cb;
    }

    memcpy(&m_conf, conf, sizeof(m_conf));

    if (is_local_provisioning)
    {
        lib_joining->enableProxy(false);
        Shared_Data_addDataReceivedCb(&m_ptk_received_item);
        Random_init(getUniqueId() ^
                    lib_time->getTimestampHp());
        m_counter = Random_get16();
    }

    m_started = false;
    m_init = true;

    return PROV_RET_OK;
}

provisioning_ret_e Provisioning_Proxy_start(void)
{
    LOG(LVL_INFO, "Start sending joining beacons.");

    if (!m_init || m_started)
    {
        LOG(LVL_ERROR, "%s : PROV_RET_INVALID_STATE.", __func__);
        return PROV_RET_INVALID_STATE;
    }

    app_lib_joining_beacon_tx_param_t param =
    {
        .interval = JOINING_TX_INTERVAL,
        .addr = JOINING_NETWORK_ADDRESS,
        .channel = JOINING_NETWORK_CHANNEL,
        .tx_power = m_conf.tx_power,
        .type = JOINING_BEACON_TYPE,
        .payload = m_conf.payload,
        .payload_num_bytes = m_conf.num_bytes
    };

    if (lib_joining->startJoiningBeaconTx(&param) != APP_RES_OK)
    {
        LOG(LVL_ERROR, "%s : PROV_RET_JOINING_LIB_ERROR.", __func__);
        return PROV_RET_JOINING_LIB_ERROR;
    }

    if (m_conf.is_local_sec_allowed || m_conf.is_local_unsec_allowed)
    {
        lib_joining->enableProxy(false);
        Shared_Data_addDataReceivedCb(&m_ptk_received_item);
    }

    m_started = true;

    return PROV_RET_OK;
}

provisioning_ret_e Provisioning_Proxy_stop(void)
{
    LOG(LVL_INFO, "Stop sending joining beacons.");

    if (!m_init || !m_started)
    {
        LOG(LVL_ERROR, "%s : PROV_RET_INVALID_STATE.", __func__);
        return PROV_RET_INVALID_STATE;
    }

    if (m_conf.is_local_sec_allowed || m_conf.is_local_unsec_allowed)
    {
        lib_joining->enableProxy(true);
        Shared_Data_removeDataReceivedCb(&m_ptk_received_item);
    }

    if (lib_joining->stopJoiningBeaconTx() != APP_RES_OK)
    {
        LOG(LVL_ERROR, "%s : PROV_RET_JOINING_LIB_ERROR.", __func__);
        return PROV_RET_JOINING_LIB_ERROR;
    }

    m_started = false;

    return PROV_RET_OK;
}
