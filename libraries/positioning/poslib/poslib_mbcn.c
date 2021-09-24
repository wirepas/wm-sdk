/**
 * @file       poslib_mbcn.c
 * @brief      Implementation of the mini-beacon module
 * @copyright  Wirepas Ltd.2021
 */



#define DEBUG_LOG_MODULE_NAME "POSLIB_MBCN"
#ifdef DEBUG_POSLIB_LOG_MAX_LEVEL
#define DEBUG_LOG_MAX_LEVEL DEBUG_POSLIB_LOG_MAX_LEVEL
#else
#define DEBUG_LOG_MAX_LEVEL LVL_NOLOG
#endif

#include "debug_log.h"
#include <string.h>
#include "api.h"
#include "poslib.h"
#include "poslib_tlv.h"
#include "poslib_mbcn.h"
#include "app_scheduler.h"
#include "shared_data.h"

// Local mini-beacon (mbcn) state variables
static poslib_mbcn_payload_t m_payload;  //stores the mbcn payload
static app_lib_data_to_send_t m_mbcn;  //stores the mbcn data frame
static poslib_mbcn_config_t m_settings; // copy of the current mbcn settings
static bool m_started = false;

static uint32_t mbcn_task()
{
    app_lib_data_send_res_e rc;

    m_payload.seq++;
    rc = Shared_Data_sendData(&m_mbcn, NULL);

    if (rc != APP_LIB_DATA_SEND_RES_SUCCESS)
    {
        LOG(LVL_ERROR, "Mbcn send rc: %u, seq: %u", rc, m_payload.seq);
    }

    //FixMe: add randomization of send interval
    return m_settings.tx_interval_ms;
}

static uint8_t encode_mbcn(poslib_mbcn_config_t * settings, uint8_t * buf, uint8_t length)
{
    poslib_tlv_record rcd;
    poslib_tlv_item_t item;
    uint16_t features = 0;
    poslib_mbcn_record_t * record = settings->records;
    uint8_t records_len = sizeof(settings->records) / sizeof(settings->records[0]);
    rcd.buffer = buf;
    rcd.length = length;
    rcd.index = 0;

    // transmit rate
    item.type = POSLIB_MBCN_TX_INTERVAL;
    item.length = sizeof(settings->tx_interval_ms);
    item.value = (uint8_t *) &settings->tx_interval_ms;
    PosLibTlv_Encode_addItem(&rcd, &item);
    LOG(LVL_DEBUG, "MBCN interval: %u", rcd.index);

    // node feature flags  FixMe: fill the features flags
    item.type = POSLIB_MBCN_FEATURES;
    item.length = sizeof(features);
    item.value = (uint8_t *) &features;
    PosLibTlv_Encode_addItem(&rcd, &item);
    LOG(LVL_DEBUG, "MBCN features buf %u", rcd.index);

    // other records
    for (uint8_t i = 0; i < records_len; i++)
    {
        record = &settings->records[i];

        if (record->type != POSLIB_MBCN_INVALID_TYPE && 
            record->type <= POSLIB_MBCN_MAX_TYPE && 
            record->length != 0 && 
            record->length <= sizeof(record->value))
        {
            item.type = record->type;
            item.length = record->length;
            item.value = (uint8_t *) record->value;
            PosLibTlv_Encode_addItem(&rcd, &item);
            LOG(LVL_DEBUG, "MBCN add record. type: %u, len: %u, buf: %u", 
                        record->type, record->length, rcd.index);  
        }
    }
    LOG(LVL_DEBUG, "MBCN records: %u", rcd.index);
    return rcd.index;
}

void PosLibMbcn_stop()
{
    App_Scheduler_cancelTask(mbcn_task);
    m_started = false;
    LOG(LVL_ERROR, "Mini-beacon stopped"); 
}

