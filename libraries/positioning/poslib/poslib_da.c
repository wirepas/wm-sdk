/**
 * @file       poslib_da.c
 * @brief      Directed-advertiser module.
 *
 * @copyright  Wirepas Ltd 2021
 */

#define DEBUG_LOG_MODULE_NAME "POSLIB_DA"
#ifdef DEBUG_POSLIB_LOG_MAX_LEVEL
#define DEBUG_LOG_MAX_LEVEL DEBUG_POSLIB_LOG_MAX_LEVEL
#else
#define DEBUG_LOG_MAX_LEVEL LVL_NOLOG
#endif
#include "debug_log.h"
#include "poslib.h"
#include "poslib_da.h"
#include "shared_data.h"
#include "poslib_measurement.h"
#include "poslib_control.h"
#include <string.h>
#include <stdlib.h>

#define MAX_PAYLOAD_LEN 102 //FixMe: move this to poslib.h (there is no generic config in app.h)

/* Router specific structures */
// received filter used by routed to receive positioning packets from tag
static shared_data_item_t m_router_pos_item;

//received data filter used by tag to receive ACK data from router
static shared_data_item_t m_tag_ack_item;
 
static uint16_t m_sequence = 0;  // the sequence of the rising packets

#define IS_DA_ROUTER(x) (x == APP_LIB_STATE_DIRADV_SUPPORTED)
#define IS_UNKNOWN_DA_ROUTER(x) (x == APP_LIB_STATE_DIRADV_UNKNOWN)
#define IS_NOT_DA_ROUTER(x) (x == APP_LIB_STATE_DIRADV_NOT_SUPPORTED)
#define ROUTER_COST_INVALID(x) (x == APP_LIB_STATE_INVALID_ROUTE_COST || x == APP_LIB_STATE_COST_UNKNOWN)

#define MAX_NBORS 20 
#define NBOR_LAST_SEEN_WINDOW 2
#define NO_ROUTE_FOUND_ADDRESS 0

app_lib_state_nbor_info_t m_nbors[MAX_NBORS];
app_lib_state_nbor_list_t m_nbors_list =
{
    .number_nbors = MAX_NBORS,
    .nbors = m_nbors,
};


/**
 * @brief   Callback for router positioning data reception
 * @param[in]   item pointer to shared data filter \ref shared_data_item_t        
 * @param[in]   data pointer to received data \ref app_lib_data_received_t         
 * @return  See \ref app_lib_data_receive_res_e
 */
static app_lib_data_receive_res_e router_pos_data_cb(const shared_data_item_t * item,
                        const app_lib_data_received_t * data)
{
    uint8_t bytes[MAX_PAYLOAD_LEN];
    uint8_t len = 0;
    app_lib_data_to_send_t payload = {
        .bytes = NULL,
        .num_bytes = 0,
        .dest_address = APP_ADDR_ANYSINK,
        .src_endpoint = POS_SOURCE_ENDPOINT,
        .dest_endpoint = POS_DESTINATION_ENDPOINT,
        .qos = APP_LIB_DATA_QOS_NORMAL,
        .delay = 0,
        .flags = APP_LIB_DATA_SEND_FLAG_NONE,
        .tracking_id = 0,
    };
    poslib_meas_message_header_t msg_header;
    poslib_meas_record_da_t da;
    app_lib_data_send_res_e res;
    
    if (item->filter.src_endpoint != POSLIB_DA_SRC_EP &&
            item->filter.dest_endpoint != POSLIB_DA_DEST_EP)
    {

        LOG(LVL_ERROR, "Incorect EP: %u/%u", 
            item->filter.src_endpoint, item->filter.dest_endpoint);
        return APP_LIB_DATA_RECEIVE_RES_HANDLED;
    }

    if (data->num_bytes == 0)
    {
        LOG(LVL_INFO, "Empty payload received from: %u", data->mac_src_address);
        return APP_LIB_DATA_RECEIVE_RES_HANDLED;
    }

    // Add generic mesage header
    msg_header.sequence = m_sequence;
    m_sequence++;
    memcpy(bytes + len, &msg_header, sizeof(msg_header));
    len += sizeof(msg_header);

    //Add DA record
    da.header.type =  POSLIB_MEAS_DA;
    da.header.length = data->num_bytes + sizeof(da.node_addr);
    da.node_addr = data->mac_src_address;
    memcpy(bytes+len, &da, sizeof(da));
    len += sizeof(da);

    // Copy full message from node
    if (data->num_bytes <= sizeof(bytes) - len)
    {
        memcpy(bytes + len, data->bytes, data->num_bytes);
        len += data->num_bytes;
    }
    else
    {
        LOG(LVL_ERROR, "Cannot send! Too large payload: %u bytes", data->num_bytes);
        return APP_LIB_DATA_RECEIVE_RES_HANDLED;
    }

    // send data
    payload.bytes = bytes;
    payload.num_bytes = len;
    payload.delay = data->delay; // Inherits incoming packet delay

    res = Shared_Data_sendData(&payload, NULL);
    if (res == APP_LIB_DATA_SEND_RES_SUCCESS)
    {
        LOG(LVL_DEBUG, "Sent DA data - src: %u", data->mac_src_address);
    }
    else
    {
        LOG(LVL_ERROR, "Failed to send DA data. Error %u", res);
    }
    return APP_LIB_DATA_RECEIVE_RES_HANDLED;
}


