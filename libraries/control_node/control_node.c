/* Copyright 2020 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

#include <string.h>
#include <limits.h>

#include "control_node.h"
#include "time.h"
#include "app_scheduler.h"
#include "shared_data.h"
#include "tlv.h"

#define DEBUG_LOG_MODULE_NAME "CTR NODE"
#define DEBUG_LOG_MAX_LEVEL LVL_NOLOG
#include "debug_log.h"

/** \brief Maximum time_last_seen (sec) for a router to be selected as
 *         destination.
 */
#define NBOR_MAX_TIME_LAST_SEEN 60

/** \brief Max execution time for diagnostic task (max measured = 52us). */
#define DIAG_TASK_EXEC_TIME_US   60

/** \brief Maximum execution time for timeout task (max measured = 23us). */
#define TIMEOUT_TASK_EXEC_TIME_US   30

/** \brief Minimum period at which diagnostics can be sent. */
#define MIN_DIAG_PERIOD_MS 30000

/** \brief Returned by \ref get_da_router_address when no route is found. */
#define NO_ROUTE_FOUND_ADDRESS 0

/** \brief Control node state machine states. */
typedef enum
{
    /** Library is not initialized. */
    CTRL_STATE_UNINIT = 0,
    /** Idle. */
    CTRL_STATE_IDLE = 1,
    /** Packet is sent waiting ack or backup route retry. */
    CTRL_STATE_BUSY = 2,
} control_node_state_e;

/** The library state */
static control_node_state_e m_state = CTRL_STATE_UNINIT;

/** Internal structure to manage packet sending and its backup route. */
static struct
{
    /** Packet (or backup packet) has been sent. */
    bool sent;
    /** Packet (or backup packet) has been received. */
    bool ack;
    /** Sending process finished for primary route (or backup). */
    bool end;
    /** Router address used to send the packet. */
    app_addr_t address;
} m_send_handle[2];


/** Local copy of packet sent header. (needed by retry mechanism). */
static app_lib_data_to_send_t m_data;
/** Local copy of packet sent buffer. (needed by retry mechanism). */
static uint8_t m_data_buff[MAX_PAYLOAD];
/** Local copy of packet sent user callback. (needed by retry mechanism). */
static app_lib_data_data_sent_cb_f m_sent_cb;

/** Time the packet was first sent. */
static app_lib_time_timestamp_coarse_t m_time_sent;

/** Copy of the configuration passed to control node library. */
static control_node_conf_t m_conf;

/* Structure holding diagnostics data. */
static struct
{
    /** Number of diag packet sent with success. */
    uint16_t success;
    /** Number of diag packet sent with errors. */
    uint16_t error;
    /** Time last diag packet was sent. */
    uint32_t send_time_us;
} m_diag_info;

/** Forward declaration of functions. */
static uint32_t backup_route_task(void);
static void packet_sent_cb(const app_lib_data_sent_status_t * status);
static app_lib_data_receive_res_e ack_received (
                                        const shared_data_item_t * item,
                                        const app_lib_data_received_t * data);

/** Advertiser Ack packet received filter and callback. */
static shared_data_item_t m_ack_received_item = {
    .cb = ack_received,
    .filter = {
        .mode = SHARED_DATA_NET_MODE_UNICAST,
        .src_endpoint = DIRADV_EP_SRC_ACK,
        .dest_endpoint = DIRADV_EP_DEST,
        .multicast_cb = NULL
    }
};

/**
 * \brief       Returns the address of the best DA capable router.
 *              Router with the best RSSI and last_seen <
 *              \ref NBOR_MAX_TIME_LAST_SEEN. If none is found, Router with the
 *              best RSSI is selected.
 * \param       exclude
 *              Exclude this address from selectable routers. 0 if not used.
 * \return      The address of the router. 0 if none found.
 */
