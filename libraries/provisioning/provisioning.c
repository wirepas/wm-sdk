/* Copyright 2019 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

#include "provisioning.h"
#include "provisioning_int.h"
#include "app_scheduler.h"
#include "shared_data.h"
#include "node_configuration.h"
#include "random.h"
#include "api.h"
#include "aessw.h"

#define DEBUG_LOG_MODULE_NAME "PROV LIB"
#define DEBUG_LOG_MAX_LEVEL LVL_INFO
#include "debug_log.h"

/** How long to wait if a packet send has failed. */
#define DELAY_RESEND_PACKET_MS 4000

/** \brief Provisioning state machine states. */
typedef enum
{
    PROV_STATE_UNINIT = 0, /**< Library is not initialized. */
    PROV_STATE_IDLE = 1, /**< Waiting Start event. */
    PROV_STATE_WAIT_JOINING = 2, /**< Waiting node joining network. */
    PROV_STATE_START = 3,  /**< Sending Start packet. */
    PROV_STATE_WAIT_DATA = 4,  /**< Waiting Data packet from server. */
    PROV_STATE_WAIT_ACK_SENT = 5 /**< Waiting Ack packet send event. */
} provisioning_state_e;

/** \brief Provisioning events. */
static struct
{
    uint8_t rxd_packet:1; /**< Packet received on provisioning endpoints. */
    uint8_t timeout:1; /**< Timeout event occured. */
    uint8_t start:1; /**< Application started provisioning. */
    uint8_t ack_sent:1; /**< Ack packet has been sent. */
    uint8_t joined:1; /**< Node joined a new network. */
} m_events;

/** Hold how many retries are left for provisioning. */
static uint8_t m_retry;
/** Current tiemout for timeout task. */
static uint32_t m_timeout_ms;
/** Current session id. */
static uint8_t m_session_id;
/** The state of the provisioning state machine. */
static provisioning_state_e m_state = PROV_STATE_UNINIT;
/** Provisioning packet received filter and callback. */
static shared_data_item_t m_ptk_received_item;
/** Copy of the configuration passed to provisioning library. */
static provisioning_conf_t m_conf;
/** AES initialization vector. */
static uint8_t m_iv[AES_128_KEY_BLOCK_SIZE];

//Static memory could be optimized by giving memory management to application.
/** Buffer used to store received packets. */
static uint8_t m_data_buffer[PROV_PDU_SIZE];
/** Length of last received packet. */
static uint8_t m_data_length;
/** Joining result. */
static provisioning_res_e m_joining_res;

/** Function forward declaration. */
static uint32_t run_state_machine(void);
static void reset_provisioning(void);

/**
 * \brief   Get the destination address. When joining send to next hop
 *          that will forward to Sink.
 * \return  The packet destination address.
 */
static app_addr_t get_dest_address(void)
{
    app_lib_state_route_info_t info;

    lib_state->getRouteInfo(&info);
    return info.next_hop;
}

/**
 * \brief   Get the node address.
 * \return  The node address.
 */
static app_addr_t get_node_address(void)
{
    app_addr_t addr;

    lib_settings->getNodeAddress(&addr);

    return addr;
}

/**
 * \brief   Provisioning packet received callback. Raises rxd_packet.
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
    LOG(LVL_INFO, "Event : RXD PACKET.");
    m_events.rxd_packet = 1;

    if (data->num_bytes > PROV_PDU_SIZE)
    {
        m_data_length = PROV_PDU_SIZE;
    }
    else
    {
        m_data_length = data->num_bytes;
    }

    memcpy(&m_data_buffer,data->bytes,m_data_length);

    if (App_Scheduler_addTask_execTime(run_state_machine,
                              APP_SCHEDULER_SCHEDULE_ASAP,
                              500) != APP_SCHEDULER_RES_OK)
    {
        reset_provisioning();
        m_conf.end_cb(PROV_RES_ERROR_INTERNAL);
        LOG(LVL_ERROR, "Event : RXD PACKET, Error adding task.");
    }

    return APP_LIB_DATA_RECEIVE_RES_HANDLED;
}

/**
 * \brief Task called when a timeout expires. Raises the timeout event.
 */
