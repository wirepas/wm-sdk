/**
 * @file       poslib_control.c
 * @brief      contains the routines with Poslib API and general control.
 * @copyright  Wirepas Ltd 2021
 */
#define DEBUG_LOG_MODULE_NAME "POSLIB CONTROL"
#ifdef DEBUG_POSLIB_LOG_MAX_LEVEL
#define DEBUG_LOG_MAX_LEVEL DEBUG_POSLIB_LOG_MAX_LEVEL
#else
#define DEBUG_LOG_MAX_LEVEL LVL_NOLOG
#endif
#include "debug_log.h"
#include <string.h>
#include "api.h"
#include "app_scheduler.h"
#include "random.h"
#include "poslib.h"
#include "posapp_settings.h"
#include "poslib_ble_beacon.h"
#include "poslib_control.h"
#include "poslib_event.h"
#include "poslib_measurement.h"
#include "poslib_decode.h"
#include "poslib_da.h"
#include "shared_data.h"
#include "shared_appconfig.h"
#include "shared_offline.h"
#include "poslib_mbcn.h"
#ifdef ROUTE_CHECK
#include "stack_state.h"
#endif

/** Module internal type definitions */

typedef struct {
    bool online;
    bool offline;
    bool scan_start;
    bool scan_end;
    bool data_sent;
    bool data_sent_complete;
    bool app_cfg;
    bool timeout;
    bool oneshot;
    bool offline_req;
    bool sent_success;
    bool connected;
    bool scan_fail;
    bool is_static;
} control_events_t;

/* Scheduled state */
typedef struct {
    poslib_state_e state;
    poslib_mode_e mode;
    bool scheduled;
    bool outside_wm;
    uint16_t sequence;
    control_events_t events;
    uint32_t last_update_s;
    uint32_t next_update_s;
    uint32_t update_start_hp;
} control_state_t;

typedef struct {
    bool reg;
    uint16_t id; //FixME: all Shared libraries should use uint8_t
} callback_state_t;

#define SHARED_INVALID_ID 65535
#define MOTION_DEFAULT POSLIB_MOTION_STATIC

#define TIMEOUT_APPCFG_MS 30000
#define TIMEOUT_SCAN_MS 60000
#define SCAN_MARGIN_RATIO 2 // +(scan_time/SCAN_MARGIN_RATIO) added to mini-beacon period

#define MS_TIME_FROM(x) (lib_time->getTimeDiffUs(lib_time->getTimestampHp(), x) / 1000)

/** Can be selected using PosLib_motion */
static poslib_motion_mode_e m_motion_mode;
/** Poslib general settings received from application or from app_config */
static poslib_settings_t m_pos_settings;
static poslib_aux_settings_t m_aux_settings;

/** Shared offline registration state */
static callback_state_t m_shared_offline = {.reg = false, .id = SHARED_INVALID_ID};

/** Shared AppCfg registration state */
static callback_state_t m_shared_appcfg = {.reg = false, .id = SHARED_INVALID_ID};
/** Legacy AppCfg registration state */
static callback_state_t m_legacy_appcfg = {.reg = false, .id = SHARED_INVALID_ID};


/** Buffer for appconfig */
static uint8_t m_appcfg[APP_LIB_DATA_MAX_APP_CONFIG_NUM_BYTES]; 
static uint8_t m_appcfg_len = 0;

/** Control state variables */
static bool m_poslib_started = false;
static bool m_poslib_configured = false;
static bool m_poslib_init = false;
static control_state_t m_ctrl;

/** Events private data */
//ToDo: generalize event private data later 
static bool m_data_sent_success = false;
static poslib_internal_event_type_e m_timeout_event_type = POSLIB_CTRL_EVENT_NONE;

/** Local function definition */
static void schedule_next(bool update_now);
static void handle_update_state(poslib_internal_event_t * event);
static void stop_tasks(void);
static uint32_t get_update_period();

#define TIME_SEC_TO_MS(tm)  ((tm) * 1000U)
#define TIME_MS_TO_SEC(tm) ((tm) / 1000U)
/** For app_config receiving TLV type for positioning control
 *  Reserved from Wirepas reserved range. Do not use in any other features
 */
#define POSLIB_TLV_TYPE            0xC1

/**
 * @brief   Shared offline library callback when entering to offline..
 * @param   delay_s
 *          Delay the node will stay offline if none of the registered module
 *          ask to enter online state asynchronously
 */
static void on_offline_cb(uint32_t delay_s)
{
    LOG(LVL_INFO, "PosLib on_offline for %d", delay_s);
    PosLibEvent_add(POSLIB_CTRL_EVENT_OFFLINE);
}

/**
 * @brief   Shared offline library callback when entering to online.
 *          Saves event time for new offline sleep time calculation..
 * @param   delay_from_deadline_s
 *          Delay in second per module relative to the initially requested time.
 */
static void on_online_cb(uint32_t delay_from_deadline_s)
{
    LOG(LVL_DEBUG, "Online"); 
    PosLibEvent_add(POSLIB_CTRL_EVENT_ONLINE);
}

/**
 * @brief   Shared offline callbacks.
 */

static offline_setting_conf_t m_shared_offline_cbs =
{
    .on_offline_event = on_offline_cb,
    .on_online_event = on_online_cb
};

static void data_send_cb(const app_lib_data_sent_status_t * status)
{
    m_data_sent_success = status->success;
    PosLibEvent_add(POSLIB_CTRL_EVENT_DATA_SENT);
}

#ifdef ROUTE_CHECK
static void route_cb(app_lib_stack_event_e event, void * param)
{
    PosLibEvent_add(POSLIB_CTRL_EVENT_ROUTE_CHANGE);
}
#endif

static uint32_t led_off_cb(void)
{
    PosLibEvent_add(POSLIB_CTRL_EVENT_LED_OFF);
    m_aux_settings.led_on_duration_s = 0;
    return APP_SCHEDULER_STOP_TASK;
}

