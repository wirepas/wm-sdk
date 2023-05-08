/* Copyright 2017 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

#include <stdint.h>
#include <stdbool.h>

#include "waps.h"
#include "comm/waps_comm.h"
#include "waps_buffer_sizes.h"
#include "waps_item.h"
#include "waps_protocol.h"
#include "uart/waps_uart_protocol.h"
#include "usart.h"
#include "sl_list.h"
#include "api.h"


/** Global access to lower level via function pointers */
waps_prot_t                         waps_prot;

/** Buffers for WAPS protocol */
static uint8_t                      m_waps_tx_buffer[WAPS_TX_BUFFER_SIZE];
static uint8_t                      m_waps_rx_buffer[WAPS_RX_BUFFER_SIZE];

/** Current reply frame */
waps_item_t *                       prot_reply;

/** Last indication information */
waps_item_t *                       prot_indication;
uint8_t                             prot_seq;

/** Callback to upper layer of a received request */
waps_request_receive_f              m_upper_cb;

/**
 * \brief   Frame received callback from lower level
 * \param   data
 *          Data received from lower layer
 * \param   size
 *          Amount of bytes in data
 * \return  true
 *          Packet was valid
 *          false
 *          Packet was invalid
 */
static bool                         frame_receive(void * data, uint32_t size);

bool Waps_prot_init(waps_request_receive_f cb, uint32_t baudrate, bool flow_ctrl)
{
    bool res = false;
    m_upper_cb = cb;

    res = Waps_uart_init(frame_receive,
                         baudrate,
                         flow_ctrl,
                         m_waps_tx_buffer,
                         m_waps_rx_buffer);
    waps_prot.send_reply = Waps_protUart_sendReply;
    waps_prot.write_hw = Waps_uart_send;
    waps_prot.update_irq = Waps_uart_setIrq;
    waps_prot.flush_hw = Waps_uart_flush;
    waps_prot.frame_removed = Waps_protUart_frameRemoved;
    waps_prot.process_response = Waps_protUart_processResponse;

    return res;
}

bool Waps_prot_hasIndication(void)
{
    return (bool)(prot_indication != NULL);
}

void Waps_prot_updateIrqPin(void)
{
    /* Pin control */
    if((sl_list_size(&waps_ind_queue)) ||
       (prot_indication != NULL))
    {
        /* We have stuff to send */
        Waps_prot_updateIrq(true);
    }
    else
    {
        /* Nothing to send */
        Waps_prot_updateIrq(false);
    }
}

static bool frame_receive(void * data, uint32_t size)
{
    /* Check that the reported frame payload size matches the received data */
    waps_frame_t * comm_frame = (waps_frame_t *)data;
    waps_item_t * item;
    if(comm_frame->splen == (size - WAPS_MIN_FRAME_LENGTH))
    {
        item = Waps_itemReserve(WAPS_ITEM_TYPE_REQUEST);
        if(item != NULL)
        {
            item->time = lib_time->getTimestampCoarse();
            item->pre_cb = NULL;
            item->post_cb = NULL;
            memcpy((void *) (&item->frame), data, size);
            // Give item to upper layer
            m_upper_cb(item);
            return true;
        }
    }
    return false;
}

bool prot_send_item(waps_item_t * item)
{
    if (item != NULL)
    {
        waps_frame_t * frame = (waps_frame_t *)&item->frame;
        if (item->pre_cb != NULL)
        {
            item->pre_cb(item);
        }
        if (!Waps_hw_write(frame, WAPS_MIN_FRAME_LENGTH + frame->splen))
        {
            return false;
        }
        if (item->post_cb != NULL)
        {
            item->post_cb(item);
        }
    }
    return true;
}
