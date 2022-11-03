/**
 * @file    posapp_settings.c
 * @brief   Application settings module.
 *
 * @copyright  Wirepas Ltd 2021
 */

#define DEBUG_LOG_MODULE_NAME "POSAPP_SETTINGS"
#ifdef DEBUG_POSLIB_LOG_MAX_LEVEL
#define DEBUG_LOG_MAX_LEVEL DEBUG_POSLIB_LOG_MAX_LEVEL
#else
#define DEBUG_LOG_MAX_LEVEL LVL_NOLOG
#endif
#include "debug_log.h"
#include <string.h>
#include "api.h"
#include "posapp_settings.h"
#include "stack_state.h"

/*
 *  Network keys define in mcu/common/start.c and
 *  used only if default_network_cipher_key and default_network_authen_key
 *  are defined in one of the config.mk (set to NULL otherwise)
 */
extern const uint8_t * authen_key_p;
extern const uint8_t * cipher_key_p;

static app_lib_settings_role_t m_new_role;

void get_default_settings(poslib_settings_t * settings)
{
    /* Note all defined fields must be set to their default value */
    
    /* General settings */
    settings->node_mode = POSLIB_DEVICE_MODE;
    settings->node_class = POSLIB_DEVICE_CLASS;

    /* Update rate */
    settings->update_period_static_s = POSLIB_UPDATE_PERIOD_S;
    settings->update_period_dynamic_s = POSLIB_UPDATE_PERIOD_DYNAMIC_S; 
    settings->update_period_offline_s = POSLIB_UPDATE_PERIOD_OFFLINE_S;

    /* BLE settings */
    settings->ble.type = POSLIB_BLETX_TYPE;
    settings->ble.mode = POSLIB_BLETX_MODE;
    settings->ble.activation_delay_s = POSLIB_BLETX_ACTIVATION_DELAY_S;

    settings->ble.ibeacon.tx_interval_ms = POSLIB_BLETX_INTERVAL_MS;
    settings->ble.ibeacon.tx_power = POSLIB_BLETX_POWER;
    settings->ble.ibeacon.channels = APP_LIB_BEACON_TX_CHANNELS_ALL;

    settings->ble.eddystone.tx_interval_ms = POSLIB_BLETX_INTERVAL_MS;
    settings->ble.eddystone.tx_power = POSLIB_BLETX_POWER;
    settings->ble.eddystone.channels = APP_LIB_BEACON_TX_CHANNELS_ALL;

#ifdef MOTION_SUPPORTED
    settings->motion.enabled = true;
    settings->motion.threshold_mg = POSAPP_MOTION_THRESHOLD_MG;
    settings->motion.duration_ms = POSAPP_MOTION_DURATION_MS;
#else
    settings->motion.enabled = false;
    settings->motion.threshold_mg = 0;
    settings->motion.threshold_mg = 0;
#endif

    // Mini-beacon 
    settings->mbcn.enabled = POSLIB_MBCN_ENABLED;
    settings->mbcn.tx_interval_ms = POSLIB_MBCN_TX_INTERVAL_MS;
    memset(settings->mbcn.records, 0 , sizeof(settings->mbcn.records));
    // Default custom records can be initialized here
    settings->da.routing_enabled = POSLIB_DA_ROUTING_ENABLED;
    settings->da.follow_network = POSLIB_DA_FOLLOW_NETWORK;
}

static void stack_state_cb(app_lib_stack_event_e event, void * param)
{
    app_res_e res;
    // We are registered only for Stack stopped event
    res = lib_settings->setNodeRole(m_new_role);

    if (res == APP_RES_OK)
    {
        LOG(LVL_INFO, "New role %u set", m_new_role);
    }
    else
    {
        LOG(LVL_ERROR, "Cannot set new role %u", m_new_role);
    }
}

