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


#include "api.h"
#include "app_scheduler.h"

/** Define safety margin for processing WAPS */
#define WAPS_SAFETY_MARGIN_US  8000u

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

bool Waps_init(void)
{
    sl_list_init(&waps_request_queue);
    sl_list_init(&waps_ind_queue);
    sl_list_init(&waps_reply_queue);
    if (Waps_prot_init(receive_request))
    {
        Waps_itemInit();
        // Check autostart
        app_lib_state_stack_state_e state = lib_state->getStackState();
        uint32_t autostart;
        lib_storage->readPersistent(&autostart, sizeof(autostart));
        if((state == APP_LIB_STATE_STOPPED) && (autostart & MSAP_AUTOSTART))
        {
            // Start the stack
            lib_state->startStack();
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
            // Frame is not a response
            if(process_request(item))
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

void Waps_sinkUpdated(uint8_t seq, const uint8_t * config, uint16_t interval)
{
    bool is_new = false;

    // Seek if there is existing APP_CONFIG_RX_IND. If so, reuse it
    waps_item_t * item = find_indication(WAPS_FUNC_MSAP_APP_CONFIG_RX_IND);

    // No existing APP_CONFIG_RX_IND found, allocate new
    if (item == NULL)
    {
        item = Waps_itemReserve(WAPS_ITEM_TYPE_INDICATION);
        is_new = true;
    }

    // This might overwrite the old indication
    Msap_handleAppConfig(seq, config, interval, item);

    // Add indication if new
    if (is_new)
    {
        add_indication(item);
    }
}

void Waps_onScannedNbors(void)
{
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

app_lib_data_receive_res_e
Waps_receiveUnicast(const app_lib_data_received_t * data)
{
    waps_item_t * item = Waps_itemReserve(WAPS_ITEM_TYPE_INDICATION);
    if(item)
    {
        // Destination is obviously self
        app_addr_t addr;
        lib_settings->getNodeAddress(&addr);
        w_addr_t dst = Addr_to_Waddr(addr);
        Dsap_packetReceived(data, dst, item);
        add_indication(item);
        return APP_LIB_DATA_RECEIVE_RES_HANDLED;
    }
    return APP_LIB_DATA_RECEIVE_RES_NO_SPACE;
}

app_lib_data_receive_res_e
Waps_receiveBcast(const app_lib_data_received_t * data)
{
    waps_item_t * item = Waps_itemReserve(WAPS_ITEM_TYPE_INDICATION);
    if(item)
    {
        w_addr_t dst;
        // Destination is either broadcast or multicast
        if (data->dest_address == APP_ADDR_BROADCAST)
        {
            dst = WADDR_BCAST;
        }
        else
        {
            // Copy multicast group address to target
            dst = data->dest_address;
        }
        Dsap_packetReceived(data, dst, item);
        add_indication(item);
        return APP_LIB_DATA_RECEIVE_RES_HANDLED;
    }
    return APP_LIB_DATA_RECEIVE_RES_NO_SPACE;
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
