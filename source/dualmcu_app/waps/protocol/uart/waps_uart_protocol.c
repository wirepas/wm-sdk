/* Copyright 2017 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

#include <stdint.h>
#include "waps/comm/waps_comm.h"
#include "waps/sap/function_codes.h"
#include "waps_frames.h"
#include "waps_private.h"
#include "waps/protocol/waps_protocol.h"
#include "waps/protocol/waps_protocol_private.h"

/**
 * \brief   Send one indication
 * \param   rx_frame
 *          Information about frame received from UART.
 *          Determines if sending indication is allowed
 */
static void send_indication(waps_frame_t * rx_frame);

bool Waps_protUart_processResponse(waps_item_t * item)
{
    /* Only one possible response for us: indication received */
    if(WapsFunc_isResponse(item->frame.sfunc))
    {
        waps_frame_t * frame = (waps_frame_t *)&item->frame;
        if (prot_indication != NULL)
        {
            if((frame->sfid == prot_seq) &&
               (frame->sfunc == (prot_indication->frame.sfunc | 0x80)))
            {
                /* Response to last indication */
                Waps_itemFree(prot_indication);
                prot_indication = NULL;
                prot_seq++;
            }
            /* (Re)send the indication */
            send_indication(frame);
        }
        return true;
    }
    return false;
}

void Waps_protUart_sendReply(void)
{
    if(prot_reply == NULL)
    {
        prot_reply = (waps_item_t *)sl_list_pop_front(&waps_reply_queue);
    }
    if(prot_reply != NULL)
    {
        if (prot_send_item(prot_reply))
        {
            send_indication(&prot_reply->frame);
            Waps_itemFree(prot_reply);
            prot_reply = NULL;
        }
    }
}

void Waps_protUart_frameRemoved(void)
{
    // Do nothing (atm)
}

static void send_indication(waps_frame_t * rx_frame)
{
    /* Two cases when sending indications is allowed:
     * 1) Client asks for IND, we respond with CNF & first IND
     * 2) Client responds to IND, with PLD[0] set */

    if (rx_frame->sfunc != WAPS_FUNC_MSAP_INDICATION_POLL_CNF)
    {
        /* Func is not poll CNF, see if frame is response with OK bit set */
        if(!(WapsFunc_isResponse(rx_frame->sfunc) &&
            (rx_frame->spld[0] == 1)))
        {
            /* Either not response, or is response but OK bit is not set */
            goto dont_send_ind;
        }
    }
    if(prot_indication == NULL)
    {
        /* New frame start (get oldest frame) */
        prot_indication = (waps_item_t *)sl_list_pop_front(&waps_ind_queue);
    }
    if(prot_indication != NULL)
    {
        prot_indication->frame.sfid = prot_seq;
        prot_indication->frame.spld[0] = queued_indications();
        if (!prot_send_item(prot_indication))
        {
            /* Failed, put back to front of queue (minimize delays) */
            sl_list_push_front(&waps_ind_queue, (sl_list_t *)prot_indication);
            prot_indication = NULL;
        }
    }

dont_send_ind:
    Waps_prot_updateIrqPin();
}