static app_lib_settings_role_t enforce_node_mode(poslib_settings_t * settings, 
                                        app_lib_settings_role_t role)
{
    app_lib_settings_role_t new_role = role;
    bool is_ll_mode = (role == APP_LIB_SETTINGS_ROLE_HEADNODE_LL ||
                       role == APP_LIB_SETTINGS_ROLE_SUBNODE_LL ||
                       role == APP_LIB_SETTINGS_ROLE_AUTOROLE_LL ||
                       role == APP_LIB_SETTINGS_ROLE_SINK_LL);

    switch(settings->node_mode)
    {
        case POSLIB_MODE_AUTOSCAN_ANCHOR:
        case POSLIB_MODE_OPPORTUNISTIC_ANCHOR:
            // Check if it is a headnode or a subnode with mini beacons enabled
            if (!(role == APP_LIB_SETTINGS_ROLE_HEADNODE_LE ||
                  role == APP_LIB_SETTINGS_ROLE_HEADNODE_LL ||
                  (role == APP_LIB_SETTINGS_ROLE_SUBNODE_LE && settings->mbcn.enabled) ||
                  (role == APP_LIB_SETTINGS_ROLE_SUBNODE_LL && settings->mbcn.enabled)))
            {
                // Need to force to Headnode with same mode as before:
                if (is_ll_mode)
                {
                    new_role = APP_LIB_SETTINGS_ROLE_HEADNODE_LL;
                }
                else
                {
                    new_role = APP_LIB_SETTINGS_ROLE_HEADNODE_LE;
                }
            }
            break;
        case POSLIB_MODE_NRLS_TAG:
        case POSLIB_MODE_AUTOSCAN_TAG:
                if (is_ll_mode)
                {
                    new_role = APP_LIB_SETTINGS_ROLE_SUBNODE_LL;
                }
                else
                {
                    new_role = APP_LIB_SETTINGS_ROLE_SUBNODE_LE;
                }
            break;
        case POSLIB_MODE_DA_TAG:
            new_role = APP_LIB_SETTINGS_ROLE_ADVERTISER;
            break;
        default:
            break;
    }
    return new_role;
}

static uint8_t enforce_node_role(poslib_settings_t * settings, 
                                app_lib_settings_role_t role)
{
    uint8_t node_mode = settings->node_mode;
   
    switch (role)
    {
        case APP_LIB_SETTINGS_ROLE_HEADNODE_LL:
        case APP_LIB_SETTINGS_ROLE_HEADNODE_LE:
            if (node_mode != POSLIB_MODE_AUTOSCAN_ANCHOR || 
                node_mode != POSLIB_MODE_OPPORTUNISTIC_ANCHOR)
            {
                node_mode = POSAPP_ANCHOR_DEFAULT_ROLE;
                LOG(LVL_INFO, "Setting node mode to anchor default: %u",  node_mode);
            }
            break;
        case APP_LIB_SETTINGS_ROLE_SUBNODE_LL:
        case APP_LIB_SETTINGS_ROLE_SUBNODE_LE:
            if (!(node_mode == POSLIB_MODE_NRLS_TAG ||
                  node_mode == POSLIB_MODE_AUTOSCAN_TAG ||
                  ((node_mode == POSLIB_MODE_AUTOSCAN_ANCHOR ||
                    node_mode == POSLIB_MODE_OPPORTUNISTIC_ANCHOR)
                    && settings->mbcn.enabled)))
            {
                node_mode = POSAPP_TAG_DEFAULT_ROLE;
                LOG(LVL_INFO, "Setting node mode to tag default: %u",  node_mode);
            }
            break;
        case APP_LIB_SETTINGS_ROLE_ADVERTISER:
            if (node_mode != POSLIB_MODE_DA_TAG)
            {
                node_mode = POSLIB_MODE_DA_TAG;
                LOG(LVL_INFO, "Setting node mode to DA tag default: %u",  node_mode);
            }
            break;
        default:
            LOG(LVL_ERROR,"Node role %u unknown", role);
    }
    return node_mode;
}