static app_addr_t get_da_router_address(app_addr_t exclude)
{
    #define MAX_NBORS   10
    uint32_t nb_da_nbors = 0;
    app_lib_state_nbor_info_t nbors_temp[MAX_NBORS];
    app_lib_state_nbor_list_t nbors_list =
    {
        .number_nbors = MAX_NBORS,
        .nbors = &nbors_temp[0],
    };
    int8_t max_rssi = INT8_MIN;
    int8_t max_rssi_within_window = INT8_MIN;
    uint8_t idx_rssi=MAX_NBORS;
    uint8_t idx_rssi_within_window=MAX_NBORS;

    lib_state->getNbors(&nbors_list); // Always return APP_RES_OK

    if(nbors_list.number_nbors)
    {
        for (uint32_t i = 0; i < nbors_list.number_nbors; i++)
        {
            /* Router is DA capable. */
            if (nbors_list.nbors[i].address != exclude &&
                nbors_list.nbors[i].diradv_support ==
                                                APP_LIB_STATE_DIRADV_SUPPORTED)
            {
                /* Find router with best RSSI with last_update in time
                 * window.
                 */
                if (nbors_list.nbors[i].last_update <
                                                    NBOR_MAX_TIME_LAST_SEEN &&
                    nbors_list.nbors[i].norm_rssi >  max_rssi_within_window)
                {
                    max_rssi_within_window = nbors_list.nbors[i].norm_rssi;
                    idx_rssi_within_window = i;
                }

                /* Fall back: Find router with best RSSI. */
                if (nbors_list.nbors[i].norm_rssi >  max_rssi)
                {
                    max_rssi = nbors_list.nbors[i].norm_rssi;
                    idx_rssi = i;
                }
                nb_da_nbors++;
            }
        }
    }

    if (idx_rssi_within_window != MAX_NBORS)
    {
        LOG(LVL_DEBUG, "Found up to date DA router "
                       "(@:%u, rssi:%d, last_up:%d, da_nbors:%d/%d).",
                       nbors_list.nbors[idx_rssi_within_window].address,
                       nbors_list.nbors[idx_rssi_within_window].norm_rssi,
                       nbors_list.nbors[idx_rssi_within_window].last_update,
                       nb_da_nbors,
                       nbors_list.number_nbors);
        return nbors_list.nbors[idx_rssi_within_window].address;
    }
    else if (idx_rssi != MAX_NBORS)
    {
        LOG(LVL_WARNING, "Fall back to best Rssi DA Router "
                         "(@:%u, rssi:%d, last_up:%d, da_nbors:%d/%d).",
                         nbors_list.nbors[idx_rssi].address,
                         nbors_list.nbors[idx_rssi].norm_rssi,
                         nbors_list.nbors[idx_rssi].last_update,
                         nb_da_nbors,
                         nbors_list.number_nbors);
        return nbors_list.nbors[idx_rssi].address;
    }
    else
    {
        LOG(LVL_ERROR, "No DA router found. (nb_nbors:%d)",
                                                    nbors_list.number_nbors);
        return NO_ROUTE_FOUND_ADDRESS;
    }
}

/**
 * \brief Safety timer is called. Reset send state machine.
 */
static uint32_t reset_task(void)
{
    App_Scheduler_cancelTask(backup_route_task);

    m_send_handle[0].end = true;
    m_send_handle[0].ack = true;
    m_send_handle[0].address = NO_ROUTE_FOUND_ADDRESS;

    m_send_handle[1].end = true;
    m_send_handle[1].ack = true;
    m_send_handle[1].address = NO_ROUTE_FOUND_ADDRESS;

    m_state = CTRL_STATE_IDLE;

    LOG(LVL_ERROR, "Reset timeout fired. Error sending packet.");

    if (m_sent_cb != NULL)
    {
        app_lib_data_sent_status_t status;
        memset(&status, 0, sizeof(app_lib_data_sent_status_t));
        status.success = false;

        LOG(LVL_INFO, "Call cb with success==false (%u)", m_sent_cb);
        m_sent_cb(&status);
        /* Set to NULL to avoid calling it a second time. */
        m_sent_cb = NULL;
    }
    else
    {
        LOG(LVL_DEBUG, "cb already called or NULL");
    }

    return APP_SCHEDULER_STOP_TASK;
}

