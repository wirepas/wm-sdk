/* Copyright 2019 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */
#include "shared_data.h"
#include <string.h>
#define DEBUG_LOG_MODULE_NAME "SHARED D"
#define DEBUG_LOG_MAX_LEVEL LVL_NOLOG
#include "debug_log.h"

#ifdef WIRESHARK
#include "wireshark.h"
#endif

/** Some helpers macros for packet filtering. */
#define IS_UNICAST(mode) (mode == SHARED_DATA_NET_MODE_UNICAST)
#define IS_BROADCAST(mode) (mode == SHARED_DATA_NET_MODE_BROADCAST)
#define IS_MULTICAST(mode) (mode == SHARED_DATA_NET_MODE_MULTICAST)
#define IS_ANYCAST(mode) (mode == SHARED_DATA_NET_MODE_ALL)

/** Helper macro to check if address is MULTICAST. */
#define IS_MULTICAST_ADDRESS(address) ((address & 0xff000000) == APP_ADDR_MULTICAST)

/** Helper macro to check if address is UNICAST. */
#define IS_UNICAST_ADDRESS(address) (address != APP_ADDR_BROADCAST && !IS_MULTICAST_ADDRESS(address))

/** Minimum endpoint value. */
#define ENDPOINT_MIN SHARED_DATA_UNUSED_ENDPOINT

/** Maximum endpoint value. */
#define ENDPOINT_MAX 255

/** Maximum number of buffers for tracked packets. */
#ifndef SHARED_DATA_MAX_TRACKED_PACKET
#define SHARED_DATA_MAX_TRACKED_PACKET 16
#endif

/** Head of data callbacks / filters linked list. */
static sl_list_head_t m_shared_data_head;

/** True when one function is iterating through the whole list. */
static bool m_iterating_list;

/** Internal structure to store data_sent cb set by different modules */
typedef struct {
    app_lib_data_data_sent_cb_f cb;
    app_lib_data_tracking_id_t id;
} tracked_packet_item_t;

/** Callbacks for packets being tracked. */
static tracked_packet_item_t m_tracked_packets[SHARED_DATA_MAX_TRACKED_PACKET];

/**
 * Is library initialized
 */
static bool m_initialized = false;


/**
 * @brief   Delete marked items from linked list.
 */
static void delete_marked_items(void)
{
    shared_data_item_t * item;
    sl_list_t * i = sl_list_begin((sl_list_t *)&m_shared_data_head);

    /* Iterate the list and remove marked items. */
    lib_system->enterCriticalSection();
    m_iterating_list = false;

    i = sl_list_begin((sl_list_t *)&m_shared_data_head);

    while (i != sl_list_end((sl_list_t *)&m_shared_data_head))
    {
        item = (shared_data_item_t *) i;

        /* Get next item early so that item can be removed from the below. */
        i = sl_list_next(i);

        if (item->reserved2 == true)
        {
            Shared_Data_removeDataReceivedCb(item);
        }
    }
    lib_system->exitCriticalSection();
}

/**
 * @brief   Enable recepton back if all itemas are ready
 */
static void enable_reception_if_needed(void)
{
    shared_data_item_t * item;
    bool ready = true;
    sl_list_t * i = sl_list_begin((sl_list_t *)&m_shared_data_head);

    m_iterating_list = true;

    while (i != sl_list_end((sl_list_t *)&m_shared_data_head))
    {
        item = (shared_data_item_t *) i;

        if (item->reserved3)
        {
            ready = false;
            break;
        }
        i = sl_list_next(i);
    }

    delete_marked_items();

    if (ready)
    {
        /* All filters are ready, notify stack */
        lib_data->allowReception(true);
    }
}

/**
 * @brief   Filter the received packet.
 * @param   filter
 *          Filter to apply to the received packet.
 * @param   data
 *          The received data packet.
 * @return  True if packet is to be received, false if packet is not for
 *          this callback.
 */