static void check_role(poslib_settings_t * settings, bool force_set_role)
{
    app_lib_settings_role_t role;
    app_lib_settings_role_t new_role;
    uint8_t node_mode = settings->node_mode; 


    lib_settings->getNodeRole(&role);
    new_role = role;
  
    if (force_set_role)
    {
        // Enforce the positioning mode -> node role might change
       new_role = enforce_node_mode(settings, role);
    }
    else
    {
        // Ensure node mode matches node role -> when differ enforce 
        // default mode for the role
        settings->node_mode = enforce_node_role(settings, role);
    }
   
    (void) node_mode;
    LOG(LVL_INFO, "Mode mode: %u->%u, role %u->%u",  node_mode, settings->node_mode, 
                                                     role, new_role);

    if (force_set_role && role != new_role)
    {
        app_res_e res;
        res = Stack_State_addEventCb(stack_state_cb, 1 << APP_LIB_STATE_STACK_EVENT_STACK_STOPPED);

        if (res != APP_RES_OK)
        {
            LOG(LVL_ERROR, "Cannot register stack state CB. res: %u", res);
            return;
        }

        m_new_role = new_role;
        LOG(LVL_WARNING, "Stopping stack");
        lib_state->stopStack();
    }
}


#if defined(CONF_USE_PERSISTENT_MEMORY)
/**
 * @brief   Converts a poslib_persistent_settings_t structure into 
 *          poslib_settings_t
 *          ! Warning only common fields will be updated
 * @param   p 
 *          pointer to source poslib_persistent_settings_t structure 
 *          input configuration      
 * @param   s
 *          pointer to destination poslib_settings_t structure

 * @return  bool
 *          true: conversion succeded, false: input not valid, 
 *          output not updated
 */
static bool persistent_to_poslib(poslib_persistent_settings_t * p, 
                                poslib_settings_t * s)
{
    /* Check persistent data validity */
    if (p->poslib_record_magic != POSLIB_RECORD_MAGIC)
    { 
        return false;
    }
    
    /*All fields valid from version 1 onwards filled here */
    if(p->poslib_settings_version > 0)
    {
        /* General settings */
        s->node_class = p->node_class;
        s->node_mode = p->node_mode;

        /*Update rate */
        s->update_period_static_s = p->update_period_static_s;
        s->update_period_offline_s = p->update_period_offline_s;
        s->update_period_dynamic_s = p->update_period_dynamic_s;

        /*BLE settings */
        s->ble.type = p->ble_type;
        s->ble.mode = p->ble_mode;

        s->ble.eddystone.channels =  p->ble_eddystone_channels;
        s->ble.eddystone.tx_interval_ms = p->ble_eddystone_tx_interval_ms;
        s->ble.eddystone.tx_power = p->ble_eddystone_tx_power;
        
        s->ble.ibeacon.channels = p->ble_ibeacon_channels;
        s->ble.ibeacon.tx_interval_ms = p->ble_ibeacon_tx_interval_ms;
        s->ble.ibeacon.tx_power = p->ble_ibeacon_tx_power;
        
        s->ble.activation_delay_s = p->ble_activation_delay_s;

#ifdef MOTION_SUPPORTED
        s->motion.enabled =  p->motion_enabled;
        s->motion.threshold_mg =  p->motion_threshold_mg;
        s->motion.duration_ms =  p->motion_duration_ms;
#else
        s->motion.enabled =  false;
        s->motion.threshold_mg =  0; //FixMe: define invalid values
        s->motion.duration_ms =  0;
#endif
    }

    /* All fields valid from version N onwards filled here under 
        version check */
    if (p->poslib_settings_version == 2)
    {
        /* Mini-beacon settings */
        s->mbcn.enabled = p->mbcn_enabled;
        s->mbcn.tx_interval_ms = p->mbcn_tx_interval_ms;
        uint8_t i;
        for (i = 0; i < POSLIB_MBCN_RECORDS; i++)
        {
            s->mbcn.records[i].type = p->mbcn_records[i].type;
            s->mbcn.records[i].length = p->mbcn_records[i].length;
            memcpy(s->mbcn.records[i].value, p->mbcn_records[i].value, s->mbcn.records[i].length);
        }

        /* DA settings */
        s->da.routing_enabled = p->da_routing_enabled;
        s->da.follow_network = p->da_follow_network;
    }
    return true;
}