/**
 * \brief Task called when a timeout expires. Try to send packet to backup
 *        route.
 */
static uint32_t backup_route_task(void)
{
    app_lib_data_send_res_e res;
    LOG(LVL_WARNING, "Timeout, send packet to backup router.");

    m_send_handle[1].sent = true;
    m_data.dest_address = m_send_handle[1].address;
    /* Add eleasped time since first try to travel time. */
    m_data.delay = lib_time->getTimestampCoarse() - m_time_sent;
    res = Shared_Data_sendData(&m_data, packet_sent_cb);
    if (res != APP_LIB_DATA_SEND_RES_SUCCESS)
    {
        m_send_handle[1].end = true;
        LOG(LVL_WARNING, "Error sending to backup router (res:%d)", res);
    }

    /* This should no be called. */
    if (m_send_handle[0].end == true && m_send_handle[1].end == true)
    {
        App_Scheduler_cancelTask(reset_task);
        app_lib_data_sent_status_t status;
        memset(&status, 0, sizeof(app_lib_data_sent_status_t));
        status.success = false;
        LOG(LVL_INFO, "Call cb with success==false (%u)", m_sent_cb);
        if (m_sent_cb != NULL)
        {
            m_sent_cb(&status);
            m_sent_cb = NULL;
        }
        m_state = CTRL_STATE_IDLE;
    }

    return APP_SCHEDULER_STOP_TASK;
}

/**
 * \brief Called when packet is sent.
 *
 * \param   status
 *          Status of the sent packet
 */
static void packet_sent_cb(const app_lib_data_sent_status_t * status)
{
    if (m_state != CTRL_STATE_BUSY)
    {
        /* Should not happen. */
        LOG(LVL_ERROR, "packet_sent_cb : Invalid state.");
        return;
    }

    LOG(LVL_DEBUG, "packet_sent_cb (to:%u, success:%d)", status->dest_address,
                                                         status->success);

    /* Is this first packet or backup packet sent callback ? */
    if (status->dest_address == m_send_handle[0].address)
    {
        m_send_handle[0].end = true;
        /* Cancel timeout. */
        App_Scheduler_cancelTask(backup_route_task);

        /* Is packet already sent to backup route ? */
        if (!m_send_handle[1].sent)
        {
           if (status->success) // Don't send the backup packet.
           {
                m_send_handle[1].end = true;
           }
           else  // First packet failed, Send to backup route imediatelly.
           {
                app_lib_data_send_res_e res;
                m_data.dest_address = m_send_handle[1].address;
                /* Add eleasped time since first try to travel time. */
                m_data.delay = lib_time->getTimestampCoarse() - m_time_sent;
                res = Shared_Data_sendData(&m_data, packet_sent_cb);
                m_send_handle[1].sent = true;
                LOG(LVL_WARNING, "Sending to primary router failed, try backup");
                if (res != APP_LIB_DATA_SEND_RES_SUCCESS)
                {
                    m_send_handle[1].end = true;
                    LOG(LVL_ERROR, "Error sending to backup router (res:%d)", res);
                }
           }
        }
        /* else Backup packet already sent; wait for reply. */
    }
    else if (status->dest_address == m_send_handle[1].address)
    {
        m_send_handle[1].end = true;
    }
    else
    {
        /* Should not happen. */
        LOG(LVL_ERROR, "Packet sent cb : cb from unexpected source.");
        m_send_handle[0].end = true;
        m_send_handle[0].ack = true;
        m_send_handle[1].end = true;
        m_send_handle[1].ack = true;
    }

    /* Call App callback in case of succesd or if the two packet send have
     * failed.
     */
    if (status->success ||
        (m_send_handle[0].end && m_send_handle[1].end))
    {

        if (m_sent_cb != NULL)
        {
            LOG(LVL_INFO, "Packet sent (success:%d, s1:%d, s2:%d); "
                          "call USER cb",
                          status->success,
                          m_send_handle[0].end,
                          m_send_handle[1].end);
            m_sent_cb(status);
            /* Set to NULL to avoid calling it a second time. */
            m_sent_cb = NULL;
        }
        else
        {
            LOG(LVL_DEBUG, "Packet sent; cb already called or NULL");
        }
    }

    /* Both packet have been sent and sent callback called. Nothing more to do. */
    if (m_send_handle[0].end && m_send_handle[1].end)
    {
        App_Scheduler_cancelTask(reset_task);
        m_state = CTRL_STATE_IDLE;
    }
}