static uint32_t timeout_task(void)
{
    LOG(LVL_INFO, "Event : TIMEOUT.");
    m_events.timeout = 1;
    if (App_Scheduler_addTask_execTime(run_state_machine,
                              APP_SCHEDULER_SCHEDULE_ASAP,
                              500) != APP_SCHEDULER_RES_OK)
    {
        reset_provisioning();
        m_conf.end_cb(PROV_RES_ERROR_INTERNAL);
        LOG(LVL_ERROR, "Event : TIMEOUT, Error adding task.");
    }

    return APP_SCHEDULER_STOP_TASK;
}

/**
 * \brief The packet sent callback. Raises the ack_sent event.
 */
static void packet_sent_cb(const app_lib_data_sent_status_t * status)
{
    LOG(LVL_INFO, "Event : ACK SENT.");
    /* No need to check if packet has been sent successfully. */
    m_events.ack_sent = 1;
    if (App_Scheduler_addTask_execTime(run_state_machine,
                              APP_SCHEDULER_SCHEDULE_ASAP,
                              500) != APP_SCHEDULER_RES_OK)
    {
        reset_provisioning();
        m_conf.end_cb(PROV_RES_ERROR_INTERNAL);
        LOG(LVL_ERROR, "Event : ACK SENT, Error adding task.");
    }
}

/**
 * \brief   The end joining callback. Raises joined event.
 * \param   result
 *          Result of the joining process.
 */
static void end_joining_cb(provisioning_res_e result)
{
    LOG(LVL_INFO, "Event : JOINING END.", result);

    m_joining_res = result;
    m_events.joined = 1;
    if (App_Scheduler_addTask_execTime(run_state_machine,
                              APP_SCHEDULER_SCHEDULE_ASAP,
                              500) != APP_SCHEDULER_RES_OK)
    {
        reset_provisioning();
        m_conf.end_cb(PROV_RES_ERROR_INTERNAL);
        LOG(LVL_ERROR, "Event : JOINING END, Error adding task.");
    }
}



/**
 * \brief   Helper function that send an ACK packet.
 * \return  \ref APP_SCHEDULER_STOP_TASK packet is sent successfully,
 *          \ref APP_SCHEDULER_SCHEDULE_ASAP in case of error.
 */
static uint32_t send_ack_packet(void)
{
    app_lib_data_send_res_e res;
    pdu_prov_data_ack_t ack_data =
    {
        .pdu_header =
        {
            .type = PROV_PACKET_TYPE_DATA_ACK,
            .address = get_node_address(),
            .session_id = m_session_id
        }
    };

    app_lib_data_to_send_t data_to_send =
    {
        .bytes = (uint8_t *)&ack_data,
        .num_bytes = sizeof(pdu_prov_data_ack_t),
        .dest_address = get_dest_address(),
        .delay = 0,
        .qos = APP_LIB_DATA_QOS_HIGH,
        .flags = APP_LIB_DATA_SEND_FLAG_NONE,
        .src_endpoint = PROV_UPLINK_EP,
        .dest_endpoint = PROV_DOWNLINK_EP
    };

    /* ACK is optional and sent only to avoid a timeout on the server side.
     * So if it fails move to next state.
     */
    res = Shared_Data_sendData(&data_to_send, packet_sent_cb);
    if (res != APP_LIB_DATA_SEND_RES_SUCCESS)
    {
        LOG(LVL_WARNING, "State WAIT_DATA : Error sending ACK (res:%d).", res);
        return APP_SCHEDULER_SCHEDULE_ASAP;
    }
    else
    {
        /* Set a timeout in case the packets takes to much time
            * to be sent.
            */
        m_timeout_ms = m_conf.timeout_s * 1000;
        if (App_Scheduler_addTask_execTime(timeout_task,
                                    m_timeout_ms,
                                    500) != APP_SCHEDULER_RES_OK)
        {
            reset_provisioning();
            m_conf.end_cb(PROV_RES_ERROR_INTERNAL);
            LOG(LVL_ERROR, "State WAIT_DATA : Error adding task.");
        }
    }

    return APP_SCHEDULER_STOP_TASK;
}