/**
 * @brief   Converts a poslib_settings_t structure into 
 *          poslib_persistent_settings_t 
 *          ! Warning only common fields will be updated
 * @param   s
 *          pointer to source poslib_settings_t structure
 * @param   p 
 *          pointer to destination poslib_persistent_settings_t structure 
 *          input configuration
 * @return  void
 */
static void poslib_to_persistent(poslib_settings_t * s, 
                                poslib_persistent_settings_t * p)
{

    /* Last version is used always */
    p->poslib_record_magic = POSLIB_RECORD_MAGIC;
    p->poslib_settings_version = POSLIB_PERSISTENT_VERSION;

    /* General settings */
    p->node_class = s->node_class;
    p->node_mode = s->node_mode;

    /*Update rate */
    p->update_period_static_s = s->update_period_static_s;
    p->update_period_offline_s = s->update_period_offline_s;
    p->update_period_dynamic_s = s->update_period_dynamic_s;
    
    /*BLE settings */
    p->ble_type = s->ble.type;
    p->ble_mode = s->ble.mode;

    p->ble_eddystone_channels = s->ble.eddystone.channels;
    p->ble_eddystone_tx_interval_ms = s->ble.eddystone.tx_interval_ms;
    p->ble_eddystone_tx_power = s->ble.eddystone.tx_power;

    p->ble_ibeacon_channels = s->ble.ibeacon.channels;
    p->ble_ibeacon_tx_interval_ms = s->ble.ibeacon.tx_interval_ms;
    p->ble_ibeacon_tx_power = s->ble.ibeacon.tx_power;

    p->ble_activation_delay_s = s->ble.activation_delay_s;

#ifdef MOTION_SUPPORTED
    p->motion_enabled = s->motion.enabled;
    p->motion_threshold_mg = s->motion.threshold_mg;
    p->motion_duration_ms = s->motion.duration_ms;
#else
    p->motion_enabled = false;
    p->motion_threshold_mg = 0; //FixMe: define invalid values
    p->motion_duration_ms = 0;
#endif

    /* Mini-beacon settings */
    p->mbcn_enabled = s->mbcn.enabled;
    p->mbcn_tx_interval_ms = s->mbcn.tx_interval_ms;
    uint8_t i;
    for (i = 0; i < POSLIB_MBCN_RECORDS; i++)
    {
        p->mbcn_records[i].type = s->mbcn.records[i].type;
        p->mbcn_records[i].length = s->mbcn.records[i].length;
        memcpy(p->mbcn_records[i].value, s->mbcn.records[i].value, s->mbcn.records[i].length);
    }

    /* DA settings */
    p->da_routing_enabled = s->da.routing_enabled;
    p->da_follow_network = s->da.follow_network;
}

bool PosApp_Settings_store(poslib_settings_t * settings)
{
    posapp_persistent_settings_t posapp_persistent_old;
    posapp_persistent_settings_t posapp_persistent_new;

    poslib_to_persistent(settings, &posapp_persistent_new.poslib);
   
    if (App_Persistent_read((uint8_t *)&posapp_persistent_old, sizeof(posapp_persistent_settings_t)) == APP_PERSISTENT_RES_OK)
    {
        if(memcmp(&posapp_persistent_old.poslib, &posapp_persistent_new.poslib, sizeof(poslib_persistent_settings_t)) == 0)
        {
            LOG(LVL_DEBUG, "Settings not updated, skip flash write");
            return false;
        }
    }

    /* Save settings if: different than previous | not yet saved | previous corrupted */
    if (App_Persistent_write((uint8_t *)&posapp_persistent_new, sizeof(posapp_persistent_settings_t)) == APP_PERSISTENT_RES_OK)
    {
        LOG(LVL_DEBUG, "PosLib settings writen to flash");
    }
    else
    {
        LOG(LVL_ERROR, "PosLib settings flash write failed");
    }

    check_role(settings, true);
    return true;
}

