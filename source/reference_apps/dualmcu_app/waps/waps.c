/* Copyright 2017 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */


#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "protocol/waps_protocol.h"
#include "waps_frames.h"
#include "sap/dsap.h"
#include "sap/csap.h"
#include "sap/msap.h"
#include "sap/function_codes.h"
#include "waps.h"
#include "waps_private.h"
#include "sap/persistent.h"


#include "api.h"
#include "app_scheduler.h"
#include "shared_data.h"
#include "shared_appconfig.h"
#include "shared_neighbors.h"
#include "stack_state.h"

/** Define safety margin for processing WAPS */
#define WAPS_SAFETY_MARGIN_US  8000u

/** Max timeout to handle a request queued from uart
 *  API says 300ms
 */
#define WAPS_MAX_REQUEST_QUEUING_TIME_MS    300

/** Convert MS to coarse
 *  Because of 1/128s granularity and way it is measured, delay can be shorter but it is not an issue.
 *  It is better to delete a request that is 290ms old instead of playing one that is
 *  310ms old. On the other side, the second one will already be replayed.
 */
#define WAPS_MAX_REQUEST_QUEUING_TIME_COARSE ((WAPS_MAX_REQUEST_QUEUING_TIME_MS * 128 / 1000))


/**
 * \brief   Waps_exec is the core of WAPS
 *          Waps_exec must be allocated as much run time as possible to
 *          make it reliable. It processes primitives from uart_waps and
 *          responses to these primitives. It also processes messages from
 *          WSN and produces primitives from these messages.
 * \post    WAPS processes primitive.
 * \return  Next requested invocation time, or OS_NO_TIMETABLE
 */
static uint32_t Waps_exec(void);

/** Something to send ? */
static bool frames_pending(void);

/** Pushes one indication to indication queue */
static void add_indication(waps_item_t * msg);

/** Add reply */
static void add_reply(waps_item_t * resp);

/** Read single request from request queue */
static waps_item_t * read_request(void);

/** New request callback from lower layer */
static void receive_request(waps_item_t * item);

static void app_config_received_cb(uint16_t type,
                                   uint8_t length,
                                   uint8_t * value_p);

/** Callback when a neighbor scan is done */
static void on_scanned_nbors_cb(void);

/**
 * \brief   Process request and generate reply
 * \return  True, if a reply was generated
 */
static bool process_request(waps_item_t * item);

/**
 * \brief   Find similar item (for re-using memory)
 * \param   id
 *          Id to search for in indications
 */
static waps_item_t * find_indication(uint8_t id);

/** WAPS internal message queues */
sl_list_head_t              waps_ind_queue;
sl_list_head_t              waps_reply_queue;
sl_list_head_t              waps_request_queue;

// Signal from protocol (keep as u32 to prevent compiler from packing)
static uint32_t             m_signal;

// Number of channels, cached for get_num_channels()
static app_lib_settings_net_channel_t num_channels;

static app_lib_data_receive_res_e data_cb(
                    const shared_data_item_t * shared_data_item,
                    const app_lib_data_received_t * data)
{
    w_addr_t dst;
    waps_item_t * item = Waps_itemReserve(WAPS_ITEM_TYPE_INDICATION);
    if(item)
    {
        if (data->dest_address == APP_ADDR_BROADCAST)
        {
            dst = WADDR_BCAST;
        }
        else if ((data->dest_address & 0xff000000) == APP_ADDR_MULTICAST)
        {
            dst = data->dest_address;
        }
        else
        {
            // Destination is obviously self
            app_addr_t addr;
            lib_settings->getNodeAddress(&addr);
            dst = Addr_to_Waddr(addr);
        }
        Dsap_packetReceived(data, dst, item);
        add_indication(item);
        return APP_LIB_DATA_RECEIVE_RES_HANDLED;
    }
    return APP_LIB_DATA_RECEIVE_RES_NO_SPACE;
}

static shared_data_item_t m_waps_data_filter =
{
    .cb = data_cb,
    .filter = {
                .mode = SHARED_DATA_NET_MODE_ALL,
                .src_endpoint = -1,
                .dest_endpoint = -1,
                .multicast_cb = Multicast_isGroupCb,
              }
};

static void item_free_threshold_cb(void)
{
    Shared_Data_readyToReceive(&m_waps_data_filter);
}

