/* Copyright 2019 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */
#include "shared_libdata.h"
#include <string.h>
#define DEBUG_LOG_MODULE_NAME "SHARED D"
#define DEBUG_LOG_MAX_LEVEL LVL_NOLOG
#include "debug_log.h"

/** Some helpers macros for packet filtering. */
#define IS_UNICAST(mode) (mode == SHARED_LIBDATA_NET_MODE_UNICAST)
#define IS_BROADCAST(mode) (mode == SHARED_LIBDATA_NET_MODE_BROADCAST)
#define IS_MULTICAST(mode) (mode == SHARED_LIBDATA_NET_MODE_MULTICAST)
#define IS_ANYCAST(mode) (mode == SHARED_LIBDATA_NET_MODE_ALL)

/** Minimum endpoint value. */
#define ENDPOINT_MIN SHARED_LIBDATA_UNUSED_ENDPOINT

/** Maximum endpoint value. */
#define ENDPOINT_MAX 255

/** Maximum number of buffers for tracked packets. */
#define MAX_TRACKED_PACKET 16

/** Head of data callbacks / filters linked list. */
static sl_list_head_t m_shared_data_head;

/** True when one function is iterating through the whole list. */
static bool m_iterating_list;

/** Callbacks for packets being tracked. */
static app_lib_data_data_sent_cb_f m_tracked_packets[MAX_TRACKED_PACKET];

/**
 * \brief   Filter the received packet.
 * \param   filter
 *          Filter to apply to the received packet.
 * \param   data
 *          The received data packet.
 * \return  True if packet is to be received, false if packet is not for
 *          this callback.
 */
static bool filter_received_packet(const shared_libdata_filter_t * filter,
                                   const app_lib_data_received_t * data)
{
    if (IS_UNICAST(filter->mode) &&
                (data->dest_address == APP_ADDR_BROADCAST ||
                 ((data->dest_address & 0xff000000) == APP_ADDR_MULTICAST)))
    {
        return false;
    }

    if (IS_BROADCAST(filter->mode) && data->dest_address != APP_ADDR_BROADCAST)
    {
        return false;
    }

    /* MultiCast group is filtered in group query callback. */

    if ((filter->src_endpoint == SHARED_LIBDATA_UNUSED_ENDPOINT ||
         (uint8_t)filter->src_endpoint == data->src_endpoint) &&
        (filter->dest_endpoint == SHARED_LIBDATA_UNUSED_ENDPOINT ||
         (uint8_t)filter->dest_endpoint == data->dest_endpoint))
    {
        return true;
    }

    return false;
}

/**
 * \brief   Determine if the received packet belong to the group multicast
 *          based on the registered filters.
 * \note    Limitation : All accepted multicast packet will be received by
 *          multicast items and broadcast items.
 * \param   group_addr
 *          Group address.
 * \return  True if one of the filter belong to the group. False otherwise.
 */
static bool group_query_cb(app_addr_t group_addr)
{
    shared_libdata_item_t const * item;
    sl_list_t * i = sl_list_begin((sl_list_t *)&m_shared_data_head);

    lib_system->enterCriticalSection();
    while (i != sl_list_end((sl_list_t *)&m_shared_data_head))
    {
        item = (shared_libdata_item_t const *) i;

        if (!IS_MULTICAST(item->filter.mode) && !IS_ANYCAST(item->filter.mode))
        {
        }

        else if(item->filter.multicast_group ==
                                            SHARED_LIBDATA_UNUSED_MULTISCAST ||
                item->filter.multicast_group == group_addr)
        {
            return true;
        }

        i = sl_list_next(i);
    }
    lib_system->exitCriticalSection();

    return false;
}

static app_lib_data_receive_res_e received_cb(
                                        const app_lib_data_received_t * data)
{
    app_lib_data_receive_res_e res = APP_LIB_DATA_RECEIVE_RES_NOT_FOR_APP;
    shared_libdata_item_t * item;
    sl_list_t * i = sl_list_begin((sl_list_t *)&m_shared_data_head);

    LOG(LVL_DEBUG, "Received packet "
        "(src add: %u, dest add: %u, src ep: %d, dest ep: %d",
        data->src_address,
        data->dest_address,
        data->src_endpoint,
        data->dest_endpoint);

    m_iterating_list = true;

    while (i != sl_list_end((sl_list_t *)&m_shared_data_head))
    {
        item = (shared_libdata_item_t *) i;

        if (filter_received_packet(&item->filter, data))
        {
            app_lib_data_receive_res_e cb_res = item->cb(item, data);
            if (cb_res == APP_LIB_DATA_RECEIVE_RES_HANDLED)
            {
                res = APP_LIB_DATA_RECEIVE_RES_HANDLED;
            }
            /* Packet is dropped in case callback
             * returns APP_LIB_DATA_RECEIVE_RES_NO_SPACE.
             */
        }
        else
        {
            LOG(LVL_DEBUG, "Packet dropped");
        }


        i = sl_list_next(i);
    }

    /* Iterate the list and remove marked items. */
    lib_system->enterCriticalSection();
    m_iterating_list = false;

    i = sl_list_begin((sl_list_t *)&m_shared_data_head);

    while (i != sl_list_end((sl_list_t *)&m_shared_data_head))
    {
        item = (shared_libdata_item_t *) i;

        /* Get next item early so that item can be removed from the below. */
        i = sl_list_next(i);

        if (item->reserved2 == true)
        {
            Shared_LibData_removeDataReceivedCb(item);
        }
    }
    lib_system->exitCriticalSection();

    return res;
}