#else
bool PosApp_Settings_store(poslib_settings_t * settings)
{
    return false;
}
#endif


bool PosApp_Settings_configureNode(void)
{

    bool ret = true;
    app_addr_t node_address =  getUniqueAddress();
    app_lib_settings_net_addr_t network_address= CONF_NETWORK_ADDRESS;
    app_lib_settings_net_channel_t network_channel = CONF_NETWORK_CHANNEL;
    app_lib_settings_role_t node_role = CONF_ROLE;

#if defined(CONF_USE_PERSISTENT_MEMORY)
    /*Update settings from persistent storage*/
    posapp_persistent_settings_t persistent;
    app_persistent_res_e res;
    res = App_Persistent_read((uint8_t *)&persistent, sizeof(posapp_persistent_settings_t));
    LOG(LVL_DEBUG, "Persistent read res: %u",res);
    if (res == APP_PERSISTENT_RES_OK)
    {
        //Node address
        if (persistent.node.address != 0xFFFFFF)
        {
            node_address = persistent.node.address;
        }
        // Network address
        if (persistent.node.network_address != 0xFFFFFF)
        {
            network_address = persistent.node.network_address;
        }
        // Network channel
        if (persistent.node.network_channel != 0xFF)
        {
            network_channel = persistent.node.network_channel;
        }
        // Node role
        if (persistent.node.role != 0xFF)
        {
            node_role = persistent.node.role;
        }
    }
#endif

    LOG(LVL_DEBUG, "Node address, Network role, Network channel, Network role: %u, %u, %u, %u", node_address, network_address, network_channel, node_role);
    /*Node role will be set only once at first boot */
    {
        app_addr_t na = 0;
        if(lib_settings->getNodeAddress(&na) == APP_RES_INVALID_CONFIGURATION &&
            lib_settings->setNodeRole(node_role) != APP_RES_OK)
        {
            LOG(LVL_ERROR, "Cannot set node role to: %u, node_role");
            ret = false;
        }
        else
        {
            LOG(LVL_INFO, "Role not set. Add: %u", na);
        }
    }

    /* Configuration will be applied only for the  parameters which 
    are not yet set */
    
    if (configureNode(node_address, network_address, network_channel,
                    authen_key_p, cipher_key_p) != APP_RES_OK)
    {
        LOG(LVL_ERROR, "Cannot set node parameters");
        ret = false;
    }

/* Print node settings if debug active */
#ifdef USE_DEBUG_PRINT_MODULE

    lib_settings->getNodeAddress(&node_address);
    lib_settings->getNetworkAddress(&network_address);
    lib_settings->getNetworkChannel(&network_channel);
    lib_settings->getNodeRole(&node_role);

    LOG(LVL_INFO, "Node configuration: addr: %u, nw_addr: %u, nw_ch: %u, role: %u",
        node_address, network_address, network_channel, node_role);
#endif

    return ret;
}

void PosApp_Settings_get(poslib_settings_t * settings)
{
    
    get_default_settings(settings);

#if defined(CONF_USE_PERSISTENT_MEMORY)
   {
        posapp_persistent_settings_t persistent;
        if (App_Persistent_read((uint8_t *)&persistent, sizeof(posapp_persistent_settings_t)) == APP_PERSISTENT_RES_OK)
        {
            persistent_to_poslib(&persistent.poslib, settings);
        }
   } 
#endif
    check_role(settings, false); 
}