static bool router_ack_cb(const ack_gen_input_t * in, ack_gen_output_t * out)
{
    
    if (in->src_endpoint == POS_SOURCE_ENDPOINT &&
        in->dest_endpoint == POS_DESTINATION_ENDPOINT)
    {
        uint8_t * cfg;
        uint8_t len;
        PosLibCtrl_getAppConfig(&cfg, &len);  //FixMe: extend config also to app part
        out->data = cfg;
        out->length = len;

        LOG(LVL_DEBUG, "DA ACK sent. addr: %u len: %u",  in->sender, len);
    }
    else
    {
        out->data = NULL;
        out->length = 0;
        LOG(LVL_DEBUG, "DA ACK void addr: %u, src_ep: %u, dest_ep: %u app cfg: %u", 
        in->sender, in->src_endpoint, in->dest_endpoint);
    }
    return true;
}

static void init_router_pos_item()
{
    memset(&m_router_pos_item, 0, sizeof(m_router_pos_item));
    m_router_pos_item.cb = router_pos_data_cb;
    m_router_pos_item.filter.mode = SHARED_DATA_NET_MODE_UNICAST;
    m_router_pos_item.filter.src_endpoint = POSLIB_DA_SRC_EP;
    m_router_pos_item.filter.dest_endpoint = POSLIB_DA_DEST_EP;
    m_router_pos_item.filter.multicast_cb = NULL;
}

bool start_router()
{
    app_lib_settings_role_t role;
    app_res_e res;

    lib_settings->getNodeRole(&role);
    
    // Activation posible only for LL headnode
    if ( role != APP_LIB_SETTINGS_ROLE_HEADNODE_LL)
    {
        LOG(LVL_INFO, "Only LL router supported. Role: %u", role);
        return false;
    }
    
    // FixME: replace with shared DA library when available
    lib_advertiser->setRouterAckGenCb(router_ack_cb);

    init_router_pos_item();
    res = Shared_Data_addDataReceivedCb(&m_router_pos_item);
    if (res != APP_RES_OK)
    {
        m_router_pos_item.cb = NULL;
        LOG(LVL_ERROR, "Cannot register DA data reception");
        return false;
    }
    
    LOG(LVL_INFO, "DA router started!");
    return true;
}

static void stop_router()
{
    // FixME: replace with shared DA library when available
    lib_advertiser->setRouterAckGenCb(NULL);
    if (m_router_pos_item.cb != NULL)
    {
        Shared_Data_removeDataReceivedCb(&m_router_pos_item);
        m_router_pos_item.cb = NULL;
    }
    LOG(LVL_INFO, "DA router stoped!");
}


/**
 * \brief       Compares two neigbours based on DA router status, latest update, RSSI
 * \param       pa, pb pointer to the a & b data structures
 *              Exclude this address from selectable routers. 0 if not used.
 * \return      -1 if a is selected, 1 is b is selected, 0 if equal
 */


static int compare_neighbours(const void * pa, const void *pb)
{
    app_lib_state_nbor_info_t * a = (app_lib_state_nbor_info_t* ) pa;
    app_lib_state_nbor_info_t * b = (app_lib_state_nbor_info_t* ) pb;
    int32_t dt;

    // compare using last update/RSSI if A and B are both valid or both unknown
    if ((IS_DA_ROUTER(a->diradv_support) && IS_DA_ROUTER(b->diradv_support)) ||
        (IS_UNKNOWN_DA_ROUTER(a->diradv_support) && IS_UNKNOWN_DA_ROUTER(b->diradv_support)))
    {
        dt = a->last_update - b->last_update; 
        if (abs(dt) > NBOR_LAST_SEEN_WINDOW)
        {
            // select latest updated
            return dt;
        }
        // Select strongest RSSI
        return (b->norm_rssi - a->norm_rssi); 
    } 
    else if (IS_DA_ROUTER(a->diradv_support) ||
            (IS_UNKNOWN_DA_ROUTER(a->diradv_support) && IS_NOT_DA_ROUTER(b->diradv_support)))
    {
        // A valid, B uknown/invalid -> select A
        return -1;
    }
    else if (IS_DA_ROUTER(b->diradv_support) ||
            (IS_UNKNOWN_DA_ROUTER(b->diradv_support) && IS_NOT_DA_ROUTER(a->diradv_support)))
    {
        // B valid, A uknown/invalid -> select B
        return 1;
    }
    // A & B invalid -> equal
    return 0;    
}

/**
 * \brief  Updates neiggbours list stored in m_nbors_list
 *         sorted based on criteria implemented in 
 * \return  number of neigbours valid for DA communication
 */