static bool filter_received_packet(const shared_data_filter_t * filter,
                                   const app_lib_data_received_t * data)
{
    /* Filter expect Unicast packet. */
    if (IS_UNICAST(filter->mode) && !IS_UNICAST_ADDRESS(data->dest_address))
    {
        /* Not an Unicast packet. */
        return false;
    }

    /* Filter expect Broadcast packet. */
    if (IS_BROADCAST(filter->mode) && data->dest_address != APP_ADDR_BROADCAST)
    {
        /* Not a Broadcast packet. */
        return false;
    }

    /* Filter expect Multicast packet. */
    if (IS_MULTICAST(filter->mode) &&
        !IS_MULTICAST_ADDRESS(data->dest_address))
    {
        /* Not a Multicast packet. */
        return false;
    }

    /* Check multicast cb for any multicast address if it exists */
    /* If we are at this point, filter already match multicast if multicast address */
    if (IS_MULTICAST_ADDRESS(data->dest_address))
    {
        /* If Not multicast group. */
        if (filter->multicast_cb != NULL &&
            filter->multicast_cb(data->dest_address) == false)
        {
            /* Multicast group doesn't match. */
            return false;
        }
    }

    /* Filter source endpoint. */
    if ((filter->src_endpoint != SHARED_DATA_UNUSED_ENDPOINT &&
        (uint8_t)filter->src_endpoint != data->src_endpoint))
    {
        return false;
    }

    /* Filter destination endpoint. */
    if ((filter->dest_endpoint != SHARED_DATA_UNUSED_ENDPOINT &&
        (uint8_t)filter->dest_endpoint != data->dest_endpoint))
    {
        return false;
    }

    return true;
}

/**
 * @brief   Determine if the received packet belong to at least one of the
 *          multicast group of the registered filters.
 * @note    Limitation : The multicast_cb will be called again in received_cb().
 * @param   group_addr
 *          Group address.
 * @return  True if one of the filter belong to the group. False otherwise.
 */
static bool group_query_cb(app_addr_t group_addr)
{
    shared_data_item_t * item;
    sl_list_t * i = sl_list_begin((sl_list_t *)&m_shared_data_head);

    LOG(LVL_DEBUG, "group_cb (addr: %u)", group_addr);

    while (i != sl_list_end((sl_list_t *)&m_shared_data_head))
    {
        item = (shared_data_item_t *) i;

        if (!IS_MULTICAST(item->filter.mode) && !IS_ANYCAST(item->filter.mode))
        {
        }

        else if (item->filter.multicast_cb == NULL)
        {
            /* Accept packet if callback is not set. */
            return true;
        }
        else if (item->filter.multicast_cb(group_addr))
        {
            return true;
        }

        i = sl_list_next(i);
    }

    delete_marked_items();

    return false;
}

static app_lib_data_receive_res_e received_cb(
                                        const app_lib_data_received_t * data)
{
    app_lib_data_receive_res_e res = APP_LIB_DATA_RECEIVE_RES_NOT_FOR_APP;
    shared_data_item_t * item;

#ifdef WIRESHARK
    Wireshark_print(
        data->src_address,
        data->dest_address,
        data->qos,
        data->src_endpoint,
        data->dest_endpoint,
        data->rssi,
        data->delay_hp,
        data->bytes,
        data->num_bytes
        );
#endif

    sl_list_t * i = sl_list_begin((sl_list_t *)&m_shared_data_head);

    LOG(LVL_DEBUG, "Rx (%u, %d -> %d)",
        data->dest_address,
        data->src_endpoint,
        data->dest_endpoint);

    m_iterating_list = true;

    while (i != sl_list_end((sl_list_t *)&m_shared_data_head))
    {
        item = (shared_data_item_t *) i;

        if (filter_received_packet(&item->filter, data))
        {
            app_lib_data_receive_res_e cb_res = item->cb(item, data);
            if (cb_res == APP_LIB_DATA_RECEIVE_RES_HANDLED &&
                res == APP_LIB_DATA_RECEIVE_RES_NOT_FOR_APP)
            {
                /* At lease one filter consumed the packet */
                res = cb_res;
            }
            else if (cb_res == APP_LIB_DATA_RECEIVE_RES_NO_SPACE)
            {
                LOG(LVL_DEBUG, "No space for filter (ep: %d -> %d, m: %u)",
                  item->filter.src_endpoint,
                  item->filter.dest_endpoint,
                  item->filter.mode);

                /* Filter cannot receive data anymore, mark it as busy */
                item->reserved3 = true;
                /* Report it to stack (It will not be overwritten by anyone else) */
                res = cb_res;
            }
        }
        else
        {
            /* Packet is dropped. */
        }

        i = sl_list_next(i);
    }

    delete_marked_items();

    return res;
}