static void process_led_status(poslib_aux_settings_t * aux)
{
    if (aux->led_on_duration_s == 0)
    {
        LOG(LVL_DEBUG,"LED off");
        PosLibEvent_add(POSLIB_CTRL_EVENT_LED_OFF);
        App_Scheduler_cancelTask(led_off_cb);
    }
    else
    {
        PosLibEvent_add(POSLIB_CTRL_EVENT_LED_ON);
        LOG(LVL_DEBUG,"LED on. Duration (s): %d", aux->led_on_duration_s);
        App_Scheduler_addTask_execTime(led_off_cb, TIME_SEC_TO_MS(aux->led_on_duration_s), 500);
    }
}

static void process_appcfg()
{ 
    poslib_settings_t settings = m_pos_settings;
    poslib_aux_settings_t aux = m_aux_settings;

    LOG(LVL_INFO,"AppCfg received");

    if(PosLibDecode_config((const uint8_t *) &m_appcfg, m_appcfg_len, &settings, &aux))
    {
        if(memcmp(&m_pos_settings, &settings, sizeof(poslib_settings_t)) != 0)
        {
            m_pos_settings = settings;
            PosLibEvent_add(POSLIB_CTRL_EVENT_CONFIG_CHANGE);
        }
        if (aux.led_cmd_seq != m_aux_settings.led_cmd_seq)
        {
            LOG(LVL_DEBUG, "LED on duration in second, LED command sequence: %d, %x", aux.led_on_duration_s, aux.led_cmd_seq);
            m_aux_settings = aux;
            process_led_status(&m_aux_settings);
        }
        else
        {
            LOG(LVL_DEBUG, "LED command sequence not changed.")
        }
    }
    else
    {
        LOG(LVL_ERROR,"No valid appcfg");
    }
}

static void appconfig_cb(uint16_t types, uint8_t length, uint8_t * bytes)
{
    if (types == POSLIB_TLV_TYPE)
    {
        memcpy(&m_appcfg[0], bytes, length);
        m_appcfg_len = length;
        PosLibEvent_add(POSLIB_CTRL_EVENT_APPCFG); 
    }
    else
    {
        LOG(LVL_ERROR, "AppCfg CB called with type: %u", types);
    }
}

static void appconfig_legacy_cb(uint16_t types, uint8_t length, uint8_t * bytes)
{
    if (types == SHARED_APP_CONFIG_INCOMPATIBLE_FILTER)
    {
        /*Legacy AppCfg is not supported. We just need to inform that app cfg was received*/
        m_appcfg_len = 0;   
        PosLibEvent_add(POSLIB_CTRL_EVENT_APPCFG); 
    }
    else
    {
        LOG(LVL_ERROR, "Legacy AppCfg CB called with type: %u", types);
    }
}

static bool register_callbacks(void)
{
    bool ret = true;
     /** AppCfg callback */
    if(!m_shared_appcfg.reg)
    {
        shared_app_config_res_e res;
        shared_app_config_filter_t app_cfg_filter = {
            .type = POSLIB_TLV_TYPE,
            .cb = (shared_app_config_received_cb_f) appconfig_cb,
            .call_cb_always = true
        };

        res = Shared_Appconfig_addFilter(&app_cfg_filter, &m_shared_appcfg.id);
        if(res == SHARED_APP_CONFIG_RES_OK)
        {
            m_shared_appcfg.reg = true;
            LOG(LVL_DEBUG, "AppCfg callback registered");
        }
        else
        {
            LOG(LVL_ERROR, "Cannot register AppCfg callback. res: %u", res);
            m_shared_appcfg.reg = false;
            ret = false;
        }
    }

     /** AppCfg legacy callback */
    if(!m_legacy_appcfg.reg)
    {
        shared_app_config_res_e res;
        shared_app_config_filter_t app_cfg_filter = {
        .type = SHARED_APP_CONFIG_INCOMPATIBLE_FILTER,
        .cb = (shared_app_config_received_cb_f) appconfig_legacy_cb,
        .call_cb_always = false
        };

        res = Shared_Appconfig_addFilter(&app_cfg_filter, &m_legacy_appcfg.id);
        if(res == SHARED_APP_CONFIG_RES_OK)
        {
            m_legacy_appcfg.reg = true;
            LOG(LVL_DEBUG, "Legacy AppCfg callback registered");
        }
        else
        {
            LOG(LVL_ERROR, "Cannot register legacy AppCfg callback. res: %u", res);
            m_legacy_appcfg.reg = false;
            ret = false;
        }
    }

    #ifdef ROUTE_CHECK
    /** Route change callback */
    {
        app_res_e res = Stack_State_addEventCb(route_cb,
                                               1 << APP_LIB_STATE_STACK_EVENT_ROUTE_CHANGED);
        if (res != APP_RES_OK)
        {
            ret = false;
            LOG(LVL_ERROR, "Cannot register route change callback. res. %u", res)
        }
    }
    #endif

    return ret;
}

static void deregister_callbacks(void)
{
    /** AppCfg callback */
    if(m_shared_appcfg.reg)
    {
        shared_app_config_res_e res;
        res = Shared_Appconfig_removeFilter(m_shared_appcfg.id);
        m_shared_appcfg.reg = false;
        if (res != SHARED_APP_CONFIG_RES_OK)
        {
            LOG(LVL_ERROR, "Cannot deregister AppCfg callback. res: %u", res);
        }
    }

     /** Legacy AppCfg callback */
    if(m_legacy_appcfg.reg)
    {
        shared_app_config_res_e res;
        res = Shared_Appconfig_removeFilter(m_legacy_appcfg.id);
        m_legacy_appcfg.reg = false;
        if (res != SHARED_APP_CONFIG_RES_OK)
        {
            LOG(LVL_ERROR, "Cannot deregister legacy AppCfg callback. res: %u", res);
        }
    }

    #ifdef ROUTE_CHECK
    /** Route change callback */
    Stack_State_removeEventCb(route_cb);
    #endif
}