/**
 * \brief  Decrypt/Authenticate (if needed) the data packet and decode the
 *         CBOR buffer to get Provisioning DATA.
 * \return \ref PROV_RES_SUCCESS if received provisioning DATA is valid.
 */
provisioning_res_e process_data_packet(void)
{
    provisioning_res_e res = PROV_RES_SUCCESS;
    pdu_prov_t * pdu = (pdu_prov_t *) &m_data_buffer;

    provisioning_data_conf_t data_conf =
    {
        .end_cb = NULL,
        .user_data_cb = NULL,
        /* This is only possible because this function can't be preempted by
        * packet reception callback.
        */
        .buffer = (uint8_t *)&m_data_buffer + PROV_DATA_OFFSET,
        .length = m_data_length - PROV_DATA_OFFSET
    };

    LOG(LVL_DEBUG, "State WAIT_DATA : Received DATA packet:");
    LOG_BUFFER(LVL_DEBUG, m_data_buffer, m_data_length);

    if (m_conf.method == PROV_METHOD_UNSECURED)
    {
        if (pdu->data.key_index == 0)
        {
            /* Nohting to do. */
            res = PROV_RES_SUCCESS;
        }
        else
        {
            /* Key index need to be 0 for unsecured method. */
            res = PROV_RES_INVALID_PACKET;

            LOG(LVL_WARNING,
                "State WAIT_DATA : key index not null (unsecured method).");
        }
    }

    else if (m_conf.method == PROV_METHOD_SECURED)
    {
        if (pdu->data.key_index == 1)
        {
            /* Decrypt the data buffer before CBOR decoding. */
            aes_data_stream_t data_stream;
            aes_omac1_state_t omac1_state;
            uint32_t icb[AES_128_KEY_BLOCK_SIZE/4];
            uint32_t temp;
            uint8_t mic[PROV_MIC_SIZE];

            data_conf.length -= PROV_MIC_SIZE;

            /* Initialize counter. */
            memcpy(icb, m_iv, AES_128_KEY_BLOCK_SIZE);
            temp = icb[0] + pdu->data.counter;
            if (temp < icb[0])
            {
                if (++(icb[1]) == 0)
                {
                    if (++(icb[2]) == 0)
                    {
                        ++(icb[3]);
                    }
                }
            }
            icb[0] = temp;

            LOG(LVL_DEBUG, "State WAIT_DATA - Ctr: %d, - IV:",
                            pdu->data.counter);
            LOG_BUFFER(LVL_DEBUG, m_iv, AES_128_KEY_BLOCK_SIZE);
            LOG(LVL_DEBUG, "State WAIT_DATA - ICB:");
            LOG_BUFFER(LVL_DEBUG, ((uint8_t*)icb), AES_128_KEY_BLOCK_SIZE);

            /* Decrypt DATA + MIC. */
            aes_setupStream(&data_stream,
                            &m_conf.key[ENC_KEY_OFFSET],
                            (uint8_t*)icb);
            aes_crypto128Ctr(&data_stream,
                                data_conf.buffer,
                                data_conf.buffer, // overwrite input buffer
                                data_conf.length + PROV_MIC_SIZE);

            /* Authenticate whole packet (Hdr + Key idx + Ctr + Data). */
            aes_initOmac1(&omac1_state, m_conf.key);
            aes_omac1(&omac1_state,
                    mic,
                    PROV_MIC_SIZE,
                    m_data_buffer,
                    m_data_length - PROV_MIC_SIZE);

            if (memcmp(mic,
                &data_conf.buffer[data_conf.length],
                PROV_MIC_SIZE))
            {
                LOG(LVL_ERROR, "State WAIT_DATA : MIC doesn't match.");
                res = PROV_RES_INVALID_DATA;
            }
            else
            {
                LOG(LVL_INFO, "State WAIT_DATA : Packet decrypted and "
                            "authenticated.");
            }
        }
        else
        {
            res = PROV_RES_INVALID_DATA;
            /* Only factory key (idx=1) is supported. */
            LOG(LVL_WARNING, "State WAIT_DATA : Only key index = 1 "
                             "is supported (secured method).");
        }
    }
    else
    {
        res = PROV_RES_INVALID_DATA;
        LOG(LVL_WARNING, "State WAIT_DATA : Invalid method.");
    }


    if (res == PROV_RES_SUCCESS &&
        Provisioning_Data_decode(&data_conf, true) != PROV_RET_OK)
    {
        LOG(LVL_WARNING, "State WAIT_DATA : Invalid provisioning data.");
        res = PROV_RES_INVALID_DATA;
    }

    return res;
}

