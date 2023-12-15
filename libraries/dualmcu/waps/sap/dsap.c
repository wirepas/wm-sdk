/* Copyright 2017 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

#include "dsap.h"
#include "dsap_frames.h"
#include "function_codes.h"
#include "waps_private.h"
#include "waddr.h"
#include "lock_bits.h"
#include "api.h"
#include "shared_data.h"
#include "waps.h"

/**
 * \brief   Update delay (callback from protocol interface)
 * \param   item
 *          Item that gets updated
 */
static void update_packet_delay(waps_item_t * item);

static uint8_t get_singlemcu_flag_from_dualmcu_tx_option(uint8_t tx_opts)
{
    uint8_t flags = 0;
     // Check TX options
    if(tx_opts & TX_OPTS_IND_REQ)
    {
        flags = APP_LIB_DATA_SEND_FLAG_TRACK;
    }
    else
    {
        flags = APP_LIB_DATA_SEND_FLAG_NONE;
    }

    if (tx_opts & TX_OPTS_UNACK_CSMA_CA)
    {
        flags |= APP_LIB_DATA_SEND_FLAG_UNACK_CSMA_CA;
    }

    if (tx_opts & TX_OPTS_HOPLIMIT_MASK)
    {
        flags |= APP_LIB_DATA_SEND_SET_HOP_LIMITING;
    }

    return flags;
}

static uint8_t get_single_mcu_hop_limit_from_dualmcu_tx_option(uint8_t tx_opts)
{
    return (tx_opts & TX_OPTS_HOPLIMIT_MASK) >>
            TX_OPTS_HOPLIMIT_OFFSET;
}

static void dataSentCb(const app_lib_data_sent_status_t * status)
{
    Waps_packetSent(status->tracking_id,
                    status->src_endpoint,
                    status->dest_endpoint,
                    status->queue_time,
                    status->dest_address,
                    status->success);
}