/**
 * @brief       Check that given parameters are valid based on selected mode.
 * @param       settings
 * @return      POS_RET_INVALID_PARAM when parameters check fails,
 *              POS_RET_OK when parameters check success.
 */
static poslib_ret_e check_params(poslib_settings_t * settings)
{
    if (settings->update_period_static_s < POSLIB_MIN_MEAS_RATE_S ||
        settings->update_period_static_s > POSLIB_MAX_MEAS_RATE_S)
    {
        LOG(LVL_ERROR, "Outside range static period: %d", settings->update_period_static_s);
        return POS_RET_INVALID_PARAM;
    }

    if (settings->update_period_dynamic_s > POSLIB_MAX_MEAS_RATE_S || 
        (settings->update_period_dynamic_s < POSLIB_MIN_MEAS_RATE_S && 
         settings->update_period_dynamic_s != 0))
    {
        LOG(LVL_ERROR, "Outside range dynamic period: %d", settings->update_period_static_s);
        return POS_RET_INVALID_PARAM;
    }

    if (settings->update_period_offline_s > POSLIB_MAX_MEAS_RATE_S || 
        (settings->update_period_offline_s < POSLIB_MIN_MEAS_RATE_S && 
         settings->update_period_offline_s != 0))
    {
        LOG(LVL_ERROR, "Outside range offline period: %d", settings->update_period_static_s);
        return POS_RET_INVALID_PARAM;
    }

    return POS_RET_OK;
}

static poslib_ret_e check_role(poslib_settings_t * settings)
/**
 * @brief       Check that given parameters are valid based on selected role.
 * @param       settings
 * @return      POS_RET_INVALID_PARAM when parameters check fails,
 *              POS_RET_OK when parameters check success.
 */
{
    app_lib_settings_role_t role;
    uint8_t poslib_mode = settings->node_mode;
    lib_settings->getNodeRole(&role);

    switch (poslib_mode)
    {
        case POSLIB_MODE_AUTOSCAN_ANCHOR:
        case POSLIB_MODE_OPPORTUNISTIC_ANCHOR:
        {
            bool valid = (role == APP_LIB_SETTINGS_ROLE_HEADNODE_LL) ||
                         (role == APP_LIB_SETTINGS_ROLE_HEADNODE_LE) ||
                         (role == APP_LIB_SETTINGS_ROLE_SINK_LL) ||
                         (role == APP_LIB_SETTINGS_ROLE_SINK_LE) ||
                         (role == APP_LIB_SETTINGS_ROLE_SUBNODE_LE && settings->mbcn.enabled) ||
                         (role == APP_LIB_SETTINGS_ROLE_SUBNODE_LL && settings->mbcn.enabled) ;

            if (!valid)
            {
                LOG(LVL_ERROR, "Anchor mode: %u but role: %u", poslib_mode, role);
                return POS_RET_INVALID_PARAM; 
            }
            break;
        }
        case POSLIB_MODE_NRLS_TAG:
        case POSLIB_MODE_AUTOSCAN_TAG:
        {
            if (role != APP_LIB_SETTINGS_ROLE_SUBNODE_LE &&
                role != APP_LIB_SETTINGS_ROLE_SUBNODE_LL)
            {
                LOG(LVL_ERROR, "Tag mode: %u but role: %u", poslib_mode, role);
                return POS_RET_INVALID_PARAM;
            }
            break;
        }
        case POSLIB_MODE_DA_TAG:
        {
            if (role != APP_LIB_SETTINGS_ROLE_ADVERTISER)
            {
                LOG(LVL_ERROR, "DA Tag mode: %u but role: %u", poslib_mode, role);
                return POS_RET_INVALID_PARAM;
            }
            break;
        }
        default:
         {
            LOG(LVL_ERROR, "Unknown node mode: %u, role: %u", poslib_mode, role);
            return POS_RET_INVALID_PARAM;
            break;
         }

    }
    return POS_RET_OK;
}

/**
 * @brief       Check that beacon given parameters are valid if feature is enabled.
 * @param       settings
 * @return      POS_RET_INVALID_PARAM when parameters check fails,
 *              POS_RET_OK when parameters check success.
 */
static poslib_ret_e check_ble_params(poslib_settings_t * settings)
{
    poslib_ble_type_e ble_type = settings->ble.type;

    if (ble_type != POSLIB_BEACON_ALL && ble_type != POSLIB_IBEACON &&
        ble_type != POSLIB_EDDYSTONE)
    {
        LOG(LVL_ERROR, "Incorrect BLE type: %d", ble_type);
        return POS_RET_INVALID_PARAM;
    }

    /** Check for valid beacon configuration if feature is enabled */
    /** ble tx max power is not checked as radio specific */
    if (ble_type == POSLIB_EDDYSTONE || ble_type == POSLIB_BEACON_ALL)
    {
        if (settings->ble.eddystone.tx_interval_ms >
            APP_LIB_BEACON_TX_MAX_INTERVAL ||
            settings->ble.eddystone.tx_interval_ms < 
            APP_LIB_BEACON_TX_MIN_INTERVAL ||
            settings->ble.eddystone.channels >
                APP_LIB_BEACON_TX_CHANNELS_ALL)
        {
            LOG(LVL_ERROR, "FAIL eddystone settings: %d", ble_type);
            return POS_RET_INVALID_PARAM;
        }
    }

    if (ble_type == POSLIB_IBEACON || ble_type == POSLIB_BEACON_ALL)
    {
        if (settings-> ble.ibeacon.tx_interval_ms > 
            APP_LIB_BEACON_TX_MAX_INTERVAL ||
            settings-> ble.ibeacon.tx_interval_ms < 
            APP_LIB_BEACON_TX_MIN_INTERVAL ||
            settings->ble.ibeacon.channels >
                APP_LIB_BEACON_TX_CHANNELS_ALL)
         {
            LOG(LVL_ERROR, "FAIL ibeacon settings: %d", ble_type);
            return POS_RET_INVALID_PARAM;
         }
    }

    return POS_RET_OK;
}