bool PosLibMbcn_start(poslib_mbcn_config_t * settings)
{
    app_scheduler_res_e rc;

    m_settings = *settings;

    if (!settings->enabled)
    {
        m_started = false;
        return false;
    }
    
    if (!m_started)
    {
        // Initialize constant values in mini-beacon data sent structure
        m_mbcn.bytes = (uint8_t *) &m_payload;
        m_mbcn.dest_address = APP_ADDR_BROADCAST;
        m_mbcn.src_endpoint = POSLIB_MBCN_SRC_EP;
        m_mbcn.dest_endpoint = POSLIB_MBCN_DEST_EP;
        m_mbcn.qos = APP_LIB_DATA_QOS_NORMAL;
        m_mbcn.delay = 0;
        m_mbcn.flags = APP_LIB_DATA_SEND_NW_CH_ONLY;
        m_mbcn.tracking_id = 0;
        m_mbcn.hop_limit = 1;
    }
    
    // Fill payload content
    m_payload.seq = 0;
    m_mbcn.num_bytes = sizeof(m_payload.seq);  
    m_mbcn.num_bytes += encode_mbcn(settings, m_payload.data, sizeof(m_payload.data));

    //Add the task sending mini-beacons
    if (!m_started)
    {
        rc = App_Scheduler_addTask_execTime(mbcn_task, settings->tx_interval_ms, 50);
        if (rc== APP_SCHEDULER_RES_OK)
        {
            LOG(LVL_ERROR, "Mini-beacon started. Tx: %u length:%u bytes", settings->tx_interval_ms, m_mbcn.num_bytes);
            m_started = true;
        }
        else
        {
            LOG(LVL_ERROR, "Cannot add mini-beacon task: %u", rc);
            m_started = false;
        }
    }

    return m_started;
}

bool PosLibMbcn_decode(uint8_t * buf, uint8_t length, poslib_mbcn_data_t * mbcn)
{
    poslib_tlv_record rcd;
    poslib_tlv_item_t item;
    poslib_mbcn_record_t * record;
    poslib_mbcn_payload_t * payload = (poslib_mbcn_payload_t *) buf;
    poslib_tlv_res_e res;
    uint8_t i = 0;
    bool ret = true;

    if (buf == NULL || mbcn == NULL || length < sizeof(payload->seq))
    {
        LOG(LVL_ERROR, "Incorrect input parameters!");
        return false;
    }
    
    mbcn->seq = payload->seq;
    rcd.buffer = payload->data;
    rcd.length = length - sizeof(payload->seq);
    rcd.index = 0;

    // sets both type & length to 0 invalidating them
    memset(mbcn->records, 0, sizeof(mbcn->records));

    while (i < sizeof(mbcn->records))
    {
        res = PosLibTlv_Decode_getNextItem(&rcd, &item); 

        if (res == POSLIB_TLV_RES_OK)
        {
            switch (item.type)
            {
                case POSLIB_MBCN_TX_INTERVAL:
                {
                    if (item.length == sizeof(mbcn->tx_interval_ms))
                    {
                        memcpy(&mbcn->tx_interval_ms, item.value, item.length);
                    }
                    else
                    {
                        LOG(LVL_ERROR, "Incorrect tx interval length: %u", item.length);
                    }
                    break;
                }

                case POSLIB_MBCN_FEATURES:
                {
                    
                    if (item.length == sizeof(mbcn->features))
                    {
                        memcpy(&mbcn->features, item.value, item.length);
                    }
                    else
                    {
                        LOG(LVL_ERROR, "Incorrect features length: %u", item.length);
                    }
                    break;
                }

                case POSLIB_MBCN_AREA_ID:
                case POSLIB_MBCN_FLOOR_ID:
                case POSLIB_MBCN_X:
                case POSLIB_MBCN_Y:
                case POSLIB_MBCN_Z:
                case POSLIB_MBCN_CUSTOM_1:
                case POSLIB_MBCN_CUSTOM_2:
                case POSLIB_MBCN_CUSTOM_3:
                case POSLIB_MBCN_CUSTOM_4:
                {
                    record = &mbcn->records[i];
                    if (item.length <= sizeof(record->value))
                    {
                        memcpy(record->value, item.value, item.length);
                        record->type = item.type;
                        record->length = item.length;
                        i++;
                    }
                    break;
                }
                default:
                {
                    LOG(LVL_ERROR, "Unknown record type: %u", item.type);
                    break;
                }
            }
        }
        else
        {

            if (res == POSLIB_TLV_RES_ERROR)
            {
                LOG(LVL_ERROR, "Mbcn decoding error!");
                ret = false;
            }
            break; //end decoding on error | end of data
        }
    }
    return ret;
}