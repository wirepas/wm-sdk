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
#include "protocol/waps_protocol_private.h" // To access prot_indication
#include "sap/persistent.h"


#include "api.h"
#include "app_scheduler.h"
#include "shared_data.h"
#include "shared_appconfig.h"
#include "stack_state.h"
#include "ds.h"

/** Define safety margin for processing WAPS */
#define WAPS_SAFETY_MARGIN_US  8000u

/** Max timeout to handle a request queued from uart
 *  API says 300ms
 */
#define WAPS_MAX_REQUEST_QUEUING_TIME_MS    450

/** Convert MS to coarse
 *  Because of 1/128s granularity and way it is measured, delay can be shorter but it is not an issue.
 *  It is better to delete a request that is 290ms old instead of playing one that is
 *  310ms old. On the other side, the second one will already be replayed.
 */
#define WAPS_MAX_REQUEST_QUEUING_TIME_COARSE ((WAPS_MAX_REQUEST_QUEUING_TIME_MS * 128 / 1000))

/** Initialization time deep sleep preventation
  * In case stack is not running, some devices (like Thunderboard BG22) requires extra run time before
  * deep sleep can be enabled, othervise re-flashing of the device would not work without powercycle.
  * This is because debugger takes some time to connect, and it does not have a chance after
  * the boot time optimizations were implemented. Each time system goes to deep sleep by autopower,
  * the debugger needs to attempt the connection again as HFXO was shut down for deep sleep.
  * After deep sleep, establishing debugger connection fails again as the device goes again to
  * deep sleep by the request of autopower. To get the debugger connection established
  * little more initial run time is needed. After debugger connection is established once,
  * the HFXO will stay on also in deep sleep by request of debugging interface.
  */
#define WAPS_INIT_DEEP_SLEEP_PREVENTATION_TIME_MS      100

/** Advise scheduler how long will it take to get
  * deep sleep preventation bit removed.
  */
#define WAPS_TASK_EXEC_TIME_FOR_REMOVE_DS_PREV_US      500

/** Maximum time in ms to perform a gargbage collect on indication queued.
 *  Every period, the queued indication will be compared to the max
 *  TTL set in the node to removed if too old
 */
#define WAPS_GARBAGE_COLLECT_PERIOD_MS                 5000

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

/**
 * \bried   Waps_init_completed_for_deep_sleep
 *          Initialization time has completed and deep sleep could be enabled.
 */
static uint32_t Waps_init_completed_for_deep_sleep(void);

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
static void on_scanned_nbors_cb(app_lib_stack_event_e event, void * param);

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

/**
 * \brief   Delete indication that exceeded their max TTL
 */
static uint32_t garbage_collect_old_indication_task(void);

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