bool Waps_init(void)
{
    uint16_t id;
    shared_app_config_filter_t app_config_filter = {
        // Any type is fine, just used to know app config is received
        .type = 1,
        .cb = app_config_received_cb,
        .call_cb_always = true,
    };

    // id is not persistent as we never release it (for now)
    Shared_Neighbors_addScanNborsCb(on_scanned_nbors_cb, &id);
    //register callbacks
    Shared_Data_addDataReceivedCb(&m_waps_data_filter);
    Shared_Appconfig_addFilter(&app_config_filter, &id);

    sl_list_init(&waps_request_queue);
    sl_list_init(&waps_ind_queue);
    sl_list_init(&waps_reply_queue);
    // Cache number of channels. For the purposes of
    // lib_settings->setReservedChannels(), minimum channel is always 1
    uint16_t tmp1, tmp2;
    lib_settings->getNetworkChannelLimits(&tmp1, &tmp2);
    num_channels = (app_lib_settings_net_channel_t)tmp2;
    // Initialize submodules
    Persistent_init();
    if (Waps_prot_init(receive_request))
    {
        // Initialize item pool with a threshold to 50%
        Waps_itemInit(item_free_threshold_cb, 50);
        // Check autostart
        bool autostart;
        if((!Stack_State_isStarted()) &&
           Persistent_getAutostart(&autostart) == APP_RES_OK &&
           autostart)
        {
            Stack_State_startStack();
        }
        // Queue indication to show that stack has started (or waiting to start)
        add_indication(Msap_getStackStatusIndication());
        // Clear the signal here (can be set after Waps_prot_init())
        m_signal = 0;
        return true;
    }

    return false;
}

uint32_t Waps_exec(void)
{
    // Task is scheduled, clear signal
    m_signal = 0;

    // Handle only a single message here, and return to scheduler immediately.
    waps_item_t * item = read_request();
    if(item != NULL)
    {
        // Handle message from user
        if(!Waps_prot_processResponse(item))
        {
            // Frame is not a response, check if request is not too old

            // Request queued for too long time must not be executed
            // In reality, it will only happen when a scratchpad is exchanged and app
            // is not scheduled anymore for very long period > 10s
            if (lib_time->getTimestampCoarse() - item->time > WAPS_MAX_REQUEST_QUEUING_TIME_COARSE)
            {
                // Nothing to do except freeing memory, done just later
            }
            else if(process_request(item))
            {
                // Valid request needs reply (memory item re-used)
                add_reply(item);
                goto send_reply;
            }
        }
        // Frame is an invalid request or valid response -> free memory
        Waps_prot_frameRemoved();
        Waps_itemFree((void *)item);
        item = NULL;
    }
send_reply:
    // As sending reply might fail, must re-enter WAPS to attempt again
    Waps_prot_sendReply();

    // Re-schedule next
    if(frames_pending())
    {
        // Not all is done wake us up, again
        return APP_SCHEDULER_SCHEDULE_ASAP;
    }
    else
    {
        return APP_SCHEDULER_STOP_TASK;
    }
}

static void app_config_received_cb(uint16_t type,
                                   uint8_t length,
                                   uint8_t * value_p)
{
    uint8_t appconfig[APP_LIB_DATA_MAX_APP_CONFIG_NUM_BYTES];
    uint8_t appconfig_seq;
    uint16_t appconfig_interval;
    bool is_new = false;

    /* Our filter was called but we don't need particular info so
       read it from stack */
    lib_data->readAppConfig(&appconfig[0],
                            &appconfig_seq,
                            &appconfig_interval);

    // Seek if there is existing APP_CONFIG_RX_IND. If so, reuse it
    waps_item_t * item = find_indication(WAPS_FUNC_MSAP_APP_CONFIG_RX_IND);

    // No existing APP_CONFIG_RX_IND found, allocate new
    if (item == NULL)
    {
        item = Waps_itemReserve(WAPS_ITEM_TYPE_INDICATION);
        is_new = true;
    }

    // This might overwrite the old indication
    Msap_handleAppConfig(appconfig_seq,
                         appconfig,
                         appconfig_interval,
                         item);

    // Add indication if new
    if (is_new)
    {
        add_indication(item);
    }
}

