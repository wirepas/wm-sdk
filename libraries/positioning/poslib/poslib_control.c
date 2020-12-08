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

/** The state of the positioning state machine. */
static positioning_state_e m_state = POSLIB_STATE_STOPPED;
/** Counter for messages sent */
static uint16_t m_message_sequence_id;
/** Check for message sent in NRLS mode */
static bool     m_data_ack_received = false;
/** State to wait appconfig receiving in NRLS mode */
static bool     m_appconf_wait_activated = false;
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
/** Offline timeout started when no scan results and offline timeout set */
static bool m_offline_timeout_on;
/** Oneshot update req from api */
static bool m_oneshot_update_requested;
/** Used to save time when entering to sleep for adjust next period */
static uint32_t m_time_since_start_s;

/** Time defined to wait app_config receiving before entering to sleep */
#define APPCONFIG_WAITTIME_IN_SEC   20
/** For ism 2.4 bands */
#define MIN_ACYCLE_LENGTH_US     (4*500000)
#define TIME_USEC_TO_SEC(tm) ((tm) / 1000000U)
#define TIME_SEC_TO_MS(tm)  ((tm) * 1000U)
#define TIME_MS_TO_SEC(tm) ((tm) / 1000U)
#define MIN_ACYCLES_AS_SECONDS(n) \
        TIME_USEC_TO_SEC(((uint32_t) (n) * MIN_ACYCLE_LENGTH_US) + 500000)
/** For app_config receiving TLV type for positioning control */
#define POSLIB_TLV_TYPE     0x99

/**
 * @brief   The positioning state machine. React to events raised in this module.
 * @note    This function uses the fact the system is not preemptive and that
 *          all events are raised from non interrupt context.
 * @return  Time in ms to schedule this function again.
 */
static uint32_t exec_poslib_ctrl(void);

/**
 * @brief   Task to check if route is found. (called after scan end)
 * @return  APP_SCHEDULER_STOP_TASK task not continues
 */
static uint32_t run_check_offline(void);

/**
 * @brief   Task to perform functions for one positioining update.
 * @return  APP_SCHEDULER_STOP_TASK task not continues
 */
static uint32_t oneshot_measure(void);


/**
 * @brief       Changes autoscan mode  scan interval to
 *              update_period_autoscan_offline_s if value defined and stack
 *              is in offline after scan end received.
 */
static void check_interval(void);

/**
 * @brief       Enables ble after timeout update_period_bleon_offline_s with
 *              same interval if stack is in offline after scan end received and
 *              the value is defined.
 */
static void check_bleon_offline(void);

/** @brief callback for PosLib events */
static volatile poslib_events_listen_info_f m_poslib_events_cb;

/**
 * @brief       Checking set PosLib settings range.
 * @return      POS_RET_INVALID_PARAM when parameters check fails,
 *              POS_RET_OK when parameters check success.
 */
poslib_ret_e check_ctrl_params(poslib_settings_t * settings)
{
    return POS_RET_OK;
}

poslib_ret_e PosLibCtrl_setConf(poslib_settings_t * settings)
{
    poslib_ret_e param_check;
    poslib_events_info_t msg;

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
    m_pos_settings.update_period_bleon_offline_s =
        settings->update_period_bleon_offline_s;
    m_update_period_ms =  TIME_SEC_TO_MS(m_pos_settings.update_period_static_s);
    m_pos_settings.poslib_ble_settings = settings->poslib_ble_settings;
    PosLibBle_init(&m_pos_settings.poslib_ble_settings);

    // Event generated that app can update persistent data area if in use
    msg.event_id = POSLIB_APPCONFIG_NEW_SETTINGS;
    PosLibCtrl_generateEvent(&msg);

    return POS_RET_OK;
}

poslib_ret_e PosLibCtrl_start(void)
{
    PosLibCtrl_stop(false);

    switch (m_pos_settings.node_mode)
    {
        case POSLIB_MODE_AUTOSCAN_TAG:
        case POSLIB_MODE_AUTOSCAN_ANCHOR:
        {
            m_state = POSLIB_STATE_PERFORM_SCAN;
        }
        break;
        case POSLIB_MODE_OPPORTUNISTIC_ANCHOR:
        {
            m_state = POSLIB_STATE_OPPORTUNISTIC;
        }
        break;
        case POSLIB_MODE_NRLS_TAG:
        {
            m_state = POSLIB_STATE_NRLS_START;
        }
        break;
        default:
        {
            return POS_RET_INTERNAL_ERROR;
            break;
        }
    }

    /** Randomize to prevent events at the same time to network */
    if (App_Scheduler_addTask(exec_poslib_ctrl,
                              Random_jitter(10)) !=
                              APP_SCHEDULER_RES_OK)
    {
          return POS_RET_INTERNAL_ERROR;
    }

    return POS_RET_OK;
 }