/**
 * \brief Called when diag packet is sent. Update diag data.
 *
 * \param   status
 *          Status of the sent packet
 */
void diag_sent_cb(const app_lib_data_sent_status_t * status)
{
    LOG(LVL_DEBUG, "Diag sent cb (success:%d)", status->success);

    if (status->success)
    {
        m_diag_info.success++;
        m_diag_info.send_time_us =
                        lib_time->getTimeDiffUs(lib_time->getTimestampHp(),
                                                m_diag_info.send_time_us);
    }
    else
    {
        m_diag_info.error++;
        m_diag_info.send_time_us = 0xFFFFFFFF;
    }
}

/**
 * \brief       Sends DA diagnostics packet at regular interval.
 * \return      time in ms to next execution.
 */
static uint32_t diagnostic_task(void)
{
    app_lib_data_send_res_e res;
    control_diag_t diag;
    app_lib_data_to_send_t tx_diag_def =
    {
        .bytes = (const uint8_t *)&diag,
        .num_bytes = sizeof(control_diag_t),
        .delay = 0,
        .tracking_id = APP_LIB_DATA_NO_TRACKING_ID,
        .qos = APP_LIB_DATA_QOS_NORMAL,
        .flags = APP_LIB_DATA_SEND_FLAG_NONE,
        .src_endpoint = CONTROL_DIAG_SRC_EP,
        .dest_endpoint = CONTROL_DIAG_DEST_EP,
    };

    if (m_state == CTRL_STATE_UNINIT)
    {
        return APP_SCHEDULER_STOP_TASK;
    }



    /* Prepare diagnostics. */
    diag.proc_otap_seq = (uint8_t) lib_otap->getProcessedSeq();
    diag.stored_otap_seq = (uint8_t) lib_otap->getSeq();
    diag.success = m_diag_info.success;
    diag.error = m_diag_info.error;
    diag.timing_us = m_diag_info.send_time_us;

    LOG(LVL_INFO, "Sending DIAG.");
    LOG(LVL_DEBUG, " - proc_otap_seq %d", diag.proc_otap_seq);
    LOG(LVL_DEBUG, " - stored_otap_seq %d", diag.stored_otap_seq);
    LOG(LVL_DEBUG, " - diag_sent %d", diag.success);
    LOG(LVL_DEBUG, " - diag_error %d", diag.error);
    LOG(LVL_DEBUG, " - diag_timing_us %u", diag.timing_us);

    /* This is put there to have easier logs to read. */
    tx_diag_def.dest_address = get_da_router_address(0);

    res = Shared_Data_sendData(&tx_diag_def, diag_sent_cb);
    m_diag_info.send_time_us = lib_time->getTimestampHp();
    if (res != APP_LIB_DATA_SEND_RES_SUCCESS)
    {
        m_diag_info.error++;
        m_diag_info.send_time_us = 0xFFFFFFFF;
        LOG(LVL_ERROR, "Error sending DIAG (res:%d)", res);
    }
    else
    {
        /* diag_sent is incremented in packet sent cb. */
    }

    return m_conf.diag_period_ms;
}

/**
 * \brief       The ACK reception callback.
 *              Calls Ack callback with USER data if any. Manage ACK data and
 *              sets the scratchpad to be processed if needed.
 *
 * \param[in]   item
 *              Pointer to the filter item that initiated the callback.
 * \param[in]   data
 *              Pointer to the received data.
 * \return      Always APP_LIB_DATA_RECEIVE_RES_HANDLED.
 */