/**
 * @brief       Checking set PosLib settings range.
 * @return      POS_RET_INVALID_PARAM when parameters check fails,
 *              POS_RET_OK when parameters check success.
 */
static poslib_ret_e check_ctrl_params(poslib_settings_t * settings)
{
    /** Check for given parameters for selected role are valid */
    if(check_role(settings) == POS_RET_INVALID_PARAM)
    {
        return POS_RET_INVALID_PARAM;
    }

    /** Check for valid beacon configuration if feature is enabled */
    if(check_ble_params(settings) == POS_RET_INVALID_PARAM)
    {
        return POS_RET_INVALID_PARAM;
    }

    /** Check for given parameters for selected mode are valid */
    if(check_params(settings) == POS_RET_INVALID_PARAM)
    {
        return POS_RET_INVALID_PARAM;
    }

    return POS_RET_OK;
}

/**
 * @brief       Register shared_offline library if needed.
 * @return      true if already registered or new registration.
 *              false if needed registration fails.
 */
static bool shared_offline_register()
{
    shared_offline_res_e res;

    if (!m_shared_offline.reg)
    {
        
        uint8_t id; //FixME: all Shared libraries should use uint8_t, remove when change made
        res = Shared_Offline_register(&id, m_shared_offline_cbs);
        m_shared_offline.id = id;
        
        if (res == SHARED_OFFLINE_RES_OK)
        {
            m_shared_offline.reg = true;
            LOG(LVL_DEBUG, "Shared offline registered");
        }
        else
        {
            LOG(LVL_ERROR, "Cannot register shared offline");
            m_shared_offline.reg = false;
        }
    }

    return m_shared_offline.reg;
}

/**
 * @brief       Unregister shared_offline library if needed.
 * @return      true if already unregistered or success.
 *              false if needed unregistration fails.
 */
static bool shared_offline_unregister()
{
    shared_offline_res_e res;

    if (m_shared_offline.reg)
    {
        res = Shared_Offline_unregister(m_shared_offline.id);

        if (res == SHARED_OFFLINE_RES_OK)
        {
            m_shared_offline.reg = false;
            LOG(LVL_DEBUG, "Shared offline unregistered ");
        }
        else
        {
            LOG(LVL_ERROR, "Cannot unregister shared offline "); //FixMe: 
        }
    }

    return m_shared_offline.reg;
}
#ifdef ROUTE_CHECK
static app_lib_state_route_state_e get_route_state(void)
{
    app_lib_state_route_info_t info;
    lib_state->getRouteInfo(&info);
    LOG(LVL_DEBUG, "Route change - state %u sink %u next_hop %u ch %u cost %u",
                info.state, info.sink, info.next_hop, info.channel, info.cost);
    return info.state;
}
#endif

static void control_init(void)
{    
    if (!m_poslib_init)
    {
        //m_ctrl init
        memset(&m_ctrl, 0 , sizeof(m_ctrl));
        m_ctrl.last_update_s = lib_time->getTimestampS()-1;
        m_ctrl.next_update_s =  m_ctrl.last_update_s;
        m_ctrl.state = POSLIB_STATE_STOPPED;
        m_motion_mode = MOTION_DEFAULT;
        m_poslib_init = true;
        memset(&m_aux_settings, 0, sizeof(m_aux_settings));
    }
}

poslib_ret_e PosLibCtrl_setConfig(poslib_settings_t * settings)
{
    poslib_ret_e ret = check_ctrl_params(settings);
    bool config_change = false;

    if (ret != POS_RET_OK)
    {
        return ret;
    }

    if (!m_poslib_configured || 
        memcmp(&m_pos_settings , settings, sizeof(m_pos_settings)) != 0)
    {
        memcpy(&m_pos_settings , settings, sizeof(m_pos_settings));
        config_change = true;
    }
   
    m_poslib_configured = true;

    /* If started notify configuration change */
    if(m_poslib_started && config_change)
    {
        PosLibEvent_add(POSLIB_CTRL_EVENT_CONFIG_CHANGE);
    }

    return POS_RET_OK;
}

static uint32_t delayed_start()
{
    schedule_next(false);
    return APP_SCHEDULER_STOP_TASK;
}

poslib_ret_e PosLibCtrl_startPeriodic(void)
{
    if (!m_poslib_configured)
    {
        LOG(LVL_ERROR, "Start periodic requested but PosLib not configured");
        return POS_RET_NOT_CONFIGURED;
    }

    if (m_poslib_started)
    {
        LOG(LVL_ERROR, "Periodic update already started");
        return POS_RET_INVALID_STATE;
    }

    control_init();

    if (!register_callbacks())
    {
        LOG(LVL_ERROR, "Cannot register all required callbacks");
        return POS_RET_LIB_ERROR;
    }

    m_ctrl.state = POSLIB_STATE_IDLE;
    m_poslib_started = true;
    App_Scheduler_addTask_execTime(delayed_start, 5000, 500);  //FixME: this is needed to prevent a race condition in NRLS
    
    PosLibBle_start(&m_pos_settings.ble);

    if (m_pos_settings.node_mode == POSLIB_MODE_AUTOSCAN_ANCHOR ||
        m_pos_settings.node_mode == POSLIB_MODE_OPPORTUNISTIC_ANCHOR)
    {
        PosLibMbcn_start(&m_pos_settings.mbcn);
    }

    PosLibDa_start(&m_pos_settings);

    LOG(LVL_INFO, "Started periodic update");
    return POS_RET_OK;
 }