poslib_ret_e PosLibCtrl_oneshot(void)
{

    if (App_Scheduler_addTask(oneshot_measure,
        APP_SCHEDULER_SCHEDULE_ASAP) != APP_SCHEDULER_RES_OK)
    {
        return POS_RET_INTERNAL_ERROR;
    }

    return POS_RET_OK;
}

/**
 * @brief   Poslib oneshot task running initialization for positioning
 *          update. If node in NRLS sleep - stack is wake up for to get
 *          positioning update.
 * @return  APP_SCHEDULER_STOP_TASK.
 */
static uint32_t oneshot_measure(void)
{

    if (lib_sleep->getSleepState() == APP_LIB_SLEEP_STARTED)
    {
        /** NRLS wakeup starts stack scan and measurement update */
        lib_sleep->wakeupStack();
    }
    else
    {
        m_oneshot_update_requested = true;
        PosLibCtrl_stop(false);
        PosLibMeas_initMeas();
        PosLibMeas_startScan(&m_pos_settings);

        if ((m_pos_settings.node_mode == POSLIB_MODE_AUTOSCAN_TAG ||
             m_pos_settings.node_mode == POSLIB_MODE_AUTOSCAN_ANCHOR) &&
            m_state != POSLIB_STATE_STOPPED)
        {
            m_state = POSLIB_STATE_PERFORM_SCAN;
            App_Scheduler_addTask(exec_poslib_ctrl,
            APP_SCHEDULER_SCHEDULE_ASAP);
        }
    }

    return APP_SCHEDULER_STOP_TASK;
}

void PosLibCtrl_stop(bool callbacks_removed)
{
    /** Stop all scanning activity if ongoing !!! */
    App_Scheduler_cancelTask(exec_poslib_ctrl);
    App_Scheduler_cancelTask(run_check_offline);

    if (callbacks_removed)
    {
        Shared_Appconfig_removeFilter(m_appconfig_id);
        lib_sleep->setOnWakeupCb(NULL);
        Shared_Shutdown_removeShutDownCb(m_shutdown_id);
        PosLibMeas_removeCallbacks();
    }

    m_data_ack_received = false;
    m_appconf_wait_activated = false;

    if (!m_oneshot_update_requested)
    {
        m_state = POSLIB_STATE_STOPPED;
    }
}