static app_lib_data_receive_res_e ack_received (
                                        const shared_data_item_t * item,
                                        const app_lib_data_received_t * data)
{
    if ((data->src_address == m_send_handle[0].address &&
                                                    !m_send_handle[0].end) ||
        (data->src_address == m_send_handle[1].address &&
                                                        !m_send_handle[1].end))
    {
        if (m_send_handle[0].ack == false && m_send_handle[1].ack == false)
        {
            m_send_handle[0].ack = true;
            m_send_handle[1].ack = true;
            LOG(LVL_INFO, "ACK received from %u (len:%d).",
                           data->src_address,
                           data->num_bytes);
        }
        else
        {
            LOG(LVL_DEBUG, "Duplicate ACK received (ignore it).");
            return APP_LIB_DATA_RECEIVE_RES_HANDLED;
        }
    }
    else
    {
        /* This is an ACK from a DIAG packet. */
        LOG(LVL_INFO, "ACK received from DIAG. (len:%d).", data->num_bytes);
    }

    if (m_conf.ack_cb != NULL)
    {
        m_conf.ack_cb((uint8_t *)data->bytes, data->num_bytes);
    }

    return APP_LIB_DATA_RECEIVE_RES_HANDLED;
}

control_node_ret_e Control_Node_init(control_node_conf_t * conf)
{
    app_res_e app_res;
    app_lib_settings_role_t role;
    adv_option_t option = {.follow_network = true};

    LOG(LVL_INFO, "Control Node Init");
    LOG(LVL_DEBUG, "  - diag_period_ms %u",conf->diag_period_ms);
    LOG(LVL_DEBUG, "  - packet_ttl_ms %u",conf->packet_ttl_ms);

    if (conf->diag_period_ms < MIN_DIAG_PERIOD_MS)
    {
        LOG(LVL_WARNING, "diag_period_ms too low (%d). Deactivate diag.",
                         conf->diag_period_ms);
        conf->diag_period_ms = APP_SCHEDULER_STOP_TASK;
    }

    if (lib_settings->getNodeRole(&role) != APP_RES_OK)
    {
        /* The node role must be set*/
        LOG(LVL_ERROR, "Node role not set.");
        return CONTROL_RET_INVALID_ROLE;
    }

    /* Must be Advertiser role. */
    if (role != APP_LIB_SETTINGS_ROLE_ADVERTISER)
    {
        LOG(LVL_ERROR, "Wrong node role (role:%d).", role);
        return CONTROL_RET_INVALID_ROLE;
    }

    /* Copy config to local. */
    memcpy(&m_conf, conf, sizeof(control_node_conf_t));
    memset(&m_diag_info, 0, sizeof(m_diag_info));

    /* Configure Advertiser lib. */
    app_res = lib_advertiser->setOptions(&option);
    if (app_res != APP_RES_OK)
    {
        LOG(LVL_ERROR, "Error setting DA options (res:%d)", app_res);
        return CONTROL_RET_ERROR;
    }
    app_res =lib_advertiser->setQueuingTimeHp(m_conf.packet_ttl_ms);
    if (app_res != APP_RES_OK)
    {
        LOG(LVL_ERROR, "Error setting DA packets TTL (res:%d)", app_res);
        return CONTROL_RET_ERROR;
    }

    /* Set callbacks and tasks. */
    app_res = Shared_Data_addDataReceivedCb(&m_ack_received_item);
    if (app_res != APP_RES_OK)
    {
        LOG(LVL_ERROR, "Error adding Data callback (res:%d)", app_res);
        return CONTROL_RET_ERROR;
    }

    if (m_conf.diag_period_ms != APP_SCHEDULER_STOP_TASK)
    {
        app_scheduler_res_e res;

        res = App_Scheduler_addTask_execTime(diagnostic_task,
                                             m_conf.diag_period_ms,
                                             DIAG_TASK_EXEC_TIME_US);
        if (res != APP_SCHEDULER_RES_OK)
        {
            LOG(LVL_ERROR, "Error adding Task (res:%d)", res);
            return CONTROL_RET_ERROR;
        }
    }

    /* Check m_state and update accordingly */
    if (m_state == CTRL_STATE_UNINIT)
    {
        m_state = CTRL_STATE_IDLE;
    }

    return CONTROL_RET_OK;
}