static void sent_cb(const app_lib_data_sent_status_t * status)
{
    app_lib_data_data_sent_cb_f cb;
    app_lib_data_tracking_id_t module_id;

    LOG(LVL_DEBUG, "Tx done (id %u)", status->tracking_id);

    if(m_tracked_packets[status->tracking_id].cb != NULL)
    {
        cb = m_tracked_packets[status->tracking_id].cb;
        module_id = m_tracked_packets[status->tracking_id].id;

        /* Free the tracking id callback before calling it. */
        m_tracked_packets[status->tracking_id].cb = NULL;
        /* Update back the id with module set one */
        ((app_lib_data_sent_status_t *) status)->tracking_id = module_id;
        cb(status);
    }
}

app_res_e Shared_Data_init(void)
{
    if (m_initialized)
    {
        // Library already initialized
        return APP_RES_OK;
    }

#ifdef WIRESHARK
    Wireshark_init();
#endif


    sl_list_init(&m_shared_data_head);

    m_iterating_list = false;

    memset(m_tracked_packets, 0, sizeof(tracked_packet_item_t) *
                                 SHARED_DATA_MAX_TRACKED_PACKET);

    /* Set callback for received unicast and broadcast messages. */
    lib_data->setDataReceivedCb(received_cb);

    /* Set callback for multicast group query. */
    lib_settings->registerGroupQuery(group_query_cb);

    /* Set callback for sent packet. */
    lib_data->setDataSentCb(sent_cb);

    m_initialized = true;

    return APP_RES_OK;
}