static uint32_t exec_poslib_ctrl(void)
{
    uint8_t res = 0;
    uint32_t new_delay_ms = APP_SCHEDULER_STOP_TASK;
    uint32_t exec_time_s = 0;
    uint32_t new_interval_s = 0;


    switch (m_state)
    {
        case POSLIB_STATE_IDLE:
        {
            m_state = POSLIB_STATE_IDLE;
            new_delay_ms = APP_SCHEDULER_STOP_TASK;
            break;
        }
        case POSLIB_STATE_OPPORTUNISTIC:
        {
            m_oneshot_update_requested = false;
            PosLibMeas_initMeas();
            new_delay_ms = APP_SCHEDULER_STOP_TASK;
            break;
        }
        case POSLIB_STATE_NRLS_START:
        {
            m_oneshot_update_requested = false;
            PosLibMeas_initMeas();
            m_state = POSLIB_STATE_TOSLEEP;
            new_delay_ms = APP_SCHEDULER_SCHEDULE_ASAP;
            break;
        }
        case POSLIB_STATE_PERFORM_SCAN:
        {
            PosLibMeas_initMeas();
            m_state = POSLIB_STATE_PERFORM_SCAN;
            m_oneshot_update_requested = false;
            PosLibMeas_startScan(&m_pos_settings);

            /** SCAN with NRLS is requested only once when mode is changed */
            if (m_pos_settings.node_mode == POSLIB_MODE_NRLS_TAG)
            {
                return APP_SCHEDULER_STOP_TASK;
            }
            else
            {
                new_delay_ms = m_update_period_ms;
            }

            break;
        }
        case POSLIB_STATE_TOSLEEP:
        {
            if (m_data_ack_received == true || m_appconf_wait_activated == true)
            {
                m_nrls_event_time_s = lib_time->getTimestampS();

                // calculates elapsed time and adjust the period
                exec_time_s = m_nrls_event_time_s - m_time_since_start_s;

                if (TIME_MS_TO_SEC(m_update_period_ms) < exec_time_s)
                {
                    new_interval_s =
                            TIME_MS_TO_SEC(m_update_period_ms) +
                        (TIME_MS_TO_SEC(m_update_period_ms) - exec_time_s);
                }
                else
                {
                    new_interval_s = TIME_MS_TO_SEC(m_update_period_ms);
                }


                res = lib_sleep->sleepStackforTime(
                          new_interval_s,
                          APPCONFIG_WAITTIME_IN_SEC);
                m_data_ack_received = false;
                m_appconf_wait_activated = false;
                m_time_since_start_s = lib_time->getTimestampS();

                if (res == APP_RES_OK)
                {
                    m_state = POSLIB_STATE_IDLE;
                    return APP_SCHEDULER_STOP_TASK;
                }
                else
                {
                    m_state = POSLIB_STATE_PERFORM_SCAN;
                    return m_update_period_ms;
                }
            }
            else
            {
                m_appconf_wait_activated = true;
                return (MIN_ACYCLES_AS_SECONDS(3)*1000);
            }
        }
        default:
        {
            break;
        }
    }

    return new_delay_ms;
}


/**
 * @brief   Wrapper for wakeup callback
 */
static void cb_wakeup(void)
{

    poslib_events_info_t msg;
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
    }
}

static void appconfig_cb(uint16_t types, uint8_t length, uint8_t * bytes)
{
    PosLibEvent_AppConfig(bytes, length);
}

void PosLibCtrl_init(void)
{
    app_lib_settings_role_t role;
    lib_settings->getNodeRole(&role);


    shared_app_config_filter_t app_config_filter;

    app_config_filter.type = POSLIB_TLV_TYPE;
    app_config_filter.cb =
        (shared_app_config_received_cb_f)appconfig_cb;
    Shared_Appconfig_addFilter(&app_config_filter,&m_appconfig_id);
    lib_sleep->setOnWakeupCb(cb_wakeup);
    Shared_Shutdown_addShutDownCb(cb_systemshutdown,&m_shutdown_id);

    PosLibDecode_init();
    PosLibMeas_resetTable();
    m_offline_timeout_on = false;
    m_update_period_ms = TIME_SEC_TO_MS(m_pos_settings.update_period_static_s);
    m_pos_settings.source_endpoint = POS_SOURCE_ENDPOINT;
    m_pos_settings.destination_endpoint = POS_DESTINATION_ENDPOINT;
    m_oneshot_update_requested = false;
    m_motion_mode = POSLIB_MOTION_DYNAMIC;

    /** BLE started if ble_mode set in configuration and PosLib is started.
     *  BLE can be started/stopped also dynamically through app config change */
    if (m_pos_settings.poslib_ble_settings.ble_mode == POSLIB_BLE_START)
    {
        PosLib_BLE(POSLIB_BLE_START);
    }
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
    PosLibCtrl_stop(false);
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
        LOG(LVL_DEBUG, "check_interval-OFFLINE: %d", m_pos_settings.update_period_autoscan_offline_s);
    }
    else
    {
        m_update_period_ms =
            TIME_SEC_TO_MS(m_pos_settings.update_period_static_s);
        LOG(LVL_DEBUG, "check_interval-ONLINE: %d", m_pos_settings.update_period_static_s);
    }
}

static void check_bleon_offline(void)
{
    if (m_pos_settings.update_period_bleon_offline_s != 0 &&
        PosLibMeas_getNumofBeacons() == 0 && m_offline_timeout_on == false)
    {
        App_Scheduler_addTask(run_check_offline,
            TIME_SEC_TO_MS(m_pos_settings.update_period_bleon_offline_s));
        m_offline_timeout_on = true;
    }
    else if (m_pos_settings.update_period_bleon_offline_s != 0 &&
        PosLibMeas_getNumofBeacons() == 0 && m_offline_timeout_on == false)
    {
        PosLibBle_set();
        m_offline_timeout_on = false;
    }
}