poslib_ret_e PosLibCtrl_startOneshot(void)
{
    if (!m_poslib_configured)
    {
        LOG(LVL_ERROR, "Start oneshot requested but PosLib not configured");
        return POS_RET_NOT_CONFIGURED;
    }    

    control_init();
    
    if (!register_callbacks())
    {
        LOG(LVL_ERROR, "Cannot register all required callbacks");
        return POS_RET_LIB_ERROR;
    }

    PosLibEvent_add(POSLIB_CTRL_EVENT_ONESHOT);
    LOG(LVL_INFO, "Started oneshot update");
    return POS_RET_OK;
}

poslib_ret_e PosLibCtrl_stopPeriodic(void)
{
    m_poslib_started = false;
    m_ctrl.state = POSLIB_STATE_STOPPED;
    stop_tasks();
    deregister_callbacks();
    shared_offline_unregister();
    PosLibMeas_stop();
    PosLibBle_stop();
    PosLibMbcn_stop();
    PosLibDa_stop();
    return POS_RET_OK;
}

poslib_mode_e PosLibCtrl_getMode(void)
{
    return m_pos_settings.node_mode;
}

poslib_ret_e PosLibCtrl_getConfig(poslib_settings_t * settings)
{
    *settings = m_pos_settings;
    return POS_RET_OK;
}

poslib_ret_e PosLibCtrl_motion(poslib_motion_mode_e mode)
{
    //event generated on change
    if (!m_pos_settings.motion.enabled)
    {
        return POS_RET_MOTION_NOT_ENABLED;
    }
    
    if(m_motion_mode != mode)
    {
        m_motion_mode = mode;
        PosLibEvent_add(POSLIB_CTRL_EVENT_MOTION);
    }
    
    return POS_RET_OK;
}

poslib_status_e PosLibCtrl_status(void)
{
   switch (m_ctrl.state)
   {
       case POSLIB_STATE_UPDATE:
       {
            return POSLIB_UPDATE_START;
       }
        case POSLIB_STATE_IDLE:
        {
            return POSLIB_IDLE;
        }
        case POSLIB_STATE_STOPPED:
        {
            return POSLIB_STOPPED;
        }
        default:
        {
            LOG(LVL_ERROR, "State %u not handled in status reply", m_ctrl.state);
            return POSLIB_STOPPED;
        }
   }
}

poslib_measurements_e get_meas_type()
{
    poslib_measurements_e meas_type = DEFAULT_MEASUREMENT_TYPE_TAG;

    switch(m_pos_settings.node_mode)
    {
        case POSLIB_MODE_NRLS_TAG:
        case POSLIB_MODE_AUTOSCAN_TAG:
        case POSLIB_MODE_DA_TAG:
        {
            meas_type = DEFAULT_MEASUREMENT_TYPE_TAG;
            break;
        }
        case POSLIB_MODE_AUTOSCAN_ANCHOR:
        case POSLIB_MODE_OPPORTUNISTIC_ANCHOR:
        { 
            meas_type = DEFAULT_MEASUREMENT_TYPE_ANCHOR;
            break;
        }
        default:
        {
            LOG(LVL_WARNING, "Node mode %u unknown. Default to tag meas. type", 
                m_pos_settings.node_mode);
            meas_type = DEFAULT_MEASUREMENT_TYPE_TAG;
            break;
        }
    }

    return meas_type;
}

static void get_node_info(poslib_meas_record_node_info_t * node_info)
{
    uint32_t features = node_info->features;
    node_info->node_class = m_pos_settings.node_class;
    node_info->node_mode = m_pos_settings.node_mode;
    node_info->update_s = get_update_period();

    features = POSLIB_NODE_INFO_FEATURES_VERSION & POSLIB_NODE_INFO_MASK_VERSION;  
    if (m_pos_settings.motion.enabled)
    {
        features |= POSLIB_NODE_INFO_FLAG_MOTION_EN;
    }
    if (m_ctrl.events.is_static)
    {
        features |= POSLIB_NODE_INFO_FLAG_IS_STATIC;
    }
    if (PosLibBle_getEddystoneStatus())
    {
        features |= POSLIB_NODE_INFO_FLAG_EDDYSTONE_ON;
    }
    if (PosLibBle_getiBeaconStatus())
    {
        features |= POSLIB_NODE_INFO_FLAG_IBEACON_ON;
    }
    if(m_pos_settings.da.routing_enabled)
    {
        features |= POSLIB_NODE_INFO_FLAG_MBCN_ON; 
    }

    node_info->features = features;

    LOG(LVL_DEBUG, "Node info feature: %x en: %u", node_info->features, m_pos_settings.motion.enabled); 
}

static uint8_t send_measurement_message()
{   
    #define MAX_PAYLOAD_LEN 102
    size_t available_buffers;

    uint8_t bytes[MAX_PAYLOAD_LEN];
    uint8_t num_bytes = 0;
    app_lib_data_to_send_t payload;
    app_lib_data_send_res_e rc;
    uint8_t num_meas = 0;
    bool add_voltage = false;
    poslib_meas_record_node_info_t node_info;
    app_lib_settings_role_t role;

    m_data_sent_success = false;

    #ifdef CONF_VOLTAGE_REPORT
    add_voltage = true;
    #endif

    lib_data->getNumFreeBuffers(&available_buffers);
    if (available_buffers == 0)
    {
        LOG(LVL_ERROR, "Cannot send data. No buffers left");
        return 0;
    }

    // Fill in node info
    get_node_info(&node_info);

    if (PosLibMeas_getPayload(bytes, sizeof(bytes), m_ctrl.sequence,
                                get_meas_type(), add_voltage, &node_info, 
                                &num_bytes, &num_meas))
    {
        /** add payload to be sent */
        payload.bytes = bytes;
        payload.num_bytes = num_bytes;
        payload.dest_address = APP_ADDR_ANYSINK;
        payload.src_endpoint = POS_SOURCE_ENDPOINT;;
        payload.dest_endpoint = POS_DESTINATION_ENDPOINT;
        payload.qos = APP_LIB_DATA_QOS_NORMAL;
        payload.delay = 0;
        payload.flags = APP_LIB_DATA_SEND_FLAG_TRACK;
        payload.tracking_id = 0;

        lib_settings->getNodeRole(&role);
        rc = (role == APP_LIB_SETTINGS_ROLE_ADVERTISER) ? 
                    PosLibDa_sendData(&payload, data_send_cb) :
                    Shared_Data_sendData(&payload, data_send_cb);
 
        if (rc == APP_LIB_DATA_SEND_RES_SUCCESS)
        {
           m_ctrl.events.data_sent = true;
        }
        else
        {
            m_ctrl.events.data_sent = false;
            LOG(LVL_ERROR, "Failed to send data. Error %u", rc);
        }
    }

    return num_meas;
}