static void reset_provisioning(void)
{
    Shared_Data_removeDataReceivedCb(&m_ptk_received_item);
    App_Scheduler_cancelTask(timeout_task);
    App_Scheduler_cancelTask(run_state_machine);
    m_state = PROV_STATE_IDLE;
    memset(&m_events,0,sizeof(m_events));
    lib_system->setShutdownCb(NULL);
    Provisioning_Joining_stop();
}

/**
 * \brief   The Idle state function.
 * \return  Time in ms to schedule tthe state machine again.
 */
static uint32_t state_idle(void)
{
    uint32_t new_delay_ms = APP_SCHEDULER_STOP_TASK;

    if (m_events.start)
    {
        m_state = PROV_STATE_WAIT_JOINING;
        m_retry = 0;

        if (Provisioning_Joining_start() != PROV_RET_OK)
        {
            reset_provisioning();
            m_conf.end_cb(PROV_RES_ERROR_JOINING);
            LOG(LVL_ERROR, "State : IDLE, Error starting joining.");
        }
        else
        {
            /* Generate IV for Secured method. The same IV will be used even
             * for retries.
             */
            if (m_conf.method == PROV_METHOD_SECURED)
            {
                for(int i=0; i < AES_128_KEY_BLOCK_SIZE; i++)
                {
                    m_iv[i] = Random_get8();
                }
            }
        }
    }

    Shared_Data_removeDataReceivedCb(&m_ptk_received_item);
    App_Scheduler_cancelTask(timeout_task);
    memset(&m_events,0,sizeof(m_events));

    return new_delay_ms;
}

/**
 * \brief   The Wait joining network state function.
 * \return  Time in ms to schedule tthe state machine again.
 */
static uint32_t state_wait_joining(void)
{
    uint32_t new_delay_ms = APP_SCHEDULER_STOP_TASK;

    if (m_events.joined)
    {
        if (m_joining_res == PROV_RES_SUCCESS)
        {
            m_state = PROV_STATE_START;
            /* Increment at each provisioning session start. */
            m_session_id++;
            new_delay_ms = APP_SCHEDULER_SCHEDULE_ASAP;
            LOG(LVL_INFO, "State WAIT JOINING : Network joined.");
        }
        else
        {
            LOG(LVL_WARNING,
                "State WAIT JOINING : Error joining network (res: %d). "
                "END Provisioning.",
                m_joining_res);

            reset_provisioning();
            m_conf.end_cb(m_joining_res);
        }
    }

    memset(&m_events,0,sizeof(m_events));

    return new_delay_ms;
}

/**
 * \brief   The Start state function.
 * \return  Time in ms to schedule tthe state machine again.
 */