void PosLibCtrl_endScan(void)
{

    check_interval();
    check_bleon_offline();

    if (m_pos_settings.node_mode == POSLIB_MODE_NRLS_TAG)
    {
        m_state= POSLIB_STATE_TOSLEEP;

        if (App_Scheduler_addTask(exec_poslib_ctrl,
                                  APP_SCHEDULER_SCHEDULE_ASAP) !=
                                 APP_SCHEDULER_RES_OK)
        {
            LOG(LVL_ERROR, "%s : Error adding task.", __func__);
            PosLibCtrl_stop(false);
            m_state= POSLIB_STATE_TOSLEEP;
            App_Scheduler_addTask(exec_poslib_ctrl, APP_SCHEDULER_SCHEDULE_ASAP);
        }
        else
        {
            LOG(LVL_INFO, "NRLS to sleep");
        }
    }
    else if (m_pos_settings.node_mode == POSLIB_MODE_AUTOSCAN_TAG ||
            m_pos_settings.node_mode == POSLIB_MODE_AUTOSCAN_ANCHOR)
    {
        m_state= POSLIB_STATE_PERFORM_SCAN;
    }
    else if (m_pos_settings.node_mode == POSLIB_MODE_OPPORTUNISTIC_ANCHOR)
    {
        m_state= POSLIB_STATE_OPPORTUNISTIC;
        App_Scheduler_addTask(exec_poslib_ctrl, APP_SCHEDULER_SCHEDULE_ASAP);
    }
}

void PosLibCtrl_receiveAck(void)
{
    m_data_ack_received = true;

    if (m_appconf_wait_activated == true)
    {
        PosLibCtrl_stop(false);
        m_data_ack_received = true;
        m_appconf_wait_activated = true;
        m_state = POSLIB_STATE_TOSLEEP;
        App_Scheduler_addTask(exec_poslib_ctrl, APP_SCHEDULER_SCHEDULE_ASAP);
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
    positioning_state_e prev_state;
    uint32_t next_update_ms = 0;
    int32_t next_update_s = 0;
    uint32_t time_now_s = 0;

    if (m_state != POSLIB_STATE_STOPPED)
    {
        PosLibCtrl_stop(false);
        PosLibMeas_initMeas();
        prev_state = m_state;
        m_state = POSLIB_STATE_PERFORM_SCAN;

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
                lib_sleep->wakeupStack();
            }
            else if (m_last_update_time_s > (time_now_s -
                     m_pos_settings.update_period_dynamic_s) &&
                     m_motion_mode == POSLIB_MOTION_STATIC)
            {
                if (App_Scheduler_addTask(exec_poslib_ctrl,
                    APP_SCHEDULER_SCHEDULE_ASAP) != APP_SCHEDULER_RES_OK)
                {
                    m_state = prev_state;
                    return POS_RET_INTERNAL_ERROR;
                }
            }
            else if (App_Scheduler_addTask(exec_poslib_ctrl, next_update_ms) !=
                APP_SCHEDULER_RES_OK)
            {
                m_state = prev_state;
                return POS_RET_INTERNAL_ERROR;
            }

            m_motion_mode = POSLIB_MOTION_DYNAMIC;
        }
        else
        {
            m_state = prev_state;
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
    else if (m_state == POSLIB_STATE_PERFORM_SCAN ||
             m_state == POSLIB_STATE_ONESHOT_SCAN)
    {
        return POSLIB_STARTED_UPDATE_ONGOING;
    }
    else
    {
        return POSLIB_STARTED_IDLE;
    }
}

static uint32_t run_check_offline(void)
{
    app_res_e rval = 0;
    size_t count = 0;


    rval = lib_state->getRouteCount(&count);

    if (rval == APP_RES_OK && count == 0 &&
        lib_sleep->getSleepState() == APP_LIB_SLEEP_STOPPED)
    {
        PosLibBle_enableOffline();
    }

    m_offline_timeout_on = true;
    return APP_SCHEDULER_STOP_TASK;
}


bool posLibCtrl_getOneshotStatus(void)
{
    return m_oneshot_update_requested;
}