static void on_scanned_nbors_cb(void)
{
// The next block is under ifdef has dualmcu has never generated
// Neighbors indication. It was only registering for requested scans
// result that are requested by app, but dualmcu cannot ask for scans.
// Since Sharedd_neighbors libs is in use, scan result from stack will
// trigger this callback and Wirepas Terminal doesn't handle it (stay stuck).
// So for now, it has to be explicitly enabled.
#ifdef GENERATE_NEIGHBORS_INDICATION
    // Find similar indication and re-use ite
    waps_item_t * item = find_indication(WAPS_FUNC_MSAP_SCAN_NBORS_IND);
    bool is_new = false;
    if(item == NULL)
    {
        // No existing indication can be re-used, allocate new
        item = Waps_itemReserve(WAPS_ITEM_TYPE_INDICATION);
        is_new = true;
    }
    // Build indication
    Msap_onScannedNbors(item);
    // Then add the indication (if new)
    if(is_new)
    {
        add_indication(item);
    }
#endif
}

void Waps_packetSent(app_lib_data_tracking_id_t tracking_id,
                     uint8_t src_ep,
                     uint8_t dst_ep,
                     uint32_t queue_time,
                     app_addr_t dst_addr,
                     bool success)
{
    waps_item_t * item = Waps_itemReserve(WAPS_ITEM_TYPE_INDICATION);
    if(item)
    {
        w_addr_t dst = Addr_to_Waddr(dst_addr);
        pduid_t id = (pduid_t)tracking_id;
        Dsap_packetSent(id, src_ep, dst_ep, queue_time, dst, success, item);
        add_indication(item);
    }
}

uint8_t queued_indications(void)
{
    return (sl_list_size(&waps_ind_queue) ? 1 : 0);
}

void wakeup_task(void)
{
    // Simple lock
    uint32_t signaled;
    lib_system->enterCriticalSection();
    signaled = m_signal;
    m_signal = 1;
    lib_system->exitCriticalSection();
    // Ask for wake up at once
    if(!signaled)
    {
        // Do this only once
        App_Scheduler_addTask_execTime(Waps_exec, 0, WAPS_SAFETY_MARGIN_US);
    }
}

app_lib_settings_net_channel_t get_num_channels(void)
{
    // Return number of channels, which was cached in Waps_init()
    return num_channels;
}

static bool frames_pending(void)
{
    return (bool)(sl_list_size(&waps_request_queue) ||
                  sl_list_size(&waps_reply_queue));
}

static void add_indication(waps_item_t * msg)
{
    if(msg != NULL)
    {
        /* Put to back of queue */
        sl_list_push_back(&waps_ind_queue, (sl_list_t *)msg);
        Waps_prot_updateIrqPin();
    }
}

static void add_reply(waps_item_t * resp)
{
    if(resp != NULL)
    {
        sl_list_push_back(&waps_reply_queue, (sl_list_t *)resp);
    }
}

static waps_item_t * read_request(void)
{
    waps_item_t * item;
    lib_system->enterCriticalSection();
    item = (waps_item_t *)sl_list_pop_front(&waps_request_queue);
    lib_system->exitCriticalSection();
    return item;
}

static void receive_request(waps_item_t * item)
{
    lib_system->enterCriticalSection();
    sl_list_push_back(&waps_request_queue, (sl_list_t *)item);
    lib_system->exitCriticalSection();
    wakeup_task();
}

static bool process_request(waps_item_t * item)
{
    if (WapsFunc_isDsapRequest(item->frame.sfunc))
    {
        return Dsap_handleFrame(item);
    }
    else if (WapsFunc_isMsapRequest(item->frame.sfunc))
    {
        return Msap_handleFrame(item);
    }
    else if (WapsFunc_isCsapRequest(item->frame.sfunc))
    {
        return Csap_handleFrame(item);
    }
    return false;
}

static waps_item_t * find_indication(uint8_t id)
{
    // Seek if there is existing indication with id. If so, reuse it
    waps_item_t * item = (waps_item_t *) sl_list_begin(&waps_ind_queue);
    while (item != NULL)
    {
        if (item->frame.sfunc == id)
        {
            break;
        }
        else
        {
            item = (waps_item_t *) sl_list_next((sl_list_t *) item);
        }
    }
    return item;
}