static uint32_t state_start(void)
{
    app_lib_data_send_res_e res;
    uint32_t new_delay_ms = APP_SCHEDULER_STOP_TASK;

    pdu_prov_start_t start_data =
    {
        .pdu_header =
        {
            .type = PROV_PACKET_TYPE_START,
            .address = get_node_address(),
            .session_id = m_session_id
        },
        .method = m_conf.method
    };

    /* Copy UID and IV to Start packet structure. */
    memcpy(&start_data.uid,m_conf.uid,m_conf.uid_len);
    memcpy(&start_data.iv,m_iv,AES_128_KEY_BLOCK_SIZE);

    app_lib_data_to_send_t data_to_send =
    {
        .bytes = (uint8_t *)&start_data,
        .num_bytes = sizeof(pdu_prov_hdr_t) +
                     1 +
                     m_conf.uid_len +
                     AES_128_KEY_BLOCK_SIZE,
        .dest_address = get_dest_address(),
        .delay = 0,
        .qos = APP_LIB_DATA_QOS_HIGH,
        .flags = APP_LIB_DATA_SEND_FLAG_NONE,
        .src_endpoint = PROV_UPLINK_EP,
        .dest_endpoint = PROV_DOWNLINK_EP
    };

    LOG(LVL_INFO, "State START : Send START packet (method: %d, timeout: %d).",
                    m_conf.method, m_conf.timeout_s);

    res = Shared_Data_sendData(&data_to_send, NULL);
    if (res != APP_LIB_DATA_SEND_RES_SUCCESS)
    {
        LOG(LVL_WARNING, "State START : Error sending packet (res:%d).", res);
        m_retry++;

        if (m_retry > m_conf.nb_retry)
        {
            LOG(LVL_ERROR, "State START : Too many retry; END.");
            reset_provisioning();
            m_conf.end_cb(PROV_RES_ERROR_SENDING_DATA);
        }
        else
        {
            LOG(LVL_WARNING, "State START : Resend START in %dms.",
                                DELAY_RESEND_PACKET_MS);
            new_delay_ms = DELAY_RESEND_PACKET_MS;
        }
    }
    else
    {
        Shared_Data_addDataReceivedCb(&m_ptk_received_item);
        m_timeout_ms = m_conf.timeout_s * 1000;
        if (App_Scheduler_addTask_execTime(timeout_task,
                                    m_timeout_ms,
                                    500) != APP_SCHEDULER_RES_OK)
        {
            reset_provisioning();
            m_conf.end_cb(PROV_RES_ERROR_INTERNAL);
            LOG(LVL_ERROR, "State START : Error adding task.");
        }
        m_state = PROV_STATE_WAIT_DATA;
    }

    memset(&m_events,0,sizeof(m_events));

    return new_delay_ms;
}

/**
 * \brief   The Wait data packet state function.
 * \return  Time in ms to schedule tthe state machine again.
 */
static uint32_t state_wait_data(void)
{
    uint32_t new_delay_ms = APP_SCHEDULER_STOP_TASK;
    pdu_prov_t * pdu = (pdu_prov_t *) &m_data_buffer;
    provisioning_res_e res = PROV_RES_SUCCESS;

    if (m_events.rxd_packet)
    {
        /* Check session Id. */
        if(pdu->pdu_header.session_id != m_session_id)
        {
            res = PROV_RES_INVALID_PACKET;

            LOG(LVL_WARNING, "State WAIT_DATA : Invalid session Id.");
        }
        else if(pdu->pdu_header.type == PROV_PACKET_TYPE_NACK)
        {
            /* No need to retry, Node is not authorized by server. */
            m_retry = m_conf.nb_retry;
            res = PROV_RES_NACK;

            LOG(LVL_ERROR, "State WAIT_DATA : Received NACK (res:%d).",
                             pdu->nack.nack_type);
        }
        else if (pdu->pdu_header.type != PROV_PACKET_TYPE_DATA)
        {
            /* Not a DATA packet. */
            res = PROV_RES_INVALID_PACKET;

            LOG(LVL_WARNING, "State WAIT_DATA : Not a DATA packet.");
        }
        else
        {
            /* This is a DATA packet. */
            res = process_data_packet();
        }

        if (res == PROV_RES_SUCCESS)
        {
            m_state = PROV_STATE_WAIT_ACK_SENT;
            Shared_Data_removeDataReceivedCb(&m_ptk_received_item);
            App_Scheduler_cancelTask(timeout_task);

            LOG(LVL_INFO, "WAIT_DATA : Valid data packet; Send ACK.");

            new_delay_ms = send_ack_packet();
        }
    }
    else if (m_events.timeout)
    {
        LOG(LVL_WARNING, "State WAIT_DATA : Timeout.");
        res = PROV_RES_TIMEOUT;
    }

    if (res != PROV_RES_SUCCESS)
    {
        m_retry++;

        if (m_retry > m_conf.nb_retry)
        {
            LOG(LVL_ERROR, "State WAIT_DATA :  Too many retry; END.");
            reset_provisioning();
            m_conf.end_cb(res);
        }
        else
        {
            /* Double the timeout if no response from server. */
            m_conf.timeout_s = m_conf.timeout_s * 2;

            /* Resend start. */
            LOG(LVL_WARNING, "State WAIT_DATA : Retry %d/%d, Resend START.",
                             m_retry, m_conf.nb_retry);
            m_state = PROV_STATE_START;

            Shared_Data_removeDataReceivedCb(&m_ptk_received_item);
            App_Scheduler_cancelTask(timeout_task);
            new_delay_ms = APP_SCHEDULER_SCHEDULE_ASAP;
        }
    }

    memset(&m_events,0,sizeof(m_events));

    return new_delay_ms;
}