static uint32_t get_scan_duration()
{
    uint32_t scan_duration_ms = 0;
    
    if (m_pos_settings.mbcn.enabled && 
        m_pos_settings.mbcn.tx_interval_ms < 1000)
    {
        scan_duration_ms = m_pos_settings.mbcn.tx_interval_ms;
        scan_duration_ms +=  scan_duration_ms / SCAN_MARGIN_RATIO;
    }
    return scan_duration_ms;
}

static uint32_t trigger_update_task()
{
    uint32_t elapsed_s;
    uint32_t remaining_s;
    shared_offline_res_e res;

    shared_offline_register(); //FixMe: handle registration

    //Stack offline - wakeup stack
    if (Shared_Offline_get_status(&elapsed_s, &remaining_s) == 
        SHARED_OFFLINE_STATUS_OFFLINE)
    {
        res = Shared_Offline_enter_online_state(m_shared_offline.id);
        if ( res == SHARED_OFFLINE_RES_OK)
        {
            LOG(LVL_DEBUG, "Update triggered: stack wakeup");
            return APP_SCHEDULER_SCHEDULE_ASAP; 
        } 
        else if (res == SHARED_OFFLINE_RES_ALREADY_ONLINE)
        {
            LOG(LVL_WARNING, "Update triggered: stack is online");
        }
        else
        {
            LOG(LVL_ERROR, "Cannot trigger update. Wakeup stack response: %u", res);
            //FixMe: trigger error event
            return APP_SCHEDULER_STOP_TASK;
        }
    }

    //Stack online - trigger scan
    poslib_scan_ctrl_t scan_ctrl = {
        .mode = SCAN_MODE_STANDARD,
        .max_duration_ms = get_scan_duration()};
    PosLibMeas_startScan(&scan_ctrl);

    LOG(LVL_DEBUG, "Update triggered at %u", lib_time->getTimestampS());
    return APP_SCHEDULER_STOP_TASK;
}

static uint32_t timeout_task()
{
    static poslib_internal_event_t timeout_event = {
        .list = {NULL},
        .type = POSLIB_CTRL_EVENT_TIMEOUT,
    };

    //as it a local event will be handled directly
    PosLibCtrl_processEvent(&timeout_event);
    return APP_SCHEDULER_STOP_TASK;
}

static void set_timeout(poslib_internal_event_type_e type, uint32_t timeout_ms)
{
    m_timeout_event_type = type;
    App_Scheduler_addTask_execTime(timeout_task, timeout_ms, 500);
}

static void clear_timeout(void)
{
    App_Scheduler_cancelTask(timeout_task); 
}

static void stop_tasks(void)
{
    App_Scheduler_cancelTask(trigger_update_task);
    App_Scheduler_cancelTask(timeout_task);
}

static bool update_autoscan(uint32_t delta_s)
{
    uint32_t next_update_ms;

    next_update_ms = (delta_s == 0) ? APP_SCHEDULER_SCHEDULE_ASAP : TIME_SEC_TO_MS(delta_s);

    if (App_Scheduler_addTask_execTime(trigger_update_task, next_update_ms, 500) != APP_SCHEDULER_RES_OK)
    {
        LOG(LVL_ERROR, "Cannot add trigger_update task");
        return false;
    }
    
    LOG(LVL_INFO, "Update scheduled in: %u sec", delta_s);
    return true;
}

static bool update_nrls(uint32_t delta_s)
{
    shared_offline_res_e res;

    if (!shared_offline_register())
    {
        LOG(LVL_ERROR, "Cannot register to shared offline lib");
        return false;
    }

    res = Shared_Offline_enter_offline_state(m_shared_offline.id, delta_s);

    if (res != SHARED_OFFLINE_RES_OK)
    {
        LOG(LVL_ERROR, "Failed to enter offline. res: %u:", res);
        return false;
    }
    
    LOG(LVL_INFO, "Enter offline for %u sec stack %u", delta_s, lib_state->getStackState());
    return true;
}

static uint32_t get_update_period()
{
    
    if (m_ctrl.outside_wm && 
        m_pos_settings.update_period_offline_s != 0)
    {
        return  m_pos_settings.update_period_offline_s;
    }

    if (m_pos_settings.motion.enabled &&
        m_motion_mode == POSLIB_MOTION_DYNAMIC &&
        m_pos_settings.update_period_dynamic_s != 0)
    {
        return m_pos_settings.update_period_dynamic_s;
    }
    
    return m_pos_settings.update_period_static_s;
}

