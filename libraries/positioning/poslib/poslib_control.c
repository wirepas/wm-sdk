/**
 * @file       poslib_control.c
 * @brief      contains the routines with Poslib API and general control.
 * @copyright  Wirepas Ltd.2020
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
#include "bitfield.h"
#include "app_scheduler.h"
#include "random.h"
#include "poslib.h"
#include "posapp_settings.h"
#include "poslib_ble_beacon.h"
#include "poslib_control.h"
#include "poslib_event.h"
#include "poslib_measurement.h"
#include "poslib_decode.h"
#include "shared_shutdown.h"
#include "shared_data.h"
#include "shared_appconfig.h"
#include "shared_offline.h"

/** The state of the positioning state machine. */
static positioning_state_e m_state = POSLIB_STATE_STOPPED;
/** Counter for messages sent */
static uint16_t m_message_sequence_id;
/** Used to get into that appconfig is received */
static bool m_appconf_received = false;
/** bitmask for events callback requested */
static uint8_t  m_events_bitmask[1];
/** Update period */
static uint32_t m_update_period_ms;
/** Can be selected using PosLib_motion */
static poslib_motion_mode_e m_motion_mode;
/** Last time when scan is requested for update */
static uint32_t m_last_update_time_s;
/** Poslib general settings received from application or from app_config */
static poslib_settings_t m_pos_settings;
/** Filter id for shared appconfig lib */
static uint16_t m_appconfig_id;
/** Filter id for shared system lib  */
static uint16_t m_shutdown_id;
/** To save time when NRLS sleep is requested from PosLib */
static uint32_t m_nrls_event_time_s;
/** Oneshot update req from api */
static bool m_oneshot_update_requested;
/** Appconf wait time calculated from access cycle configuration in ms */
static uint16_t m_appconf_wait_ms;
/** Appconf wait time counter in exec_sleep task*/
static uint8_t m_nbr_appconf_waits = 0;
/** Registered id */
static uint8_t m_shared_offline_id;
/** Status of shared_offline registration */
static bool m_shared_offline_reg;
/** poslib mode saved over oneshot measurement */
static poslib_mode_e m_oneshot_prev_mode;
/** The state of the positioning state machine before oneshot. */
static positioning_state_e m_oneshot_prevstate;
/** data ack wait time in ms */
static uint16_t m_ack_wait_ms;
/** status true when in nrls mode data ack is waited */
static bool m_ack_wait_active;

#define TIME_SEC_TO_MS(tm)  ((tm) * 1000U)
#define TIME_MS_TO_SEC(tm) ((tm) / 1000U)
/** For app_config receiving TLV type for positioning control */
#define POSLIB_TLV_TYPE     0x99

/** Waitime to recheck if stack is started for beacon tx control */
#define POSLIB_STACK_WAIT_MS       TIME_SEC_TO_MS(1)

/** Maximum measurement rate */
#define POSLIB_MAX_MEAS_RATE_S     86400

/** Number of min ac time period which is waited for app-config in task
 * Wait time is POSLIB_NBR_OF_ACMIN_LOOPS x m_ack_wait_ms
 */
#define POSLIB_NBR_OF_ACMIN_LOOPS   14

/** Delay in ms when available beacon is checked for ble offline feature */
#define POSLIB_DELAY_BLEOFFLINE_START_MS    TIME_SEC_TO_MS(10)

/** Delay after oneshot scan end is forced to restore to previous state */
#define POSLIB_FORCED_ONESHOT_RESTORE_MS    TIME_SEC_TO_MS(5)


/**
 * @brief       Changes autoscan mode  scan interval to
 *              update_period_autoscan_offline_s if value defined and stack
 *              is in offline after scan end received.
 */
static void check_interval(void);

/**
 * @brief       Controls beacon_tx based on PosLib configuration.
 *              There is called function to check that stack is able to
 *              receive beacon tx control.
 */
static void ble_control(void);

/**
 * @brief   Wrapper for wakeup callback event generation
 */
static void cb_wakeup(void);

/**
 * @brief   Task starting periodic network scan or scan requested for oneshot
 */
static uint32_t exec_autoscan(void);

/**
 * @brief   Task which request shared_offline (nrls) to sleep
 */
static uint32_t exec_sleep(void);

/** @brief callback for PosLib events */
static volatile poslib_events_listen_info_f m_poslib_events_cb;