static void update_neigbours()
{
    m_nbors_list.number_nbors = MAX_NBORS;
    lib_state->getNbors(&m_nbors_list);

    qsort(m_nbors_list.nbors, m_nbors_list.number_nbors, 
            sizeof(m_nbors_list.nbors[0]), compare_neighbours);

    for (uint8_t i = 0; i < m_nbors_list.number_nbors; i++)
    {
        LOG(LVL_DEBUG,"neigh address: %u da: %u type: %u rssi: %i cost: %u last: %u", m_nbors[i].address, 
                m_nbors[i].diradv_support, m_nbors[i].type, m_nbors[i].norm_rssi,  m_nbors[i].cost,  m_nbors[i].last_update); 
    }
}

static app_lib_data_receive_res_e tag_ack_cb(const shared_data_item_t * item,
                        const app_lib_data_received_t * data)
{
    if (item->filter.src_endpoint == DIRADV_EP_SRC_ACK &&
            item->filter.dest_endpoint == DIRADV_EP_DEST)
    {
        if (data->num_bytes > 0)
        {
            PosLibCtrl_setAppConfig(data->bytes, data->num_bytes);
        }
        LOG(LVL_DEBUG, "DA ACK inject app config. bytes: %u", data->num_bytes);
    }
    else
    {
        LOG(LVL_ERROR, "Incorect EP: %u/%u", 
            item->filter.src_endpoint, item->filter.dest_endpoint);
    }
    return APP_LIB_DATA_RECEIVE_RES_HANDLED;
}

static void init_tag_ack_item()
{
    memset(&m_tag_ack_item, 0, sizeof(m_tag_ack_item));
    m_tag_ack_item.cb = tag_ack_cb;
    m_tag_ack_item.filter.mode = SHARED_DATA_NET_MODE_UNICAST,
    m_tag_ack_item.filter.src_endpoint = DIRADV_EP_SRC_ACK,
    m_tag_ack_item.filter.dest_endpoint = DIRADV_EP_DEST,
    m_tag_ack_item.filter.multicast_cb = NULL;
}

static bool start_tag(poslib_da_settings_t * da_settings)
{
    app_res_e res;
    adv_option_t option;
    bool ret; 

    option.follow_network = da_settings->follow_network;
    lib_advertiser->setOptions(&option);
    LOG(LVL_DEBUG, "DA tag - follow network: %u", option.follow_network);

    init_tag_ack_item();
    res = Shared_Data_addDataReceivedCb(&m_tag_ack_item);

    if (res == APP_RES_OK)
    {
        LOG(LVL_DEBUG, "Registered tag DA ACK data EP: %u/%u", 
            m_tag_ack_item.filter.src_endpoint,  m_tag_ack_item.filter.dest_endpoint);
    }
    else
    {
        m_tag_ack_item.cb = NULL;
        LOG(LVL_ERROR, "Cannot register DA tag ACK reception");
        ret = false;
    }
    return ret;
}

static void stop_tag()
{
    if (m_tag_ack_item.cb != NULL)
    {
       Shared_Data_removeDataReceivedCb(&m_tag_ack_item); 
    }
}

void PosLibDa_stop()
{
    stop_router();
    stop_tag();
}

bool PosLibDa_start(poslib_settings_t * settings)
{
    bool ret = true;
 
    switch(settings->node_mode)
    {
        case POSLIB_MODE_DA_TAG:
        {
            stop_router();
            ret = start_tag(&settings->da);
            break;
        }
        case POSLIB_MODE_AUTOSCAN_ANCHOR:
        case POSLIB_MODE_OPPORTUNISTIC_ANCHOR:
        { 
            if (settings->da.routing_enabled)
            {
                stop_tag();
                ret = start_router();
            }
            break;
        }
        default:
        {
            LOG(LVL_INFO, "Non-DA mode: %u", settings->node_mode);
            break;
        }
    }
    return ret;
}

app_lib_data_send_res_e PosLibDa_sendData(app_lib_data_to_send_t * data,
                                            app_lib_data_data_sent_cb_f sent_cb)
{
    app_lib_settings_role_t role;
    app_lib_data_send_res_e res = APP_LIB_DATA_SEND_RES_INVALID_DEST_ADDRESS; 
    lib_settings->getNodeRole(&role);

    /** If: role is not DA or destination_address is a unicast send directly */
    if (role != APP_LIB_SETTINGS_ROLE_ADVERTISER ||
        data->dest_address != APP_ADDR_ANYSINK)
    {
        return Shared_Data_sendData(data, sent_cb);
    }

    /* DA data sending - loop on updated neigbours until a DA cluster is found */
    update_neigbours();

    for (uint8_t i = 0; i < m_nbors_list.number_nbors; i++)
    {
       
       if (ROUTER_COST_INVALID(m_nbors[i].cost))
       {
           continue;
       }
       data->dest_address = m_nbors[i].address;
       res =  Shared_Data_sendData(data, sent_cb);
       if (res != APP_LIB_DATA_SEND_RES_INVALID_DEST_ADDRESS)
       {
           LOG(LVL_INFO, "DA data send. router: %u", m_nbors[i].address, res);
           break;
       }
        LOG(LVL_WARNING, "Fail DA data send. router: %u, res: %u", m_nbors[i].address, res);
    }
    return res;
}