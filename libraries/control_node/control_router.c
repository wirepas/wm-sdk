/* Copyright 2020 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */
#include <string.h>

#include "control_router.h"
#include "shared_data.h"
#include "tlv.h"

#define DEBUG_LOG_MODULE_NAME "CTR ROUT"
#define DEBUG_LOG_MAX_LEVEL LVL_NOLOG
#include "debug_log.h"

/** Forward declaration of DA received packet callback. */
app_lib_data_receive_res_e data_received (const shared_data_item_t * item,
                                          const app_lib_data_received_t * data);

/** DA diagnostics packet received filter and callback. */
static shared_data_item_t m_data_received_item = {
    .cb = data_received,
    .filter = {
        .mode = SHARED_DATA_NET_MODE_UNICAST,
        .src_endpoint = CONTROL_DIAG_SRC_EP,
        .dest_endpoint = CONTROL_DIAG_DEST_EP,
        .multicast_cb = NULL
    }
};

/** Copy of the configuration passed to control router library. */
static control_router_conf_t m_conf;
/** Has the library been initialinized */
static bool m_initialized = false;
/** Router DA acknowledge buffer. */
static uint8_t m_ack_buffer[DIRADV_MAX_ACK_LEN];

/**
 * \brief       Packet reveived from DA node callback
 * \param[in]   item
 *              Pointer to the filter item that initiated the callback.
 * \param[in]   data
 *              Pointer to the received data.
 * \return      Result code, \ref app_lib_data_receive_res_e.
 */
app_lib_data_receive_res_e data_received (const shared_data_item_t * item,
                                          const app_lib_data_received_t * data)
{
    control_fwd_diag_t diag;
    app_lib_data_send_res_e res;

    LOG(LVL_DEBUG, "Received DA diagnostic packet.");

    if (!m_initialized)
    {
        /* This should not happen. As callback is set in init. */
        return APP_LIB_DATA_RECEIVE_RES_NOT_FOR_APP;
    }

    if (data->num_bytes != sizeof(control_diag_t))
    {
        LOG(LVL_ERROR, "    Invalid DIAG length (len:%d).", data->num_bytes);
    }

    diag.address = data->src_address;
    memcpy(&diag.diag, data->bytes, sizeof(control_diag_t));

    app_lib_data_to_send_t send_data =
    {
        .bytes = (const uint8_t *) &diag,
        .num_bytes = sizeof(control_fwd_diag_t),
        .delay = 0,
        .tracking_id = APP_LIB_DATA_NO_TRACKING_ID,
        .qos = APP_LIB_DATA_QOS_NORMAL,
        .flags = APP_LIB_DATA_SEND_FLAG_NONE,
        .src_endpoint = CONTROL_DIAG_SRC_EP,
        .dest_endpoint = CONTROL_DIAG_DEST_EP,
        .dest_address = APP_ADDR_ANYSINK,
    };

    /* Forward diagnostic to the sink. */
    LOG(LVL_DEBUG, "    DIAG (forward to Sink).");
    res = Shared_Data_sendData(&send_data, NULL);
    if (res != APP_LIB_DATA_SEND_RES_SUCCESS)
    {
        LOG(LVL_DEBUG, "Error forwarding diagnostics (res:%d).",
                        res);
    }

    return APP_LIB_DATA_RECEIVE_RES_HANDLED;
}

/**
 * \brief       Callback on headnode. Generate ACK response to DA node.
 * \param[in]   in
 *              Information on received packet from advertiser
 * \param[out]  out
 *              Acknowledgement to be sent to advertiser
 * \return      Always true, i.e. send acknowledgement
 */
static bool acklistener_cb(const ack_gen_input_t * in,
                           ack_gen_output_t * out)
{
    control_ack_t ack;
    app_lib_otap_seq_t seq = 0;
    ack_gen_input_t user_in;

    tlv_res_e tlv_ret = TLV_RES_ERROR;
    tlv_item_t item;
    tlv_record record;

    if (!m_initialized)
    {
        /* This should not happen. As callback is set in init. */
        return true;
    }

    if (m_conf.ack_gen_cb != NULL)
    {
        memcpy(&user_in, in, sizeof(ack_gen_input_t));
        user_in.ack_length -= (sizeof(control_ack_t) + 4);
                                                    //+2 for USER Type & Length
                                                    //+2 for ACK  Type & Length
        if (m_conf.ack_gen_cb(in, &seq, out) == false)
        {
            /* Discard USER Ack data. */
            out->length = 0;
        }
    }

    LOG(LVL_INFO, "Generate Acknowledge. (in_len: %d)", user_in.ack_length);

    Tlv_init(&record, m_ack_buffer, DIRADV_MAX_ACK_LEN);

    /* Add ACK data to packet. */
    ack.otap_seq = seq;

    LOG(LVL_DEBUG, "    ACK (seq:%d).", seq);
    item.type = CONTROL_TYPE_ACK;
    item.length = sizeof(control_ack_t);
    item.value = (uint8_t *) &ack;
    tlv_ret = Tlv_Encode_addItem(&record, &item);

    if (tlv_ret == TLV_RES_OK &&
        out->length != 0 &&
        out->length <= user_in.ack_length)
    {
        LOG(LVL_DEBUG, "    USER (out_len:%d).", out->length);

        /* No need to check tlv_ret, only error is data to big. */
        item.type = CONTROL_TYPE_USER;
        item.length = out->length;
        item.value = out->data;
        Tlv_Encode_addItem(&record, &item);
    }

    out->data = m_ack_buffer;
    out->length = Tlv_Encode_getBufferSize(&record);

    return true;
}

control_node_ret_e Control_Router_init(control_router_conf_t * conf)
{
    app_res_e app_res;
    app_lib_settings_role_t role;

    LOG(LVL_INFO, "Control Router Init");

    /* Check parameters. */
    if (conf->ack_gen_cb == NULL)
    {
        LOG(LVL_ERROR, "Invalid parameters.");
        return CONTROL_RET_INVALID_PARAM;
    }

    if (lib_settings->getNodeRole(&role) != APP_RES_OK)
    {
        /* The node role must be set*/
        return CONTROL_RET_INVALID_ROLE;
    }

    /* Must be Low Latency Router. */
    if ((app_lib_settings_get_base_role(role) !=
         APP_LIB_SETTINGS_ROLE_HEADNODE) ||
         (app_lib_settings_get_flags_role(role) !=
         APP_LIB_SETTINGS_ROLE_FLAG_LL))
    {
        return CONTROL_RET_INVALID_ROLE;
    }

    /* Copy config to local. */
    memcpy(&m_conf, conf, sizeof(control_router_conf_t));

    /* Set callback to receive packet from advertiser and generate response
     * to sink.
     */
    lib_advertiser->setRouterAckGenCb(acklistener_cb);
    app_res = Shared_Data_addDataReceivedCb(&m_data_received_item);
    if (app_res != APP_RES_OK)
    {
        LOG(LVL_ERROR, "Error adding Data callback (res:%d)", app_res);
        return CONTROL_RET_ERROR;
    }

    m_initialized = true;

    return CONTROL_RET_OK;
}
