/**
 * @file       poslib_event.c
 * @brief      Main code to process the positioning callbacks.
 * @copyright  Wirepas Ltd. 2020
 */

#define DEBUG_LOG_MODULE_NAME "POSLIB_EVENT"
#ifdef DEBUG_POSLIB_LOG_MAX_LEVEL
#define DEBUG_LOG_MAX_LEVEL DEBUG_POSLIB_LOG_MAX_LEVEL
#else
#define DEBUG_LOG_MAX_LEVEL LVL_NOLOG
#endif
#include "debug_log.h"
#include <string.h>
#include "poslib.h"
#include "poslib_control.h"
#include "poslib_event.h"
#include "poslib_measurement.h"
#include "poslib_decode.h"
#include "shared_data.h"

/** true when ack is received and false after ack receive status is read */
static bool m_data_ack;

/**
 * \brief       Sends measurement data to stack.
 * \param       payload data payload generated
 * \param       pos_settings PosLib settings in use
 */
static void send_msg(app_lib_data_to_send_t * payload,
                     poslib_settings_t * pos_settings);

/**
 * @brief   Wrapper for reception of a data sent status callback
 * @param   status  The status
 */
static void cb_data_ack(const app_lib_data_sent_status_t * status)
{
    m_data_ack = true;
}

void PosLibEvent_ScanEnd(void)
{
    poslib_settings_t * pos_settings = Poslib_get_settings();
    size_t available_buffers;
    uint8_t len_payload = 0;
    const uint8_t * ptr_payload = 0;
    app_lib_data_to_send_t payload;
    poslib_measurements_e meas_type = DEFAULT_MEASUREMENT_TYPE_TAG;

    switch(pos_settings->node_mode)
    {
        case POSLIB_MODE_NRLS_TAG:
        case POSLIB_MODE_AUTOSCAN_TAG:
            meas_type = DEFAULT_MEASUREMENT_TYPE_TAG;
            break;
        case POSLIB_MODE_AUTOSCAN_ANCHOR:
        case POSLIB_MODE_OPPORTUNISTIC_ANCHOR:
            meas_type = DEFAULT_MEASUREMENT_TYPE_ANCHOR;
            break;
        default:
            meas_type = DEFAULT_MEASUREMENT_TYPE_TAG;
            break;
    }

    lib_data->getNumFreeBuffers(&available_buffers);

    if(PosLibMeas_getNumofBeacons() > 0 && available_buffers > 0 &&
       pos_settings->update_period_static_s > 0)
    {
        lib_system->enterCriticalSection();
        ptr_payload = PosLibMeas_initPayload();
        PosLibMeas_addPayloadRss(meas_type);
        len_payload = PosLibMeas_getPayloadLen();

        /** add voltage payload */
#if CONF_VOLTAGE_REPORT == 1
        if(!(len_payload + sizeof(poslib_meas_header_t) +
           sizeof(poslib_meas_payload_voltage_t) > MAX_PAYLOAD))
        {
            PosLibMeas_addVoltageToPayload();
            len_payload = PosLibMeas_getPayloadLen();
        }
#endif

        /** add payload to be sent */
        payload.bytes = ptr_payload;
        payload.num_bytes = len_payload;
        payload.dest_address = APP_ADDR_ANYSINK;
        payload.src_endpoint = pos_settings->source_endpoint;
        payload.dest_endpoint = pos_settings->destination_endpoint;
        payload.qos = APP_LIB_DATA_QOS_NORMAL;
        payload.delay = 0;
        payload.flags = APP_LIB_DATA_SEND_FLAG_TRACK;
        payload.tracking_id = PosLibCtrl_getSeqId();

        lib_system->exitCriticalSection();
        send_msg(&payload, pos_settings);
    }
    else // no beacons received yet
    {
        LOG(LVL_DEBUG, "\"msg\":\"no_neighbors\"");
    }
}

static void send_msg(app_lib_data_to_send_t * payload,
                     poslib_settings_t * pos_settings)
{
    app_lib_data_send_res_e rc;

    /** Clears ack status if not read */
    PosLibEvent_AckReceived();
    /** Sends message */
    rc = Shared_Data_sendData(payload, cb_data_ack);

    if (rc != APP_LIB_DATA_SEND_RES_SUCCESS)
    {
        LOG(LVL_ERROR, "\"failed_data_send\":%d", rc)
    }
}

void PosLibEvent_AppConfig(const uint8_t * bytes, uint32_t num_bytes)
{
    lib_system->enterCriticalSection();
    // acts on decoded information - if any
    if(PosLibDecode_config(bytes, num_bytes))
    {
        PosLibDecode_msg();
    }
    lib_system->exitCriticalSection();
    LOG(LVL_INFO, "\"received_app_config\"");
}


bool PosLibEvent_AckReceived(void)
{
    if (m_data_ack)
    {
        m_data_ack = false;
        return true;
    }

    return false;
}