app_res_e Shared_Data_addDataReceivedCb(shared_data_item_t * item)
{
    if (!m_initialized)
    {
        // It should be a different error code but
        // app_res_e doesn't have UNINITIALIZED error code
        return APP_RES_RESOURCE_UNAVAILABLE;
    }

    if (item->cb == NULL)
    {
        return APP_RES_INVALID_NULL_POINTER;
    }

    /* Check filter parameters. */
    if (item->filter.mode > SHARED_DATA_NET_MODE_ALL ||
        item->filter.dest_endpoint < ENDPOINT_MIN ||
        item->filter.dest_endpoint > ENDPOINT_MAX ||
        item->filter.src_endpoint < ENDPOINT_MIN ||
        item->filter.src_endpoint > ENDPOINT_MAX
        )
    {
        return APP_RES_INVALID_VALUE;
    }

    if (item->filter.mode != SHARED_DATA_NET_MODE_ALL &&
        item->filter.mode != SHARED_DATA_NET_MODE_MULTICAST &&
        item->filter.multicast_cb != NULL
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

    item->reserved3 = false;
    LOG(LVL_DEBUG, "Add Rx cb (ep: %d -> %d, m: %u)",
                   item->filter.src_endpoint,
                   item->filter.dest_endpoint,
                   item->filter.mode);

    return APP_RES_OK;
}

app_res_e Shared_Data_readyToReceive(shared_data_item_t * item)
{
    /* Clear the flag from current item if set */
    if (item->reserved3)
    {
        item->reserved3 = false;

        /* Check from all filters if it is time to enable reception back */
        enable_reception_if_needed();
    }

    return APP_RES_OK;
}

void Shared_Data_removeDataReceivedCb(shared_data_item_t * item)
{
    LOG(LVL_DEBUG, "Remove Rx cb (ep: %d -> %d, m: %u)",
                   item->filter.src_endpoint,
                   item->filter.dest_endpoint,
                   item->filter.mode);

    if(!m_iterating_list)
    {
        lib_system->enterCriticalSection();
        if ((sl_list_t *)item != sl_list_remove(&m_shared_data_head,
                                                (sl_list_t *)item))
        {
            if (item->reserved3)
            {
                enable_reception_if_needed();
            }
            memset(item, 0, sizeof(shared_data_item_t));
        }
        lib_system->exitCriticalSection();
    }
    else
    {
        item->reserved2 = true;
    }
}

app_lib_data_send_res_e Shared_Data_sendData(
                                        app_lib_data_to_send_t * data,
                                        app_lib_data_data_sent_cb_f sent_cb)
{
    app_lib_data_send_res_e res;

    if (!m_initialized)
    {
        return APP_LIB_DATA_SEND_RES_UNINITIALIZED;
    }

    if (sent_cb == NULL)
    {
        data->flags &= ~APP_LIB_DATA_SEND_FLAG_TRACK;
        // Just to be sure
        data->tracking_id = APP_LIB_DATA_NO_TRACKING_ID;
    }
    else
    {
        int free_slot = -1;
        for (int i = 0; i < SHARED_DATA_MAX_TRACKED_PACKET; i++)
        {
            /* Find the first available tracking Id. */
            if (m_tracked_packets[i].cb == NULL)
            {
                if (free_slot == -1)
                {
                    // Slot found
                    free_slot = i;
                }
            }
            else if (m_tracked_packets[i].cb == sent_cb &&
                     m_tracked_packets[i].id == data->tracking_id)
            {
                // Same tracking id already used by same cb
                // Two different cbs could reuse the same as they may belongs
                // to different modules
                LOG(LVL_DEBUG, "Invalid tracking id");
                return APP_LIB_DATA_SEND_RES_INVALID_TRACKING_ID;
            }
        }

        if (free_slot == -1)
        {
            /* No tracking Id available. */
            LOG(LVL_DEBUG, "No slot for tracked packet");
            return APP_LIB_DATA_SEND_RES_OUT_OF_TRACKING_IDS;
        }

        m_tracked_packets[free_slot].cb = sent_cb;
        m_tracked_packets[free_slot].id = data->tracking_id;
        data->flags |= APP_LIB_DATA_SEND_FLAG_TRACK;
        // Replace tracking id with internal table id
        // to easily access it later on. And it also allow
        // different app modules to use same tracking_id at the same time
        data->tracking_id = free_slot;
    }

    LOG(LVL_DEBUG, "Tx (id: %d, flag: %u)",
                   data->tracking_id,
                   data->flags);

    /* Send the data packet. */
    res = lib_data->sendData(data);

    if (res == APP_LIB_DATA_SEND_RES_SUCCESS)
    {
        app_addr_t node_addr;
        lib_settings->getNodeAddress(&node_addr);

#ifdef WIRESHARK
        Wireshark_print(
        // no rssi nor delay so 0 for both
            node_addr,
            data->dest_address,
            data->qos,
            data->src_endpoint,
            data->dest_endpoint,
            0,
            0,
            data->bytes,
            data->num_bytes
            );
#endif
    }

    /* Free resources if packet is tracked. */
    if (res != APP_LIB_DATA_SEND_RES_SUCCESS && sent_cb != NULL)
    {
        m_tracked_packets[data->tracking_id].cb = NULL;
    }
    return res;
}