bool Dsap_handleFrame(waps_item_t * item)
{
    app_lib_data_send_res_e res;
    size_t capacity = 0;
    pduid_t apdu_id;
    app_lib_data_to_send_t data;

    if (item->frame.sfunc == WAPS_FUNC_DSAP_DATA_TX_REQ)
    {
        dsap_data_tx_req_t * req = &item->frame.dsap.data_tx_req;
        // Check the size
        if (item->frame.splen != (FRAME_DSAP_DATA_TX_REQ_HEADER_SIZE +
            req->apdu_len))
        {
            return false;
        }
        apdu_id = req->apdu_id;
        // Convert data_tx_req to app_lib_data_to_send_t
        uint8_t flags = get_singlemcu_flag_from_dualmcu_tx_option(req->tx_opts);
        data = (app_lib_data_to_send_t)
        {
            .src_endpoint = req->src_endpoint,
            .dest_address = Waddr_to_Addr(req->dst_addr),
            .dest_endpoint = req->dst_endpoint,
            .qos = req->qos, // Qos value are align
            .flags = flags,
            .num_bytes = req->apdu_len,
            .bytes = req->apdu,
            .tracking_id = req->apdu_id,
            .hop_limit = get_single_mcu_hop_limit_from_dualmcu_tx_option(req->tx_opts),
        };
    }
    else if (item->frame.sfunc == WAPS_FUNC_DSAP_DATA_TX_TT_REQ)
    {
        dsap_data_tx_tt_req_t * req = &item->frame.dsap.data_tx_tt_req;
        // Check the size
        if (item->frame.splen != (FRAME_DSAP_DATA_TX_TT_REQ_HEADER_SIZE +
            req->apdu_len))
        {
            return false;
        }
        apdu_id = req->apdu_id;
        // Convert data_tx_tt_req to app_lib_data_to_send_t
        uint8_t flags = get_singlemcu_flag_from_dualmcu_tx_option(req->tx_opts);
        data = (app_lib_data_to_send_t)
        {
            .src_endpoint = req->src_endpoint,
            .dest_address = Waddr_to_Addr(req->dst_addr),
            .dest_endpoint = req->dst_endpoint,
            .qos = req->qos, // Qos value are align
            .flags = flags,
            .num_bytes = req->apdu_len,
            .bytes = req->apdu,
            .tracking_id = req->apdu_id,
            .hop_limit = get_single_mcu_hop_limit_from_dualmcu_tx_option(req->tx_opts),
        };
    }
    else if (item->frame.sfunc == WAPS_FUNC_DSAP_DATA_TX_FRAG_REQ)
    {
        dsap_data_tx_frag_req_t * req = &item->frame.dsap.data_tx_frag_req;
        if (item->frame.splen != (FRAME_DSAP_DATA_TX_FRAG_REQ_HEADER_SIZE +
            req->apdu_len))
        {
            return false;
        }
        apdu_id = req->apdu_id;
        // Convert data_tx_frag_req to app_lib_data_to_send_t
        uint8_t flags = get_singlemcu_flag_from_dualmcu_tx_option(req->tx_opts);
        // Add the fragmented flag
        flags |= APP_LIB_DATA_SEND_FRAGMENTED_PACKET;

        bool last_fragment = (req->fragment_offset_flag & DSAP_FRAG_LAST_FLAG_MASK) == DSAP_FRAG_LAST_FLAG_MASK;
        size_t fragment_offset = req->fragment_offset_flag & DSAP_FRAG_LENGTH_MASK;

        data = (app_lib_data_to_send_t)
        {
            .src_endpoint = req->src_endpoint,
            .dest_address = Waddr_to_Addr(req->dst_addr),
            .dest_endpoint = req->dst_endpoint,
            .qos = req->qos, // Qos value are align
            .flags = flags,
            .num_bytes = req->apdu_len,
            .bytes = req->apdu,
            .tracking_id = req->apdu_id,
            .hop_limit = get_single_mcu_hop_limit_from_dualmcu_tx_option(req->tx_opts),
            .fragment_info = {
                .fragment_offset = fragment_offset,
                .last_fragment = last_fragment,
                .packet_id = req->full_packet_id,
            }
        };
    }
    else
    {
        return false;
    }

    // Check that TX feature is permitted
    if (!LockBits_isFeaturePermitted(LOCK_BITS_DSAP_DATA_TX))
    {
        res = APP_LIB_DATA_SEND_RES_ACCESS_DENIED;
    }
    else if (data.num_bytes > APDU_MAX_SIZE)
    {
        // Enforce the max size of a packet. this value is hardcoded
        // in many places. Node should be able to send bigger one but
        // it will be segmented implicitely. To send bigger packets on
        // dualmcu api, explicit service to send fragment must be used
        res = APP_LIB_DATA_SEND_RES_INVALID_NUM_BYTES;
    }
    else
    {
        res = Shared_Data_sendData(&data,
                                   data.flags & APP_LIB_DATA_SEND_FLAG_TRACK ? dataSentCb : NULL);
    }

    // Get remaining buffer capacity (do not pre-check and decrement = lie)
    lib_data->getNumFreeBuffers(&capacity);

    // Send the response
    Waps_item_init(item, item->frame.sfunc + 0x80, sizeof(dsap_data_tx_cnf_t));
    item->frame.dsap.data_tx_cnf.apdu_id = apdu_id;
    item->frame.dsap.data_tx_cnf.buff_cap = (uint8_t) capacity;
    item->frame.dsap.data_tx_cnf.result = (uint8_t) res;

    return true;
}

void Dsap_packetSent(pduid_t id,
                     uint8_t src_ep,
                     uint8_t dst_ep,
                     uint32_t queue_time,
                     w_addr_t dst,
                     bool success,
                     waps_item_t * output_ptr)
{
    // Build response
    Waps_item_init(output_ptr,
                   WAPS_FUNC_DSAP_DATA_TX_IND,
                   sizeof(dsap_data_tx_ind_t));
    output_ptr->time = 0;
    dsap_data_tx_ind_t * ind_ptr = &output_ptr->frame.dsap.data_tx_ind;
    ind_ptr->apdu_id = (pduid_t)id;
    ind_ptr->dst_addr = (w_addr_t)dst;
    ind_ptr->dst_endpoint = dst_ep;
    ind_ptr->queue_delay = queue_time;
    ind_ptr->queued_indications = 0;
    ind_ptr->src_endpoint = src_ep;
    ind_ptr->result = success ? DSAP_IND_SUCCESS : DSAP_IND_TIMEOUT;
}