static void schedule_next(bool update_now)
{
    uint32_t now_s = lib_time->getTimestampS();
    uint32_t next_update_s = 0;
    uint32_t delta_s = 0; 
    uint32_t update_period_s = get_update_period();
    bool scheduled = m_ctrl.scheduled;

    if (m_ctrl.state == POSLIB_STATE_STOPPED && !update_now)
    {
        m_ctrl.state = POSLIB_STATE_STOPPED;
        m_ctrl.scheduled = false;
        return;
    }

    /* Determine the next update time */
    if (update_now)  // schedule imediatelly
    {
        next_update_s = now_s;   
    }
    else 
    {
        delta_s = (now_s - m_ctrl.last_update_s) % update_period_s;
        next_update_s = now_s + update_period_s - delta_s;

        /* Keep the previous update time if already scheduled and is earlier */
        if (scheduled && (next_update_s > m_ctrl.next_update_s)) 
        {
            return;
        }
    }

    delta_s = next_update_s - now_s; 

    if (scheduled)  
    {
        /* re-schedule if new update time; note that if in NRLS mode a 
        wakeup will be triggered */
        if (next_update_s != m_ctrl.next_update_s)
        {
            m_ctrl.next_update_s = next_update_s;
            update_autoscan(delta_s);
        }
    }
    else // new schedule
    {
        stop_tasks();
        m_ctrl.sequence++;
        m_ctrl.state = POSLIB_STATE_IDLE;
        m_ctrl.mode = m_pos_settings.node_mode;
        m_ctrl.scheduled = true;

        memset(&m_ctrl.events, 0, sizeof(control_events_t));
        m_ctrl.next_update_s = next_update_s;
        m_ctrl.events.is_static = (m_motion_mode == POSLIB_MOTION_STATIC) 
                                    && m_pos_settings.motion.enabled;

        switch (m_ctrl.mode)
        {
            case POSLIB_MODE_NRLS_TAG:
            {
                PosLibMeas_opportunisticScan(true);
                update_nrls(delta_s); //FixMe: handle return
                break;
            }

            case POSLIB_MODE_AUTOSCAN_TAG:
            case POSLIB_MODE_AUTOSCAN_ANCHOR:
            case POSLIB_MODE_DA_TAG:
            {
                LOG(LVL_INFO, "<state> Update ended - autoscan. seq: %u, time: %u",
                    m_ctrl.sequence, MS_TIME_FROM(m_ctrl.update_start_hp));
                PosLibMeas_opportunisticScan(false);
                update_autoscan(delta_s); //FixMe: handle return
                break; 
            }

            case POSLIB_MODE_OPPORTUNISTIC_ANCHOR:
            {
                PosLibMeas_opportunisticScan(true);
                m_ctrl.next_update_s = 0;
                break;
            }
            default:
            {
                break; //FixMe: handle not known mode
            }
        }
    }

    LOG(LVL_DEBUG,"<state> Schedule: last: %u next: %u now: %u delta: %u update: %u", 
            m_ctrl.last_update_s,
            m_ctrl.next_update_s,
            now_s,
            delta_s,
            update_period_s);
}

static void handle_stopped_state(poslib_internal_event_t * event)
{
    if (!m_poslib_started && m_poslib_configured && event->type == POSLIB_CTRL_EVENT_ONESHOT)
    {        
        m_ctrl.events.oneshot = true;
        schedule_next(true);
    }
}

static void handle_idle_state(poslib_internal_event_t * event)
{
    switch (event->type)
    {
        case POSLIB_CTRL_EVENT_SCAN_STARTED:
        {
            LOG(LVL_DEBUG, "Update started - scan");
            m_ctrl.update_start_hp = lib_time->getTimestampHp();  
            m_ctrl.state = POSLIB_STATE_UPDATE;
            m_ctrl.events.scan_start = true;
            set_timeout(POSLIB_CTRL_EVENT_SCAN_END, TIMEOUT_SCAN_MS);
            break;
        }
        
        case POSLIB_CTRL_EVENT_SCAN_END: 
        {
            if (m_ctrl.scheduled) //for opportunistic scan
            {
                m_ctrl.state = POSLIB_STATE_UPDATE;
                m_ctrl.events.scan_end = true;
                //event reprocess not supported; call imediatelly
                handle_update_state(event);
            }
            else
            {
                LOG(LVL_ERROR, "Scan end received but no update scheduled");
                //FixMe: handle error   
            }
            break;
        }

        case POSLIB_CTRL_EVENT_ONLINE:
        {
            LOG(LVL_DEBUG, "<state> Update started - online");
            m_ctrl.update_start_hp = lib_time->getTimestampHp();  
            m_ctrl.state = POSLIB_STATE_UPDATE;
            m_ctrl.events.online = true;
            set_timeout(POSLIB_CTRL_EVENT_SCAN_END, TIMEOUT_SCAN_MS);
            break;
        }
        case POSLIB_CTRL_EVENT_OFFLINE:
        {
            m_ctrl.events.offline = true;
            if (m_ctrl.scheduled)
            {
                m_ctrl.events.app_cfg = false;
                PosLibMeas_clearMeas();
            }
            LOG(LVL_DEBUG, "<state> Update ended - online. seq: %u, time: %u",
                                m_ctrl.sequence, 
                                MS_TIME_FROM(m_ctrl.update_start_hp));
            break;
        }

        case POSLIB_CTRL_EVENT_CONFIG_CHANGE:
        {
            /*Set motion to default if disabled*/
            if (!m_pos_settings.motion.enabled)
            {
                m_motion_mode = MOTION_DEFAULT;
            }
            PosLibBle_start(&m_pos_settings.ble);
            /* Configuration changed, re-schedule*/
            schedule_next(false);
            break;
        }

        case POSLIB_CTRL_EVENT_TIMEOUT:
        {
            LOG(LVL_ERROR, "Timeout occured in idle state: %u", lib_state->getStackState());
            //FixMe: add recovery
            break;   
        }
        case POSLIB_CTRL_EVENT_ONESHOT:
        {
            m_ctrl.events.oneshot = true;
            schedule_next(true);
            break;
        }

        case POSLIB_CTRL_EVENT_APPCFG:
        {
            process_appcfg();
            m_ctrl.events.app_cfg = true;
            break;
        }

        case POSLIB_CTRL_EVENT_MOTION:
        {
            LOG(LVL_DEBUG, "Motion event: %u", m_motion_mode);
            if (m_motion_mode == POSLIB_MOTION_DYNAMIC)
            {
                m_ctrl.events.is_static = false;
            }
            schedule_next(false);
            break;
        }

        //Ignored event
        default:
        {
            LOG(LVL_DEBUG, "Idle state - event %u ignored", event->type);
            break;
        }
    }
}

