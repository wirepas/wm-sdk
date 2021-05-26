/* Copyright 2021 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/*
 * \file    app.c
 * \brief   Headnode inventory application using directed-advertiser
 */

#include <string.h>

#include "api.h"
#include "node_configuration.h"
#include "power.h"
#include "advertiser.h"
#include "app_scheduler.h"
#include "pack.h"
#include "fifo.h"
#include "common.h"
#include "shared_appconfig.h"


/** Role settings for Advertiser node */
#define NODE_HEADNODE_LL_ROLE \
    app_lib_settings_create_role(APP_LIB_SETTINGS_ROLE_HEADNODE,APP_LIB_SETTINGS_ROLE_FLAG_LL)

#define SEND_CHECK_PERIOD_MS 500

// Source endpoint to send advertiser data
#define DIRADV_EP_SRC_DATA 248

typedef struct {
    uint32_t src_addr;
    uint32_t rcv_time;
    adv_tag_data_t data;
} adv_data_type0_t;

typedef struct __attribute__ ((packed))
{
    uint8_t src_addr[NODE_ADDRESS_SIZE];
    union {
        int8_t rssi;  //these fields have the same size;
        uint8_t otap_seq;
        uint8_t voltage;
        uint8_t seq;
    };
} adv_data_type1_t;

typedef struct __attribute__ ((packed))
{
    uint8_t type;
    uint8_t msg_count;
    adv_data_type1_t data[MSG_TYPE2_MAX_ITEMS];
}adv_data_type2_t;

typedef struct
{
    uint8_t ack_data[TAG_APP_CFG_SIZE];
    uint16_t send_check_ms;
    uint32_t send_timeout_us;
    uint8_t payload_type;
} settings_t;

typedef struct  __attribute__ ((packed))
{
    uint8_t magic;
    uint8_t ack_data[TAG_APP_CFG_SIZE];
    uint8_t send_check; // x10 ms
    uint8_t send_timeout; // x100ms
    uint8_t payload_type;
} app_config_data_t;


#define TYPE0_BUF_MAX 150
#define TYPE0_TIMEOUT_US (2 * 1000 * 1000)
#define SEND_CHECK_SCALLING 10
#define SEND_TIMEOUT_SCALLING 100000
#define RSSI_TO_dBm(x)(x/2-127)

static fifo_t m_fifo_type0;
static adv_data_type0_t m_data_type0[TYPE0_BUF_MAX];
static settings_t m_settings;
static app_lib_data_send_res_e res __attribute__((unused));
static uint8_t m_msg_count = 0;
static uint32_t m_rcv_count = 0;
static uint16_t m_appcfg_filter_id[2];




/**
 *  @brief  Callback for injecting ACK data
 *  @param[in]  in
 *              information on incoming packet
 *  @param[out] out
 *              generated ack
 *  @return     true: ack generated
 *
**/

static bool appAckDataCb(const ack_gen_input_t * in, ack_gen_output_t * out)
{
    (void) in;
    out->data = &m_settings.ack_data;
    out->length = sizeof(m_settings.ack_data);
    return true;
}

/**
 *  @brief  Callback for processing application configuration
 *  @param[in]  bytes
 *              pointer to application configuration data
 *  @param[in]  seq
 *              Application configuration sequence number
 *  @param[in]  interval
*               diagnostic update interval
 *  @param[out]  void
 *
**/

static void appCfgDataCb(uint16_t type, uint8_t length, uint8_t * bytes)
{
    app_config_data_t app_data;
    uint8_t magic = *bytes;
    bool valid = false;

    if (type == INVENTORY_APPCFG_TLV_TYPE)
    {
        valid = (magic == 0xAD) && (length == sizeof(app_config_data_t));
    }
#ifdef ENABLE_LEGACY_APPCFG
    else if (type == SHARED_APP_CONFIG_INCOMPATIBLE_FILTER)
    {
     valid = (magic == 0xAD) && (sizeof(app_config_data_t) <= sizeof(app_config_data_t));
    }
#endif

    if (valid)
    {
        memcpy(&app_data, bytes, sizeof(app_config_data_t));
        memcpy(m_settings.ack_data, app_data.ack_data, sizeof(m_settings.ack_data));

        if (app_data.send_check > 0)
        {
            m_settings.send_check_ms = app_data.send_check * SEND_CHECK_SCALLING;
        }
        if (app_data.send_timeout > 0)
        {
            m_settings.send_timeout_us = app_data.send_timeout * SEND_TIMEOUT_SCALLING;
        }
        switch (app_data.payload_type)
        {
            case ADV_TYPE2:
            case ADV_TYPE3:
            case ADV_TYPE4:
            case ADV_TYPE5:
            case ADV_TYPE6:
                m_settings.payload_type = app_data.payload_type;
                break;
            default:  //setting not changed
                break;
        }

    }
    else
    {
    }
}