/**
 * \brief   The Wait ack sent state function.
 * \return  Time in ms to schedule tthe state machine again.
 */
static uint32_t state_wait_ack_sent(void)
{
    uint32_t new_delay_ms = APP_SCHEDULER_STOP_TASK;

    if (m_events.ack_sent || m_events.timeout)
    {
        provisioning_data_conf_t data_conf =
        {
            .end_cb = m_conf.end_cb,
            .user_data_cb = m_conf.user_data_cb,
            /* Possible because data is copied to m_data_buffer and packet
             * callback is disabled in the previous state.
             */
            .buffer = (uint8_t *)&m_data_buffer + PROV_DATA_OFFSET,
            .length = m_data_length - PROV_DATA_OFFSET
        };


        /* Remove MIC size to DATA length if it was encrypted. */
        if (m_conf.method == PROV_METHOD_SECURED)
        {
            data_conf.length -= PROV_MIC_SIZE;
        }

        App_Scheduler_cancelTask(timeout_task);

        if (Provisioning_Data_decode(&data_conf, false) != PROV_RET_OK)
        {
            /* Should not happen verify is called in the previous state. */
            LOG(LVL_ERROR, "state WAIT_ACK_SENT : Invalid data.");
            reset_provisioning();
            m_conf.end_cb(PROV_RES_INVALID_DATA);
        }
        else
        {
            LOG(LVL_INFO, "State WAIT_ACK_SENT : Provisioning end "
                          "with success.");
            reset_provisioning();
        }
    }

    memset(&m_events,0,sizeof(m_events));

    return new_delay_ms;
}

/**
 * \brief   The provisioning state machine. React to events raised in this
 *          module.
 * \note    This function uses the fact the system is not preemptive and that
 *          all events are raised from non interrupt context.
 * \return  Time in ms to schedule this function again.
 */
static uint32_t run_state_machine(void)
{
    uint32_t new_delay_ms = APP_SCHEDULER_STOP_TASK;

    LOG(LVL_DEBUG, "Enter state machine :");
    LOG(LVL_DEBUG, " - state : %d",m_state);
    LOG(LVL_DEBUG, " - events : %s%s%s%s%s",
                   m_events.rxd_packet ? "rxd_packet" : "",
                   m_events.timeout ? " timeout" : "",
                   m_events.start ? " start" : "",
                   m_events.ack_sent ? " ack_sent" : "",
                   m_events.joined ? " joined" : "");

    switch (m_state)
    {
        case PROV_STATE_IDLE:
        {
            new_delay_ms = state_idle();
            break;
        }

        case PROV_STATE_WAIT_JOINING:
        {
            new_delay_ms = state_wait_joining();
            break;
        }

        case PROV_STATE_START:
        {
            new_delay_ms = state_start();
            break;
        }

        case PROV_STATE_WAIT_DATA:
        {
            new_delay_ms = state_wait_data();
            break;
        }

        case PROV_STATE_WAIT_ACK_SENT:
        {
            new_delay_ms = state_wait_ack_sent();
            break;
        }

        default:
        {
            break;
        }
    }

    return new_delay_ms;
}