static void sent_cb(const app_lib_data_sent_status_t * status)
{
    app_lib_data_data_sent_cb_f cb;

    LOG(LVL_DEBUG, "Packet sent cb (id %u)", status->tracking_id);

    if(m_tracked_packets[status->tracking_id] != NULL)
    {
        cb = m_tracked_packets[status->tracking_id];
        /* Free the tracking id callback before calling it. */
        m_tracked_packets[status->tracking_id] = NULL;
        cb(status);
    }
}

app_res_e Shared_LibData_init(void)
{
    sl_list_init(&m_shared_data_head);

    m_iterating_list = false;

    memset(m_tracked_packets,0, sizeof(app_lib_data_data_sent_cb_f) * MAX_TRACKED_PACKET);

    /* Set callback for received unicast and broadcast messages. */
    lib_data->setDataReceivedCb(received_cb);
    lib_data->setBcastDataReceivedCb(received_cb);

    /* Set callback for multicast group query. */
    lib_settings->registerGroupQuery(group_query_cb);

    /* Set callback for sent packet. */
    lib_data->setDataSentCb(sent_cb);

    return APP_RES_OK;
}

app_res_e Shared_LibData_addDataReceivedCb(shared_libdata_item_t * item)
{
    if (item->cb == NULL)
    {
        return APP_RES_INVALID_NULL_POINTER;
    }

    /* Check filter parameters. */
    if (item->filter.mode > SHARED_LIBDATA_NET_MODE_ALL ||
        item->filter.dest_endpoint < ENDPOINT_MIN ||
        item->filter.dest_endpoint > ENDPOINT_MAX ||
        item->filter.src_endpoint < ENDPOINT_MIN ||
        item->filter.src_endpoint > ENDPOINT_MAX
        )
    {
        return APP_RES_INVALID_VALUE;
    }

    if ((item->filter.mode == SHARED_LIBDATA_NET_MODE_ALL ||
         item->filter.mode == SHARED_LIBDATA_NET_MODE_MULTICAST) &&
        (item->filter.multicast_group != SHARED_LIBDATA_UNUSED_MULTISCAST) &&
        (item->filter.multicast_group < 0x80000001 ||
         item->filter.multicast_group > 0x80FFFFFD)
        )
    {
        return APP_RES_INVALID_VALUE;
    }


    lib_system->enterCriticalSection();
    if (sl_list_contains(&m_shared_data_head, (sl_list_t *)item) == 0)
    {
        item->reserved2 = false;
        sl_list_push_back(&m_shared_data_head, (sl_list_t *)item);
    }
    lib_system->exitCriticalSection();

    LOG(LVL_DEBUG, "Add received cb (src ep: %d, dst ep: %d, mode: %u)",
                   item->filter.src_endpoint,
                   item->filter.dest_endpoint,
                   item->filter.mode);

    return APP_RES_OK;
}

void Shared_LibData_removeDataReceivedCb(shared_libdata_item_t * item)
{
    LOG(LVL_DEBUG, "Remove received cb (src ep: %d, dst ep: %d, mode: %u)",
                   item->filter.src_endpoint,
                   item->filter.dest_endpoint,
                   item->filter.mode);

    if(!m_iterating_list)
    {
        lib_system->enterCriticalSection();
        if ((sl_list_t *)item != sl_list_remove(&m_shared_data_head,
                                                (sl_list_t *)item))
        {
            memset(item, 0, sizeof(shared_libdata_item_t));
        }
        lib_system->exitCriticalSection();
    }
    else
    {
        item->reserved2 = true;
    }
}

app_lib_data_send_res_e Shared_LibData_sendData(
                                        app_lib_data_to_send_t * data,
                                        app_lib_data_data_sent_cb_f sent_cb)
{
    data->tracking_id = APP_LIB_DATA_NO_TRACKING_ID;

    if (sent_cb == NULL)
    {
        data->flags &= ~APP_LIB_DATA_SEND_FLAG_TRACK;
    }
    else
    {
        for (int i = 0; i < MAX_TRACKED_PACKET; i++)
        {
            /* Find the first available tracking Id. */
            if(m_tracked_packets[i] == NULL)
            {
                m_tracked_packets[i] = sent_cb;
                data->flags |= APP_LIB_DATA_SEND_FLAG_TRACK;
                data->tracking_id = i;
                break;
            }
        }

        if (data->tracking_id == APP_LIB_DATA_NO_TRACKING_ID)
        {
            /* No tracking Id available. */
            return APP_LIB_DATA_SEND_RES_OUT_OF_TRACKING_IDS;
        }
    }

    LOG(LVL_DEBUG, "Send with tracking id : %d (flag: %u)",
                   data->tracking_id,
                   data->flags);

    /* Send the data packet. */
    return lib_data->sendData(data);
}