/** @brief Stops needed PosLib control tasks */
static void stop_tasks(void);

/**
 * @brief   Shared offline library callback when entering to offline..
 * @param   delay_s
 *          Delay the node will stay offline if none of the registered module
 *          ask to enter online state asynchronously
 */
static void on_offline_cb(uint32_t delay_s)
{
    LOG(LVL_INFO, "PosLib on_offline for %d", delay_s);
}

/**
 * @brief   Shared offline library callback when entering to online.
 *          Saves event time for new offline sleep time calculation..
 * @param   delay_from_deadline_s
 *          Delay in second per module relative to the initially requested time.
 */
static void on_online_cb(uint32_t delay_from_deadline_s)
{

    LOG(LVL_INFO, "PosLib on_online_cb, deadline in for %d",
        delay_from_deadline_s);

    m_appconf_received = false;
    m_nrls_event_time_s = lib_time->getTimestampS();
    LOG(LVL_DEBUG, "m_nrls_event_time_s: %u", m_nrls_event_time_s);
    cb_wakeup();

    if (m_pos_settings.node_mode == POSLIB_MODE_NRLS_TAG)
    {
        stop_tasks();
        App_Scheduler_addTask(exec_sleep, APP_SCHEDULER_SCHEDULE_ASAP);
        App_Scheduler_addTask(PosLibBle_check_offline,
                              POSLIB_DELAY_BLEOFFLINE_START_MS);
    }
}

/**
 * @brief   Shared offline callbacks.
 */
static offline_setting_conf_t m_shared_offline_cbs =
{
    .on_offline_event = on_offline_cb,
    .on_online_event = on_online_cb
};

/**
 * @brief       Check that given parameters are valid based on selected mode.
 * @param       settings
 * @return      POS_RET_INVALID_PARAM when parameters check fails,
 *              POS_RET_OK when parameters check success.
 */