/**
 *  @brief Data reception callback
 *  @param[in]  data
 *              received data pointer
 *  @param[out] status code
 *
**/
static
app_lib_data_receive_res_e dataReceivedCb(const app_lib_data_received_t * data)
{
    if ((data->src_endpoint == DIRADV_EP_SRC_DATA) &&
        (data->dest_endpoint == DIRADV_EP_DEST) &&
        (data->num_bytes > 1))
    {
        uint8_t type = *data->bytes;
        m_rcv_count++;
        switch (type)
        {
            case ADV_TYPE0:
            {
                if (data->num_bytes >= sizeof(adv_tag_data_t))
                {
                    adv_data_type0_t r;
                    r.src_addr = data->src_address;
                    r.rcv_time = lib_time->getTimestampHp();
                    memcpy(&r.data, data->bytes, sizeof(adv_tag_data_t));
                    fifo_push(&m_fifo_type0, &r);
                }
                break;
            }
        }
    }
    return APP_LIB_DATA_RECEIVE_RES_HANDLED;
}

/**
 *  @brief Checks is timeout for sending data elapsed
 *  @param[in]  f
 *              pointer to FIFO structure
  *  @param[in] timeout
 *              timeout out value in usec
 *  @param[out] boolean
 *              true if timeout elapsed, false otherwise
 *
**/

static bool check_data_timeout(fifo_t * f, uint32_t timeout)
{
    adv_data_type0_t * r;
    uint32_t delta;
    r = (adv_data_type0_t *) fifo_get_tail(f);
    if (r != NULL)
    {
        delta = lib_time->getTimeDiffUs(r->rcv_time, lib_time->getTimestampHp());
        if (delta > timeout)
        {
            return true;
        }
    }
    return false;
}

/**
 *  @brief Constructs payload Type2 and Type3 payloads
 *  @param[in]  payload
 *              pointer to payload buffer
 *  @param[in]  payload_type
*               type of payload ADV_TYPE2 | ADV_TYPE3
 *  @param[out] size
 *              returns size of payload in bytes
 *
**/

static uint8_t  pack_msg( adv_data_type2_t * payload, uint8_t payload_type, uint32_t msg_count)
{
    if(fifo_size(&m_fifo_type0) == 0)
    {
        return 0;
    }

    uint8_t count = 0;
    adv_data_type0_t ad;
    adv_data_type1_t * pd;

    payload->type = payload_type | MSG_TYPE2_PREFIX;
    payload->msg_count = (uint8_t) (msg_count & 0xFF);

    while ( count < MSG_TYPE2_MAX_ITEMS )
    {
        if ( fifo_pop(&m_fifo_type0, &ad) )
            {

                pd = &payload->data[count];
                Pack_packLe(&pd->src_addr, ad.src_addr, NODE_ADDRESS_SIZE);

                switch ( payload_type )
                {
                    case ADV_TYPE2:
                        pd->rssi = ad.data.rssi;
                        break;
                    case ADV_TYPE3:
                         pd->otap_seq = ad.data.proc_otap_seq;
                         break;
                    case ADV_TYPE4:
                         pd->otap_seq = ad.data.stored_otap_seq;
                         break;
                    case ADV_TYPE5:
                         pd->voltage = ad.data.voltage;
                         break;
                    case ADV_TYPE6:
                         pd->seq = (uint8_t) (ad.data.seq & 0x00FF);
                         break;
                    default:
                        return 0;
                }
                count++;
            }
        else
        {
            break;
        }
    }
    return (sizeof(adv_data_type2_t) - sizeof(adv_data_type1_t) * (MSG_TYPE2_MAX_ITEMS - count));
}

/**
 *  @brief Sends the collected advertiser data; called periodically by the scheduler
 *  @param[in]  void
 *
 *  @param[out] time
 *              returns the time until the next execution in msec
**/