bool Waps_init(uint32_t baudrate, bool flow_ctrl)
{
    uint16_t id;
    shared_app_config_filter_t app_config_filter = {
        // Any type is fine, just used to know app config is received
        .type = 1,
        .cb = app_config_received_cb,
        .call_cb_always = true,
    };

    // We are only interested by SCAN_STOPPED event
    Stack_State_addEventCb(on_scanned_nbors_cb, 1 << APP_LIB_STATE_STACK_EVENT_SCAN_STOPPED);
    //register callbacks
    Shared_Data_addDataReceivedCb(&m_waps_data_filter);
    Shared_Appconfig_addFilter(&app_config_filter, &id);

    // Enforce fragmented mode as dualmcu protocol cannot forward 1500 bytes packet
    // over uart (all size are on 1 byte) and uart drivers are not ready
    // TODO: this mode should be set by Shared_Data lib instead depending on a
    // build flag
    lib_data->setFragmentMode(APP_LIB_DATA_FRAGMENTED_MODE_ENABLED);

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
    if (Waps_prot_init(receive_request, baudrate, flow_ctrl))
    {
        // Initialize item pool with a threshold to 50%
        Waps_itemInit(item_free_threshold_cb, 50);
        // Check autostart
        bool autostart;
        if(!Stack_State_isStarted())
        {
            DS_Disable(DS_SOURCE_INIT);
            if (Persistent_getAutostart(&autostart) == APP_RES_OK &&
                autostart)
            {
                if (Stack_State_startStack() == APP_RES_OK)
                {
                    DS_Enable(DS_SOURCE_INIT);
                }
            }
            else
            {
                App_Scheduler_addTask_execTime(
                    Waps_init_completed_for_deep_sleep,
                    WAPS_INIT_DEEP_SLEEP_PREVENTATION_TIME_MS,
                    WAPS_TASK_EXEC_TIME_FOR_REMOVE_DS_PREV_US);
            }
        }
        // Queue indication to show that stack has started (or waiting to start)
        add_indication(Msap_getStackStatusIndication());
        // Clear the signal here (can be set after Waps_prot_init())
        m_signal = 0;

        // Start a task to remove old indication (according to TTL)
        App_Scheduler_addTask_execTime(garbage_collect_old_indication_task, WAPS_GARBAGE_COLLECT_PERIOD_MS, 100);
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
            if (lib_time->getTimestampCoarse() - item->time >
                WAPS_MAX_REQUEST_QUEUING_TIME_COARSE)
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

static uint32_t Waps_init_completed_for_deep_sleep(void)
{
    DS_Enable(DS_SOURCE_INIT);

    return APP_SCHEDULER_STOP_TASK;
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

static void on_scanned_nbors_cb(app_lib_stack_event_e event, void * param)
{
    app_lib_state_neighbor_scan_info_t * scan_info = (app_lib_state_neighbor_scan_info_t *) param;
    if (!scan_info->complete ||
        scan_info->scan_type != SCAN_TYPE_APP_ORIGINATED)
    {
        // Discard scan result not initiated by app or
        // those that are not full
        // All scan could generate an indication but Wirepas Terminal is not ready for that
        return;
    }

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

static bool is_indication_too_old(waps_item_t * item, uint16_t qos_qt_s[2])
{
    uint8_t qos;
    uint32_t qt, travel_time;
    uint32_t now = lib_time->getTimestampCoarse();

    // Data RX Ind and DATA RX Frag Ind are handled independently
    // even if struct are alligned, but more futur proof
    if (item->frame.sfunc == WAPS_FUNC_DSAP_DATA_RX_IND)
    {
        qos = item->frame.dsap.data_rx_ind.info & RX_IND_INFO_QOS_MASK;
        travel_time = item->frame.dsap.data_rx_ind.delay;
    }
    else if (item->frame.sfunc == WAPS_FUNC_DSAP_DATA_RX_FRAG_IND)
    {
        qos = item->frame.dsap.data_rx_frag_ind.info & RX_IND_INFO_QOS_MASK;
        travel_time = item->frame.dsap.data_rx_frag_ind.delay;
    }
    else
    {
        return false;
    }

    // Queued time on dualmcu side
    qt = now - item->time;

    // Check travel_time + queued_time vs limit set
    if (((travel_time + qt) / 128) > qos_qt_s[qos])
    {
        // Too long in transit, delete it
        return true;
    }

    return false;
}

static uint32_t garbage_collect_old_indication_task(void)
{
    waps_item_t * item;

    // Add a default value, in case it is not possible to read from stack (Not really possible)
    uint16_t qos_qt_s[2] = {10 * 60, 5 * 60};

    if (!queued_indications() && (prot_indication == NULL))
    {
        // Nothing to be done
        // Return time could be optimize to min qos time.
        // If no indication at the moment, next potential clean up will be in min quing time set
        // but let's keep things simpler
        return WAPS_GARBAGE_COLLECT_PERIOD_MS;
    }

    // Check the next indication that is already out of the queue
    if (prot_indication != NULL)
    {
        if (is_indication_too_old(prot_indication, qos_qt_s))
        {
            Waps_itemFree(prot_indication);
            prot_indication = NULL;
        }
    }

    // Check all items from the queue one by one
    item = (waps_item_t *) sl_list_begin(&waps_ind_queue);
    while (item != NULL)
    {
        // Get next immediatelly in case we remove the item
        waps_item_t * next = (waps_item_t *) sl_list_next((sl_list_t *) item);
        if (is_indication_too_old(item, qos_qt_s))
        {
            // Unfortunately, there is no api in sl_list to remove without
            // going through the list again, we could play with pointer here, but not
            // good pattern to access internal sl_list struct.
            // Anyway, list should never be very long
            sl_list_remove(&waps_ind_queue, (sl_list_t *) item);
            // Put back the item to free list and potentially re-enable the RX from stack
            Waps_itemFree(item);
        }

        // move to next one
        item = next;
    }

    return WAPS_GARBAGE_COLLECT_PERIOD_MS;
}