static poslib_ret_e check_params(poslib_settings_t * settings)
{
    if ((settings->node_mode == POSLIB_MODE_NRLS_TAG ||
        settings->node_mode == POSLIB_MODE_AUTOSCAN_TAG ||
        settings->node_mode == POSLIB_MODE_AUTOSCAN_ANCHOR) &&

        (settings->update_period_static_s == 0 ||
         settings->update_period_static_s > POSLIB_MAX_MEAS_RATE_S))
    {
        LOG(LVL_ERROR, "fail period: %d", settings->update_period_static_s);
        return POS_RET_INVALID_PARAM;
    }

    if (settings->update_period_dynamic_s > POSLIB_MAX_MEAS_RATE_S)
    {
        LOG(LVL_ERROR, "fail dynamic period: %d", settings->update_period_static_s);
        return POS_RET_INVALID_PARAM;
    }

    if (settings->update_period_autoscan_offline_s > POSLIB_MAX_MEAS_RATE_S)
    {
        LOG(LVL_ERROR, "fail auto offline period: %d", settings->update_period_static_s);
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
    uint8_t base_role;
    uint8_t poslib_mode = settings->node_mode;
    lib_settings->getNodeRole(&role);
    base_role = app_lib_settings_get_base_role(role);

    switch(base_role)
    {
         case APP_LIB_SETTINGS_ROLE_SINK:
         case APP_LIB_SETTINGS_ROLE_HEADNODE:
             if (poslib_mode != POSLIB_MODE_AUTOSCAN_ANCHOR &&
                 poslib_mode != POSLIB_MODE_OPPORTUNISTIC_ANCHOR)
             {
                 LOG(LVL_ERROR, "FAIL base role router: %d", poslib_mode);
                 return POS_RET_INVALID_PARAM;
             }
         break;
         case APP_LIB_SETTINGS_ROLE_SUBNODE:
             if (poslib_mode != POSLIB_MODE_NRLS_TAG &&
                 poslib_mode != POSLIB_MODE_AUTOSCAN_TAG)
             {
                 LOG(LVL_ERROR, "FAIL base role subnode: %d", poslib_mode);
                 return POS_RET_INVALID_PARAM;
             }
         break;
         default:
             LOG(LVL_ERROR, "FAIL check_role - no role: %d", poslib_mode);
             return POS_RET_INVALID_PARAM;
         break;
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
    poslib_ble_type_e ble_type = settings->poslib_ble_settings.ble_type;

    if (ble_type != POSLIB_BEACON_ALL && ble_type != POSLIB_IBEACON &&
        ble_type != POSLIB_EDDYSTONE)
    {
        LOG(LVL_ERROR, "FAIL ble type: %d", ble_type);
        return POS_RET_INVALID_PARAM;
    }

    /** Check for valid beacon configuration if feature is enabled */
    /** ble tx max power is not checked as radio specific */
    if (ble_type == POSLIB_EDDYSTONE || ble_type == POSLIB_BEACON_ALL)
    {
        if (settings->poslib_ble_settings.ble_eddystone.ble_tx_interval_s >
            TIME_MS_TO_SEC(APP_LIB_BEACON_TX_MAX_INTERVAL) ||
            settings->poslib_ble_settings.ble_eddystone.ble_tx_interval_s == 0 ||
            settings->poslib_ble_settings.ble_eddystone.ble_channels >
                APP_LIB_BEACON_TX_CHANNELS_ALL)
        {
            LOG(LVL_ERROR, "FAIL eddystone settings: %d", ble_type);
            return POS_RET_INVALID_PARAM;
        }
    }

    if (ble_type == POSLIB_IBEACON || ble_type == POSLIB_BEACON_ALL)
    {
        if (settings-> poslib_ble_settings.ble_ibeacon.ble_tx_interval_s >
            TIME_MS_TO_SEC(APP_LIB_BEACON_TX_MAX_INTERVAL) ||
            settings-> poslib_ble_settings.ble_ibeacon.ble_tx_interval_s == 0 ||
            settings->poslib_ble_settings.ble_ibeacon.ble_channels >
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
    /** Offline is not possible for router roles */
    if (m_pos_settings.node_mode != POSLIB_MODE_AUTOSCAN_ANCHOR &&
        m_pos_settings.node_mode != POSLIB_MODE_OPPORTUNISTIC_ANCHOR)
    {
        if (!m_shared_offline_reg)
        {
            if (Shared_Offline_register(&m_shared_offline_id,
                m_shared_offline_cbs) ==
                SHARED_OFFLINE_RES_OK)
            {
                m_shared_offline_reg = true;
                LOG(LVL_DEBUG, "PosLib shared offline registered ");
                return true;
            }
            else
            {
                LOG(LVL_ERROR, "Cannot register PosLib shared offline ");
                return false;
            }
        }
    }

    return true;
}

/**
 * @brief       Unregister shared_offline library if needed.
 * @return      true if already unregistered or  unregistration success.
 *              false if needed unregistration fails.
 */
static bool shared_offline_unregister()
{
    if (m_shared_offline_reg)
    {
        if (Shared_Offline_unregister(m_shared_offline_id) ==
            SHARED_OFFLINE_RES_OK)
        {
            m_shared_offline_reg = false;
            LOG(LVL_DEBUG, "PosLib shared offline unregistered ");
            return true;
        }
        else
        {
            LOG(LVL_ERROR, "Cannot unregister PosLib shared offline ");
            return false;
        }
    }

    return true;
}

poslib_ret_e PosLibCtrl_setConf(poslib_settings_t * settings)
{
    poslib_ret_e param_check;
    poslib_events_info_t msg;
    uint16_t ac_min_value_ms, ac_max_value_ms;

    param_check = check_ctrl_params(settings);

    if (param_check != POS_RET_OK)
    {
        return param_check;
    }

    m_pos_settings.node_class = settings->node_class;
    m_pos_settings.node_mode = settings->node_mode;
    m_pos_settings.update_period_static_s = settings->update_period_static_s;
    m_pos_settings.update_period_autoscan_offline_s =
        settings->update_period_autoscan_offline_s;
    m_pos_settings.update_period_dynamic_s = settings->update_period_dynamic_s;
    m_pos_settings.poslib_ble_settings.update_period_bleon_offline_s =
        settings->poslib_ble_settings.update_period_bleon_offline_s;
    m_update_period_ms =  TIME_SEC_TO_MS(m_pos_settings.update_period_static_s);
    m_pos_settings.poslib_ble_settings = settings->poslib_ble_settings;
    PosLibBle_init(&m_pos_settings.poslib_ble_settings);

    lib_settings->getAcRange(&ac_min_value_ms, &ac_max_value_ms);

    /** appconf wait time to check status in exec_sleep task
     *  Actual wait time defined in exec_sleep task
     */
    m_appconf_wait_ms = ac_min_value_ms + 5000;
    /** wait time to additional data ack status wait. Shoul not happen unless
     *  very high data traffic load from nrls node
     */
    m_ack_wait_ms = (ac_max_value_ms * 2) + 5000;

    // Event generated that app can update persistent data area if in use
    msg.event_id = POSLIB_APPCONFIG_NEW_SETTINGS;
    PosLibCtrl_generateEvent(&msg);

    return POS_RET_OK;
}

poslib_ret_e PosLibCtrl_start(void)
{
    LOG(LVL_DEBUG, "time: %u", lib_time->getTimestampS());

    /** Registration is needed always if PosLib is not started from API
     *  and there is shared offline requested out of PosLib
     */
    shared_offline_register();
    m_appconf_received = false;

    switch (m_pos_settings.node_mode)
    {
        case POSLIB_MODE_AUTOSCAN_TAG:
        case POSLIB_MODE_AUTOSCAN_ANCHOR:
        {
            /** Do not allow callbacks until scan is started */
            PosLibMeas_removeCallbacks();

            if (App_Scheduler_addTask(exec_autoscan,
                                      m_update_period_ms) !=
                                      APP_SCHEDULER_RES_OK)
            {
                LOG(LVL_ERROR, "%s : Error adding task.", __func__);
                return POS_RET_INTERNAL_ERROR;
            }

            m_state = POSLIB_STATE_IDLE;
        }
        break;
        case POSLIB_MODE_OPPORTUNISTIC_ANCHOR:
        {
            m_state = POSLIB_STATE_OPPORTUNISTIC;
            PosLibMeas_initMeas();
        }
        break;
        case POSLIB_MODE_NRLS_TAG:
        {
            PosLibMeas_initMeas();

            if (App_Scheduler_addTask(exec_sleep, APP_SCHEDULER_SCHEDULE_ASAP) !=
                APP_SCHEDULER_RES_OK)
            {
                LOG(LVL_ERROR, "%s : Error adding task.", __func__);
                return POS_RET_INTERNAL_ERROR;
            }

            m_state = POSLIB_STATE_IDLE;
        }
        break;
        default:
        {
            LOG(LVL_ERROR, "Poslib_control_start - ERROR:",
                m_pos_settings.node_mode);
            return POS_RET_INTERNAL_ERROR;
            break;
        }
    }

    App_Scheduler_addTask(PosLibBle_check_offline,
                          POSLIB_DELAY_BLEOFFLINE_START_MS);
    return POS_RET_OK;
 }

poslib_ret_e PosLibCtrl_oneshot(void)
{
    if (lib_sleep->getSleepState() == APP_LIB_SLEEP_STARTED)
    {
        /** NRLS wakeup starts stack scan and measurement update */
        LOG(LVL_INFO, "Wakeup stack...");
        Shared_Offline_enter_online_state(m_shared_offline_id);
        return POS_RET_OK;
    }
    else
    {
        m_oneshot_prev_mode = m_pos_settings.node_mode;
        m_oneshot_prevstate = m_state;
        stop_tasks();
        /** For scan start mode needs to be autoscan tag or anchor */
        m_pos_settings.node_mode = POSLIB_MODE_AUTOSCAN_TAG;

        if (m_oneshot_prev_mode == POSLIB_MODE_AUTOSCAN_ANCHOR)
        {
            m_pos_settings.node_mode = POSLIB_MODE_AUTOSCAN_ANCHOR;
        }

        m_oneshot_update_requested = true;

        if (App_Scheduler_addTask(exec_autoscan,
            APP_SCHEDULER_SCHEDULE_ASAP) != APP_SCHEDULER_RES_OK)
        {
            LOG(LVL_ERROR, "cannot start task for oneshot");
            return POS_RET_INTERNAL_ERROR;
        }

        LOG(LVL_INFO, "oneshot started exec_autoscan");
        return POS_RET_OK;
    }
}

static void stop_tasks(void)
{
    /** Stop all scanning activity if ongoing !!! */
    App_Scheduler_cancelTask(exec_autoscan);

    /* API start to NRLS and app-config is not yet received
     * New app-config will cause tasks stop request and check is needed
     * */
    if (!m_appconf_received)
    {
        App_Scheduler_cancelTask(exec_sleep);
    }

    m_state = POSLIB_STATE_STOPPED;
}

/**
 * @brief       Calls PosLibCtrl_endScand after timeout if stack scanend cb
 *              not called. This makes sure that PosLib state is restored to
 *              state before oneshot was requested.
 * @return      APP_SCHEDULER_STOP_TASK
 */
static uint32_t check_oneshot_scanend(void)
{
    if (m_oneshot_update_requested)
    {
        LOG(LVL_ERROR, "forced scan end if no stack scan end");
        PosLibCtrl_endScan();
    }

    return APP_SCHEDULER_STOP_TASK;
}

void PosLibCtrl_stop(void)
{
    stop_tasks();
    App_Scheduler_cancelTask(exec_sleep);
    Shared_Appconfig_removeFilter(m_appconfig_id);
    Shared_Shutdown_removeShutDownCb(m_shutdown_id);
    PosLibMeas_removeCallbacks();
    /** shared offline not in use if PosLib is stopped */
    shared_offline_unregister();
    /* Disable possible BLE set */
    PosLibBle_set(POSLIB_BLE_STOP);
    App_Scheduler_cancelTask(PosLibBle_check_offline);
}

static uint32_t exec_autoscan(void)
{
    uint32_t new_delay_ms = APP_SCHEDULER_STOP_TASK;

    if (m_state != POSLIB_STATE_PERFORM_SCAN)
    {
        LOG(LVL_DEBUG, "exec_autoscan: %d - time: %u - update_ms: %u - oneshot: %d",
            m_state, lib_time->getTimestampS(), m_update_period_ms,
            m_oneshot_update_requested);

        PosLibMeas_initMeas();
        m_state = POSLIB_STATE_PERFORM_SCAN;
        PosLibMeas_startScan(&m_pos_settings);
        new_delay_ms = m_update_period_ms;
    }
    else
    {
        LOG(LVL_ERROR, "race condition - m_state: %d", m_state);
    }

    if (m_oneshot_update_requested)
    {
        if (m_state == POSLIB_STATE_PERFORM_SCAN)
        {
            App_Scheduler_addTask(check_oneshot_scanend,
                                  POSLIB_FORCED_ONESHOT_RESTORE_MS);
        }

        return APP_SCHEDULER_STOP_TASK;
    }

    return new_delay_ms;
}

static uint32_t exec_sleep(void)
{
    shared_offline_res_e shared_offline_res;
    uint32_t new_interval_s = 0;
    uint32_t delta_time_s = 0;
    uint32_t update_period_s = 0;

    /** appconfig receive status is checked every m_appconf_wait_ms */
    if (!m_appconf_received && m_nbr_appconf_waits < POSLIB_NBR_OF_ACMIN_LOOPS)
    {
        m_nbr_appconf_waits++;
        LOG(LVL_DEBUG, "m_appconf_wait_active - wait_ms: %u, nbr: %u",
            m_appconf_wait_ms, m_nbr_appconf_waits);
        return m_appconf_wait_ms;
    }

    /** Additional wait if no ack received for sent PosLib measurement message */
    if (!PosLibEvent_AckReceived() && m_ack_wait_active == false)
    {
        LOG(LVL_DEBUG, "m_ack_wait_active: %u", m_ack_wait_ms);
        m_ack_wait_active = true;
        return m_ack_wait_ms;
    }

    m_ack_wait_active = false;
    m_nbr_appconf_waits = 0;
    update_period_s = TIME_MS_TO_SEC(m_update_period_ms);

    delta_time_s = lib_time->getTimestampS() - m_nrls_event_time_s;
    new_interval_s = (update_period_s - delta_time_s) % update_period_s;

    shared_offline_res =
        Shared_Offline_enter_offline_state(m_shared_offline_id, new_interval_s);

    LOG(LVL_DEBUG, "delta_time_s: %u - new_interval_s: %u",
        delta_time_s, new_interval_s);

    if (shared_offline_res != SHARED_OFFLINE_RES_OK)
    {
        LOG(LVL_ERROR, "SHARED_OFFLINE_RES ERROR: %u:", shared_offline_res);
    }

    return APP_SCHEDULER_STOP_TASK;
}

static void cb_wakeup(void)
{
    poslib_events_info_t msg;

    m_state = POSLIB_STATE_PERFORM_SCAN;
    msg.event_id = POSLIB_WM_WAKE_UP_REQUEST;
    PosLibCtrl_generateEvent(&msg);
}

/**
 * @brief   Wrapper for system shutdown callback
 */
static void cb_systemshutdown(void)
{
    if (m_pos_settings.node_mode == POSLIB_MODE_NRLS_TAG)
    {
        poslib_events_info_t msg;
        uint32_t time_diff_s = 0;

        time_diff_s =
            lib_time->getTimestampS() - m_nrls_event_time_s;
        msg.nrls_sleep_time_sec =
            TIME_MS_TO_SEC(m_update_period_ms) - time_diff_s;
        msg.event_id = POSLIB_WM_SLEEP_REQUEST;
        PosLibCtrl_generateEvent(&msg);
        m_state = POSLIB_STATE_IDLE;
    }
}

static void appconfig_cb(uint16_t types, uint8_t length, uint8_t * bytes)
{
    LOG(LVL_DEBUG, "time: %u", lib_time->getTimestampS());

    PosLibEvent_AppConfig(bytes, length);
    m_appconf_received = true;
}

/**
 * @brief   Task to call ble_control.
 */
static uint32_t ble_control_stack_wait(void)
{
    ble_control();
    return APP_SCHEDULER_STOP_TASK;
}

/**
 * @brief   If stack is not started there cannot be started beacon tr.
 *          Task ble_control_stack_wait is called with delay to wait stack
 *          start.
 */
static bool check_stackstate(void)
{
    /** Stack needs to be started. NRLS can be active if stack is started */
    if ((lib_state->getStackState() == APP_LIB_STATE_STOPPED) &&
        (lib_sleep->getSleepState() == APP_LIB_SLEEP_STOPPED))
    {
        LOG(LVL_DEBUG, "ble_control stack wait - stack state: %d, sleep state: %d",
            lib_state->getStackState(), lib_sleep->getSleepState());
        App_Scheduler_addTask(ble_control_stack_wait, POSLIB_STACK_WAIT_MS);
        return false;
    }
    return true;
}

static void ble_control(void)
{
    if (m_pos_settings.poslib_ble_settings.ble_mode == POSLIB_BLE_START)
    {
        if (check_stackstate())
        {
            PosLibBle_set(POSLIB_BLE_START);
        }
    }
    else if (PosLibBle_checkActive())
    {
        if (check_stackstate())
        {
            PosLibBle_set(POSLIB_BLE_STOP);
        }
    }
}

poslib_ret_e PosLibCtrl_init(void)
{
    LOG(LVL_INFO, "");

    if (m_state != POSLIB_STATE_STOPPED)
    {
        LOG(LVL_ERROR,"wrong m_state: %d: ",m_state);
        return POS_RET_INVALID_STATE;
    }

    app_lib_settings_role_t role;
    lib_settings->getNodeRole(&role);


    shared_app_config_filter_t app_config_filter;

    app_config_filter.type = POSLIB_TLV_TYPE;
    app_config_filter.cb =
        (shared_app_config_received_cb_f)appconfig_cb;
    Shared_Appconfig_addFilter(&app_config_filter,&m_appconfig_id);
    Shared_Shutdown_addShutDownCb(cb_systemshutdown,&m_shutdown_id);

    /** Shared offline in use when PosLib is started.
     *  This prevents applications requesting to offline when PosLib is
     *  not set NRLS mode. If PosLib set to NRLS mode offline works both
     *  in PosLib and in application
     */
    if (!shared_offline_register())
    {
        return POS_RET_LIB_ERROR;
    }


    PosLibDecode_init();
    PosLibMeas_resetTable();
    m_update_period_ms = TIME_SEC_TO_MS(m_pos_settings.update_period_static_s);
    m_pos_settings.source_endpoint = POS_SOURCE_ENDPOINT;
    m_pos_settings.destination_endpoint = POS_DESTINATION_ENDPOINT;
    m_oneshot_update_requested = false;
    m_motion_mode = POSLIB_MOTION_DYNAMIC;

    /** BLE started if ble_mode set in configuration and PosLib is started.
     *  BLE can be started/stopped also dynamically through app config change */
    ble_control();
    return POS_RET_OK;
}

poslib_settings_t * Poslib_get_settings(void)
{
    return &m_pos_settings;
}

void PosLibCtrl_updateSettings(poslib_settings_t * settings)
{
    poslib_events_info_t msg;

    // Event generated that app can update persistent data area if in use
    msg.event_id = POSLIB_APPCONFIG_NEW_SETTINGS;
    PosLibCtrl_generateEvent(&msg);
    stop_tasks();
    PosLibMeas_initMeas();
    m_pos_settings.node_mode = settings->node_mode;
    m_pos_settings.update_period_static_s = settings->update_period_static_s;
    m_update_period_ms =  TIME_SEC_TO_MS(m_pos_settings.update_period_static_s);
    PosLibCtrl_start();
}

uint16_t PosLibCtrl_incrementSeqId(void)
{
    m_message_sequence_id++;
    return m_message_sequence_id;
}

uint16_t PosLibCtrl_getSeqId(void)
{
    return m_message_sequence_id;
}

static void check_interval(void)
{
    // In autoscan mode if no scan results and offline interval is defined
    // there is used update_period_autoscan_offline_s period to save power
    if (PosLibMeas_getNumofBeacons() == 0 &&
        m_pos_settings.update_period_autoscan_offline_s != 0)
    {
        m_update_period_ms =
            TIME_SEC_TO_MS(m_pos_settings.update_period_autoscan_offline_s);
        LOG(LVL_DEBUG, "check_interval-OFFLINE: %d",
            m_pos_settings.update_period_autoscan_offline_s);
    }
    else
    {
        m_update_period_ms =
            TIME_SEC_TO_MS(m_pos_settings.update_period_static_s);
        LOG(LVL_DEBUG, "check_interval-ONLINE: %d",
            m_pos_settings.update_period_static_s);
    }
}

void PosLibCtrl_endScan(void)
{
    /** If oneshot measurement - there is needed to restore previous state */
    if (m_oneshot_update_requested)
    {
        m_pos_settings.node_mode = m_oneshot_prev_mode;
        m_state = m_oneshot_prevstate;
        m_oneshot_update_requested = false;
        m_oneshot_prevstate = 0;
        m_oneshot_prev_mode = 0;
        /** Cancel backup task for checking that this function happens */
        App_Scheduler_cancelTask(check_oneshot_scanend);

        if (m_state != POSLIB_STATE_STOPPED)
        {
            LOG(LVL_DEBUG, "restart - node_mode: %d - m_state: %d",
                m_pos_settings.node_mode, m_state);
            PosLibCtrl_start();
        }
        else
        {
            LOG(LVL_DEBUG, "no restart needed - node_mode: %d - m_state: %d",
                m_pos_settings.node_mode, m_state);
        }
    }
    else
    {
        check_interval();
        m_state = POSLIB_STATE_IDLE;
    }
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

void PosLibCtrl_setEventCb(poslib_events_e requested_event,
                           poslib_events_listen_info_f callback)
{
    switch (requested_event)
    {
        case POSLIB_UPDATE_ALL:
        {
            BITFIELD_SET(m_events_bitmask, POSLIB_UPDATE_START);
            BITFIELD_SET(m_events_bitmask, POSLIB_UPDATE_END);
        }
        break;
        case POSLIB_BLE_ALL:
        {
            BITFIELD_SET(m_events_bitmask, POSLIB_BLE_START_EVENT);
            BITFIELD_SET(m_events_bitmask, POSLIB_BLE_STOP_EVENT);
        }
        break;
        case POSLIB_NRLS_ALL:
        {
            BITFIELD_SET(m_events_bitmask, POSLIB_WM_SLEEP_REQUEST);
            BITFIELD_SET(m_events_bitmask, POSLIB_WM_WAKE_UP_REQUEST);
        }
        break;
        case POSLIB_APPCONFIG_NEW_SETTINGS:
        {
            BITFIELD_SET(m_events_bitmask, POSLIB_APPCONFIG_NEW_SETTINGS);
        }
        break;
        default:
            BITFIELD_SET(m_events_bitmask, requested_event);
        break;
    }

    m_poslib_events_cb = callback;
}

void PosLibCtrl_generateEvent(poslib_events_info_t * msg)
{
    bool  event_req_active = BITFIELD_GET(m_events_bitmask, msg->event_id);

    if (msg->event_id == POSLIB_UPDATE_START)
    {
        m_last_update_time_s = lib_time->getTimestampS();
    }

    if (event_req_active)
    {
        msg->time_hp = lib_time->getTimestampHp();
        m_poslib_events_cb(msg);
    }
}

void PosLibCtrl_clearEventCb(poslib_events_e requested_event)
{
    switch (requested_event)
    {
        case POSLIB_UPDATE_ALL:
        {
            BITFIELD_CLEAR(m_events_bitmask, POSLIB_UPDATE_START);
            BITFIELD_CLEAR(m_events_bitmask, POSLIB_UPDATE_END);
        }
        break;
        case POSLIB_BLE_ALL:
        {
            BITFIELD_CLEAR(m_events_bitmask, POSLIB_BLE_START_EVENT);
            BITFIELD_CLEAR(m_events_bitmask, POSLIB_BLE_STOP_EVENT);
        }
        break;
        case POSLIB_NRLS_ALL:
        {
            BITFIELD_CLEAR(m_events_bitmask, POSLIB_WM_SLEEP_REQUEST);
            BITFIELD_CLEAR(m_events_bitmask, POSLIB_WM_WAKE_UP_REQUEST);
        }
        break;
        case POSLIB_APPCONFIG_NEW_SETTINGS:
        {
             BITFIELD_CLEAR(m_events_bitmask, POSLIB_APPCONFIG_NEW_SETTINGS);
        }
        break;
        default:
            BITFIELD_CLEAR(m_events_bitmask, requested_event);
        break;
    }
}

poslib_ret_e PosLibCtrl_motion(poslib_motion_mode_e mode)
{
    uint32_t next_update_ms = 0;
    int32_t next_update_s = 0;
    uint32_t time_now_s = 0;

    LOG(LVL_INFO, "");

    if (m_state != POSLIB_STATE_STOPPED)
    {
        stop_tasks();
        PosLibMeas_initMeas();

        if (mode == POSLIB_MOTION_STATIC)
        {
            m_update_period_ms =
                TIME_SEC_TO_MS(m_pos_settings.update_period_static_s);
            m_motion_mode = POSLIB_MOTION_STATIC;
        }
        else if (mode == POSLIB_MOTION_DYNAMIC)
        {
            time_now_s = lib_time->getTimestampS();
            m_update_period_ms =
                TIME_SEC_TO_MS(m_pos_settings.update_period_dynamic_s);
            /** if the last update is older than (current time - dynamic rate)
             *  when moving from static to dynamic,
             *  an update will be triggered immediately.
             *  If sleep started wakeup which triggers scan and update
             */

            /** otherwise the next update is re-scheduled as
             * (last update + dynamic rate) ->
             * (dynamic rate s - (curr time s  - last update s))
             */

            next_update_s = TIME_MS_TO_SEC(m_update_period_ms) -
                            (time_now_s - m_last_update_time_s);

            if (next_update_s < 0)
            {
                next_update_ms = m_update_period_ms;
            }
            else
            {
                next_update_ms = TIME_SEC_TO_MS(next_update_s);
            }

            if (lib_sleep->getSleepState() == APP_LIB_SLEEP_STARTED)
            {
                /** NRLS wakeup starts stack scan and measurement update */
                    Shared_Offline_enter_online_state(m_shared_offline_id);
            }
            else if (m_last_update_time_s > (time_now_s -
                     m_pos_settings.update_period_dynamic_s) &&
                     m_motion_mode == POSLIB_MOTION_STATIC)
            {
                if (App_Scheduler_addTask(exec_autoscan,
                    APP_SCHEDULER_SCHEDULE_ASAP) != APP_SCHEDULER_RES_OK)
                {
                    return POS_RET_INTERNAL_ERROR;
                }
            }
            else if (App_Scheduler_addTask(exec_autoscan, next_update_ms) !=
                APP_SCHEDULER_RES_OK)
            {
                return POS_RET_INTERNAL_ERROR;
            }

            m_motion_mode = POSLIB_MOTION_DYNAMIC;
        }
        else
        {
            return POS_RET_INVALID_STATE;
        }
    }
    else
    {
        return POS_RET_INVALID_STATE;
    }

    return POS_RET_OK;
}

poslib_status_e PosLibCtrl_status(void)
{
    if (m_state == POSLIB_STATE_STOPPED)
    {
        return POSLIB_STOPPED;
    }
    else if (m_state == POSLIB_STATE_PERFORM_SCAN)
    {
        return POSLIB_STARTED_UPDATE_ONGOING;
    }
    else
    {
        return POSLIB_STARTED_IDLE;
    }
}


bool posLibCtrl_getOneshotStatus(void)
{
    return m_oneshot_update_requested;
}