static uint32_t send_data(void)
{

    uint16_t len = fifo_size(&m_fifo_type0);

    if ( len == 0 || \
    ((len < MSG_TYPE2_MAX_ITEMS) && !check_data_timeout(&m_fifo_type0, m_settings.send_timeout_us)))
    {
        return m_settings.send_check_ms;
    }

    uint8_t payload[102];
    uint8_t payload_len = 0;

    m_msg_count++;


    switch (m_settings.payload_type)
    {
        case ADV_TYPE2:
        case ADV_TYPE3:
        case ADV_TYPE4:
        case ADV_TYPE5:
        case ADV_TYPE6:
            payload_len = pack_msg((adv_data_type2_t *) &payload, \
            m_settings.payload_type, m_msg_count);
        break;

        default:
            payload_len = 0;
    }

    if (payload_len == 0)
    {
        return m_settings.send_check_ms;
    }

    // Send the data packet
    app_lib_data_to_send_t data;
    app_lib_data_send_res_e res;
    data.bytes = (uint8_t*) &payload;
    data.num_bytes = payload_len;
    data.dest_address = APP_ADDR_ANYSINK;
    data.src_endpoint = DIRADV_AGR_EP;
    data.dest_endpoint = DIRADV_AGR_EP;
    data.qos = APP_LIB_DATA_QOS_HIGH;
    data.delay = 0;
    data.flags = APP_LIB_DATA_SEND_FLAG_TRACK;
    data.tracking_id = m_msg_count;
    res = lib_data->sendData(&data);
    (void) res;  // !!! add check for data send

    return m_settings.send_check_ms;
}

/**
 *  @brief Configures the node network parameters
 *  @param[in]
 *
 *  @param[out]  bool  true/flase -> success/failure
 *
**/
static bool node_init(void)
{
    bool ret = true;

    if (lib_settings->setNodeRole(NODE_HEADNODE_LL_ROLE)!= APP_RES_OK)
    {
        ret = false;
    }

    if (configureNodeFromBuildParameters() != APP_RES_OK)
    {
        ret = false;
    }
    return ret;
}

/**
 *  @brief Callback for data send
 *  @param[in] status
 *             Sent status
 *  @param[out]  void
 *
**/
static void sentDataCb(const app_lib_data_sent_status_t * status)
{
}

/**
 *  @brief Sets the callbacks
 *  @param[in] void
 *
 *  @param[out]  void
 *
**/
static void set_callbacks(void)
{
    lib_data->setDataReceivedCb(dataReceivedCb);
    lib_advertiser->setRouterAckGenCb(appAckDataCb);
    lib_data->setDataSentCb(sentDataCb);
}

/**
 *  @brief Main application function
 *  @param[in]  functions
 *              Stack API functions pointer
 *  @param[out] void
 *
**/
void App_init(const app_global_functions_t * functions)
{
    shared_app_config_filter_t app_config_filter;
    
    fifo_init(&m_fifo_type0, m_data_type0, sizeof(adv_data_type0_t), TYPE0_BUF_MAX);
    memset(m_settings.ack_data, 0, sizeof(m_settings.ack_data));
    m_settings.send_check_ms = SEND_CHECK_PERIOD_MS;
    m_settings.send_timeout_us = TYPE0_TIMEOUT_US;
    m_settings.payload_type = ADV_TYPE2;

    App_Scheduler_init();
    App_Scheduler_addTask(send_data, SEND_CHECK_PERIOD_MS);
    set_callbacks();

    Shared_Appconfig_init();
    app_config_filter.type = INVENTORY_APPCFG_TLV_TYPE;
    app_config_filter.cb = appCfgDataCb;
    app_config_filter.call_cb_always = false;

    if (Shared_Appconfig_addFilter(&app_config_filter, &m_appcfg_filter_id[0]) 
                                    != SHARED_APP_CONFIG_RES_OK)
    {
    }
#ifdef ENABLE_LEGACY_APPCFG
    //register for legacy mode also
    app_config_filter.type = SHARED_APP_CONFIG_INCOMPATIBLE_FILTER;
    if (Shared_Appconfig_addFilter(&app_config_filter, &m_appcfg_filter_id[1]) 
                                      != SHARED_APP_CONFIG_RES_OK)
    {
    }
#endif

    if(node_init())
    {
        lib_state->startStack();
    }
    else
    {
        lib_state->stopStack();
    }
}