static bool is_update_completed()
{
    bool ret = m_ctrl.events.data_sent;

    /* Complete if: scan failed | timeout occured */ 
    if (m_ctrl.events.scan_fail || m_ctrl.events.timeout)
    {
        LOG(LVL_DEBUG, "<state> Update completed: scan fail: %u,timeout: %u",
                        m_ctrl.events.scan_fail, m_ctrl.events.timeout);
        return true;
    }

    if (m_ctrl.mode == POSLIB_MODE_NRLS_TAG)
    {
        /* In NRLS mode we wait for AppCfg and send complete */
        ret =  ret && m_ctrl.events.app_cfg && m_ctrl.events.data_sent_complete; 
    }
    return ret;
}

static void  update_outside_wm()
{
    uint8_t bcn = PosLibMeas_getBeaconNum();

    //Note that events are only generated on transition
    if (bcn == 0 && !m_ctrl.outside_wm)
    {
        PosLibEvent_add(POSLIB_CTRL_EVENT_OUTSIDE_WM);
        m_ctrl.outside_wm = true;
    }
    else if (bcn > 0 && m_ctrl.outside_wm)
    {
        PosLibEvent_add(POSLIB_CTRL_EVENT_UNDER_WM); 
        m_ctrl.outside_wm = false;
    }
}

static bool is_scan_valid(void)
{
    return (PosLibMeas_getBeaconNum() > 0);
}

static void handle_update_state(poslib_internal_event_t * event)
{
    switch (event->type)
    {
        case POSLIB_CTRL_EVENT_SCAN_END: 
        {
            m_ctrl.events.scan_end = true;
            clear_timeout();
            update_outside_wm();
            LOG(LVL_INFO,"<state> Scan end. time: %u bcn: %u", 
                    MS_TIME_FROM(m_ctrl.update_start_hp), PosLibMeas_getBeaconNum());

            if (is_scan_valid())
            {
                send_measurement_message();
                m_ctrl.events.scan_fail = false;
                set_timeout(POSLIB_CTRL_EVENT_APPCFG, TIMEOUT_APPCFG_MS);
            }
            else
            {
                m_ctrl.events.scan_fail = true;
                LOG(LVL_DEBUG, "<state> Scan detected 0 beacons");
            }
            break;
        }

        case POSLIB_CTRL_EVENT_APPCFG:
        {
            process_appcfg();
            m_ctrl.events.app_cfg = true;
            LOG(LVL_INFO,"<state> AppCfg received. time: %u", MS_TIME_FROM(m_ctrl.update_start_hp));
            break;
        }

        case POSLIB_CTRL_EVENT_DATA_SENT:
        {
            m_ctrl.events.data_sent_complete = true;
            m_ctrl.events.sent_success = m_data_sent_success;
            LOG(LVL_INFO,"<state> Measurement sent. success: %u time: %u ", 
                        m_ctrl.events.sent_success, MS_TIME_FROM(m_ctrl.update_start_hp));  
            break;
        }

        case POSLIB_CTRL_EVENT_MOTION:
        {
            LOG(LVL_DEBUG, "Motion event: %u", m_motion_mode);
            if (m_motion_mode == POSLIB_MOTION_DYNAMIC)
            {
                m_ctrl.events.is_static = false;
            }
            break;
        }
#ifdef ROUTE_CHECK
        case POSLIB_CTRL_EVENT_ROUTE_CHANGE:
        {

            app_lib_state_route_state_e route_state = get_route_state();
            (void)route_state;
            LOG(LVL_INFO,"<state> Route change. state: %u time: %u", m_ctrl.events.sent_success, 
                                                    MS_TIME_FROM(m_ctrl.update_start_hp));


            if (!m_ctrl.events.data_sent && (route_state == APP_LIB_STATE_ROUTE_STATE_VALID))
            {
                send_measurement_message();
                set_timeout(POSLIB_CTRL_EVENT_DATA_SENT, TIMEOUT_SEND_MS);
            }

            break;
        }
#endif

        case POSLIB_CTRL_EVENT_TIMEOUT:
        {
            LOG(LVL_ERROR, "<state> Timeout occured - event: %u state %u time_ %u", m_timeout_event_type, 
                            lib_state->getStackState(), MS_TIME_FROM(m_ctrl.update_start_hp));
            m_ctrl.events.timeout = true;
            break;   
        }

        default:
        {
            LOG(LVL_DEBUG, "Update state - event %u ignored", event->type);
            break;
        }
    }

    if (is_update_completed())
    {
        m_ctrl.scheduled = false;   
        m_ctrl.last_update_s = m_ctrl.next_update_s;
        PosLibEvent_add(POSLIB_CTRL_EVENT_UPDATE_END);
        schedule_next(false);
    }
}

 void PosLibCtrl_processEvent(poslib_internal_event_t * event)
 {
    
    if (m_ctrl.state == POSLIB_STATE_STOPPED)
    {
        handle_stopped_state(event);
    }
    else if (m_ctrl.state == POSLIB_STATE_IDLE)
    {
        handle_idle_state(event);
    }
    else if (m_ctrl.state == POSLIB_STATE_UPDATE)
    {
        handle_update_state(event);   
    }
    else
    {
        LOG(LVL_ERROR, "Unknown state: %u", m_ctrl.state)
    }
 }

 void PosLibCtrl_getAppConfig(uint8_t ** cfg, uint8_t * len)
 {
    *cfg = (uint8_t *) &m_appcfg;
    *len = m_appcfg_len;
 }

  void PosLibCtrl_setAppConfig(const uint8_t * cfg, uint8_t len)
 {
    
    if (len <= sizeof(m_appcfg))
    {
        memcpy(&m_appcfg, cfg, len);
        m_appcfg_len = len;
    }
    PosLibEvent_add(POSLIB_CTRL_EVENT_APPCFG); 
 }