control_node_ret_e Control_Node_send(app_lib_data_to_send_t * data,
                                     app_lib_data_data_sent_cb_f sent_cb)
{
    app_lib_data_send_res_e res;

    LOG(LVL_INFO, "Send DA packet.");

    if (m_state != CTRL_STATE_IDLE)
    {
        LOG(LVL_ERROR, "Invalid state or library not initialized.");
        return CONTROL_RET_INVALID_STATE;
    }

    if (data->bytes == NULL||
        data->num_bytes == 0 ||
        data->num_bytes > MAX_PAYLOAD)
    {
        LOG(LVL_ERROR, "Invalid parameter.");
        return CONTROL_RET_INVALID_PARAM;
    }

    m_state = CTRL_STATE_BUSY;

    m_time_sent = lib_time->getTimestampCoarse();

    /* Reset internal state*/
    memset(m_send_handle, 0, sizeof(m_send_handle));

    /* Get router address (and backup for retry). */
    LOG(LVL_DEBUG, "Find primary route");
    m_send_handle[0].address = get_da_router_address(0);
    LOG(LVL_DEBUG, "Find backup route");
    m_send_handle[1].address = get_da_router_address(m_send_handle[0].address);

    /* Store data to send and callback. */
    memcpy(&m_data, data, sizeof(app_lib_data_to_send_t));
    memcpy(m_data_buff, data->bytes, data->num_bytes);
    m_data.bytes = m_data_buff;
    m_sent_cb = sent_cb;

    /* Send packet with primary route. */
    m_data.dest_address = m_send_handle[0].address;
    m_send_handle[0].sent = true;
    res = Shared_Data_sendData(&m_data, packet_sent_cb);

    if (res != APP_LIB_DATA_SEND_RES_SUCCESS)
    {
        m_send_handle[0].end = true;

        /* Stop if there is no backup route. */
        if (m_send_handle[1].address == NO_ROUTE_FOUND_ADDRESS)
        {
            m_state = CTRL_STATE_IDLE;
            LOG(LVL_ERROR, "Error sending data (res:%d), and no backup route.",
                           res);
            return CONTROL_RET_SEND_ERROR;
        }

        /* Retry immediately with backup router address. */
        LOG(LVL_WARNING, "Error sending data (res:%d), try backup route.", res);
        m_data.dest_address = m_send_handle[1].address;
        m_send_handle[1].sent = true;
        res = Shared_Data_sendData(&m_data, packet_sent_cb);
        if (res != APP_LIB_DATA_SEND_RES_SUCCESS)
        {
            m_state = CTRL_STATE_IDLE;
            LOG(LVL_WARNING, "Error sending to backup route (res:%d).", res);
            return CONTROL_RET_SEND_ERROR;
        }
    }

    if (App_Scheduler_addTask_execTime(reset_task,
                                       m_conf.packet_ttl_ms*2,
                                       TIMEOUT_TASK_EXEC_TIME_US) !=
                                                        APP_SCHEDULER_RES_OK)
    {
        LOG(LVL_WARNING, "Error adding safety reset task.");
    }

    /* Check if there is a backup route or backup packet already sent. */
    if (m_send_handle[1].address == NO_ROUTE_FOUND_ADDRESS ||
        m_send_handle[1].sent == true)
    {
        /* No need to start the timer. */
        m_send_handle[1].end = true;
        return CONTROL_RET_OK;
    }

    /* Start backup route timer. */
    if (App_Scheduler_addTask_execTime(backup_route_task,
                                       m_conf.packet_ttl_ms/2,
                                       TIMEOUT_TASK_EXEC_TIME_US) !=
                                                        APP_SCHEDULER_RES_OK)
    {
        /* This should not happen. Backup packet will never be sent. */
        m_send_handle[1].end = true;
        LOG(LVL_WARNING, "Error adding timeout task.");
    }

    return CONTROL_RET_OK;
}