void Dsap_packetReceived(const app_lib_data_received_t * data,
                         w_addr_t dst_addr,
                         waps_item_t * output_ptr)
{
    uint8_t hops = data->hops;
    uint8_t info = (data->qos << RX_IND_INFO_QOS_OFFSET) & RX_IND_INFO_QOS_MASK;

    // Cap maximum hops to 63 which can be represented in interface
    if (hops > RX_IND_INFO_MAX_HOPCOUNT)
    {
        hops = RX_IND_INFO_MAX_HOPCOUNT;
    }

    // Add it to info field
    info |= (hops << RX_IND_INFO_HOPCOUNT_OFFSET) & RX_IND_INFO_HOPCOUNT_MASK;

    // Create indication depending if it is a fragmented packet or not
    if (data->fragment_info != NULL)
    {
        Waps_item_init(output_ptr,
                    WAPS_FUNC_DSAP_DATA_RX_FRAG_IND,
                    FRAME_DSAP_DATA_RX_FRAG_IND_HEADER_SIZE + data->num_bytes);

        dsap_data_rx_frag_ind_t * ind_ptr = &output_ptr->frame.dsap.data_rx_frag_ind;

        ind_ptr->src_endpoint = data->src_endpoint;
        ind_ptr->src_addr = data->src_address;
        ind_ptr->dst_addr = dst_addr;
        ind_ptr->dst_endpoint = data->dest_endpoint;
        ind_ptr->info = info;
        ind_ptr->delay = data->delay;
        ind_ptr->queued_indications = 0;
        ind_ptr->apdu_len = data->num_bytes;
        ind_ptr->fragment_offset_flag = data->fragment_info->fragment_offset & DSAP_FRAG_LENGTH_MASK;
        if (data->fragment_info->last_fragment)
        {
            ind_ptr->fragment_offset_flag |= DSAP_FRAG_LAST_FLAG_MASK;
        }
        ind_ptr->full_packet_id = data->fragment_info->packet_id;

        memcpy(ind_ptr->apdu, data->bytes, data->num_bytes);
    }
    else
    {
        Waps_item_init(output_ptr,
            WAPS_FUNC_DSAP_DATA_RX_IND,
            FRAME_DSAP_DATA_RX_IND_HEADER_SIZE + data->num_bytes);

        dsap_data_rx_ind_t * ind_ptr = &output_ptr->frame.dsap.data_rx_ind;

        ind_ptr->src_endpoint = data->src_endpoint;
        ind_ptr->src_addr = data->src_address;
        ind_ptr->dst_addr = dst_addr;
        ind_ptr->dst_endpoint = data->dest_endpoint;
        ind_ptr->info = info;
        ind_ptr->delay = data->delay;
        ind_ptr->queued_indications = 0;
        ind_ptr->apdu_len = data->num_bytes;

        memcpy(ind_ptr->apdu, data->bytes, data->num_bytes);
    }

    // Initialize delay and protocol delay incrementing function callback
    output_ptr->time = lib_time->getTimestampCoarse();
    output_ptr->pre_cb = update_packet_delay;

}


static void update_packet_delay(waps_item_t * item)
{
    if (item != NULL)
    {
        uint32_t now = lib_time->getTimestampCoarse();
        uint32_t local_delay = now - item->time;
        item->time = now;
        if (item->frame.sfunc == WAPS_FUNC_DSAP_DATA_RX_IND)
        {
            item->frame.dsap.data_rx_ind.delay += local_delay;
        }
        else if (item->frame.sfunc == WAPS_FUNC_DSAP_DATA_RX_FRAG_IND)
        {
            item->frame.dsap.data_rx_frag_ind.delay += local_delay;
        }
    }
}