provisioning_ret_e Provisioning_init(provisioning_conf_t * conf)
{
    provisioning_ret_e ret;

    LOG(LVL_DEBUG, "%s, Configuration:",__func__);
    LOG(LVL_DEBUG, " - Method: %d", conf->method);
    LOG(LVL_DEBUG, " - Nb Retries: %d", conf->nb_retry);
    LOG(LVL_DEBUG, " - Timeout (sec): %d", conf->timeout_s);
    LOG(LVL_DEBUG, " - Node UID:");
    LOG_BUFFER(LVL_DEBUG, conf->uid, conf->uid_len);
    LOG(LVL_DEBUG, " - Factory Key:");
    LOG_BUFFER(LVL_DEBUG, conf->key, conf->key_len);

    if (m_state != PROV_STATE_UNINIT && m_state != PROV_STATE_IDLE)
    {
        LOG(LVL_ERROR, "Init, PROV_RET_INVALID_STATE, (state:%d)", m_state);
        return PROV_RET_INVALID_STATE;
    }

    /* Parameters check. */
    if ((conf->method == PROV_METHOD_UNSECURED && conf->key_len != 0) ||
        (conf->method == PROV_METHOD_SECURED && conf->key_len != 32) ||
        conf->timeout_s == 0 ||
        conf->uid == NULL ||
        conf->uid_len == 0 ||
        conf->beacon_joining_cb == NULL)
    {
        LOG(LVL_ERROR, "Init, PROV_RET_INVALID_PARAM");
        return PROV_RET_INVALID_PARAM;
    }

    memcpy(&m_conf,conf,sizeof(provisioning_conf_t));

    m_ptk_received_item.filter.mode = SHARED_DATA_NET_MODE_UNICAST;
    m_ptk_received_item.filter.src_endpoint = PROV_DOWNLINK_EP;
    m_ptk_received_item.filter.dest_endpoint = PROV_UPLINK_EP;
    m_ptk_received_item.cb = pkt_received_cb;

    Random_init(getUniqueId() ^ lib_time->getTimestampHp());
    m_session_id = Random_get8();

    provisioning_joining_conf_t joining_conf =
    {
        .joining_cb = m_conf.beacon_joining_cb,
        .end_cb = end_joining_cb,
        .nb_retry = m_conf.nb_retry
    };

    ret = Provisioning_Joining_init(&joining_conf);
    if (ret != PROV_RET_OK)
    {
        return ret;
    }

    m_state = PROV_STATE_IDLE;

    return PROV_RET_OK;
}

provisioning_ret_e Provisioning_start(void)
{
    if (m_state != PROV_STATE_IDLE)
    {
        LOG(LVL_ERROR, "%s : PROV_RET_INVALID_STATE", __func__);
        return PROV_RET_INVALID_STATE;
    }

    if (App_Scheduler_addTask_execTime(run_state_machine,
                              APP_SCHEDULER_SCHEDULE_ASAP,
                              500) != APP_SCHEDULER_RES_OK)
    {
        reset_provisioning();
        m_conf.end_cb(PROV_RES_ERROR_INTERNAL);
        LOG(LVL_ERROR, "%s : Error adding task.", __func__);
        return PROV_RET_INTERNAL_ERROR;
    }
    else
    {
        m_events.start = 1;
        LOG(LVL_INFO, "Event : START");
        return PROV_RET_OK;
    }
}

provisioning_ret_e Provisioning_stop(void)
{
    /* Special case for stop event. Whatever the state is, stop
     * the provisioning session.
     */
    reset_provisioning();
    LOG(LVL_INFO, "Provisioning stopped.");
    m_conf.end_cb(PROV_RES_STOPPED);

    return PROV_RET_OK;
}