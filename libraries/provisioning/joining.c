/* Copyright 2019 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

#include <string.h>

#include "provisioning.h"
#include "provisioning_int.h"
#include "app_scheduler.h"
#include "stack_state.h"

#define DEBUG_LOG_MODULE_NAME "JOIN LIB"
#define DEBUG_LOG_MAX_LEVEL LVL_INFO
#include "debug_log.h"

/** Maximum number of joining beacons to receive in one go. */
#define MAX_NUM_RX_JOINING_BEACONS 10

/** Delay (in ms) to restart joining if beacon scan failed. */
#define DELAY_RESTART_SCAN_MS 5000

/** Delay (in ms)  to wait end of joining beacon scan. */
#define DELAY_WAIT_END_SCAN_MS 5000

/** Delay (in ms)  to wait end of joining process. */
#define DELAY_WAIT_END_JOINING_MS 20000

/** Execution time of the Joining beacon callback (measured 11 uS). */
#define JOINING_CB_EXEC_TIME_US 30

/** \brief Joining state machine states. */
typedef enum
{
    JOIN_STATE_UNINIT = 0, /**< Library is not initialized. */
    JOIN_STATE_IDLE = 1, /**< Waiting Start event. */
    JOIN_STATE_START = 2, /**< Start scanning joining beacons. */
    JOIN_STATE_WAIT_SCAN_END = 3, /**< Wait scan end event. */
    JOIN_STATE_WAIT_ROUTE_CHANGE = 4, /**< Wait route change event. */
} joining_state_e;

/** \brief Joining events. */
static struct
{
    uint8_t scan_end:1; /**< Joining beacon scan end event. */
    uint8_t route_change:1; /**< Route changed event. */
    uint8_t timeout:1; /**< Timeout event occured. */
    uint8_t start:1; /**< Application started joining. */
} m_events;

/** Hold how many retries are left for joining. */
static uint8_t m_retry;
/** The state of the joining state machine. */
static joining_state_e m_state = JOIN_STATE_UNINIT;
/** Copy of the configuration passed to joining library. */
static provisioning_joining_conf_t m_conf;


//Static memory could be optimized by giving memory management to application.
/** Buffer used to store received beacons.*/
static app_lib_joining_received_beacon_t
    m_beacon_rx_buffer[MAX_NUM_RX_JOINING_BEACONS];
/** Number of beacons received during last scan. */
static size_t m_num_beacons;

/** Function forward declaration. */
static uint32_t run_state_machine(void);
static void reset_joining(bool stopJoining);

/**
 * \brief   Remove non provisioning beacons (whose type is not
 *          JOINING_BEACON_TYPE) from beacon list.
 * \param   beacons
 *          A pointer to the first beacon or NULL
 * \return  A pointer to the (new) first beacon or NULL
 */
app_lib_joining_received_beacon_t * sort_provisioning_beacons(
                            app_lib_joining_received_beacon_t * beacons)
{
    app_lib_joining_received_beacon_t * beacon;
    app_lib_joining_received_beacon_t * prev;
    app_lib_joining_received_beacon_t * start;
    beacon = beacons;
    prev = beacons;
    start = beacons;

    while (beacon != NULL)
    {
        if (beacon->type != JOINING_BEACON_TYPE)
        {
            LOG(LVL_DEBUG, "sort_provisioning_beacons : remove beacon");
            if(beacon == start)
            {
                start = beacon->next;
            }
            else
            {
                prev->next = beacon->next;
            }
        }
        beacon = beacon->next;
    }

    return start;
}

/**
 * \brief   Joining beacon reception done callback. Raises scan_end event.
 * \param   status
 *          Status of reception \ref app_lib_joining_rx_status_e
 * \param   beacons
 *          Received beacons
 * \param   num_beacons
 *          Number of beacons received
 */
static void joining_beacon_rx_cb(
                            app_lib_joining_rx_status_e status,
                            const app_lib_joining_received_beacon_t * beacons,
                            size_t num_beacons)
{
    LOG(LVL_INFO, "Event : SCAN END.");
    LOG(LVL_DEBUG, "status : %u, num_beacons : %u",
                   (unsigned int)status, (unsigned int)num_beacons);

    m_num_beacons = num_beacons;
    m_events.scan_end = 1;
    if (App_Scheduler_addTask_execTime(run_state_machine,
                              APP_SCHEDULER_SCHEDULE_ASAP,
                              500) != APP_SCHEDULER_RES_OK)
    {
        reset_joining(true);
        m_conf.end_cb(PROV_RES_ERROR_INTERNAL);
        LOG(LVL_ERROR, "Event : SCAN END, Error adding task.");
    }
}

/**
 * \brief   Route changed callback. Raises route_change event.
 */
static void route_changed_cb(app_lib_stack_event_e event, void * param)
{
    LOG(LVL_INFO, "Event : ROUTE CHANGE.");
    m_events.route_change = 1;
    if (App_Scheduler_addTask_execTime(run_state_machine,
                              APP_SCHEDULER_SCHEDULE_ASAP,
                              500) != APP_SCHEDULER_RES_OK)
    {
        reset_joining(true);
        m_conf.end_cb(PROV_RES_ERROR_INTERNAL);
        LOG(LVL_ERROR, "Event : ROUTE CHANGE, Error adding task.");
    }
}

/**
 * \brief Task called when a timeout expires. Raises the timeout event.
 */
static uint32_t timeout_task(void)
{
    LOG(LVL_INFO, "Event : TIMEOUT.");
    m_events.timeout = 1;
    if (App_Scheduler_addTask_execTime(run_state_machine,
                              APP_SCHEDULER_SCHEDULE_ASAP,
                              500) != APP_SCHEDULER_RES_OK)
    {
        reset_joining(true);
        m_conf.end_cb(PROV_RES_ERROR_INTERNAL);
        LOG(LVL_ERROR, "Event : TIMEOUT, Error adding task.");
    }

    return APP_SCHEDULER_STOP_TASK;
}

/**
 * \brief Resets the joining state machine variables and tasks.
 * \param stopJoining
 *        If true stops the joining process.
 */
static void reset_joining(bool stopJoining)
{
    lib_joining->stopJoiningBeaconRx();
    if (stopJoining)
    {
        lib_joining->stopJoiningProcess();
    }
    App_Scheduler_cancelTask(timeout_task);
    Stack_State_removeEventCb(route_changed_cb);
    m_state = JOIN_STATE_IDLE;
    memset(&m_events,0,sizeof(m_events));
}

/**
 * \brief   The Idle state function.
 * \return  Time in ms to schedule the state machine again.
 */
static uint32_t state_idle(void)
{
    uint32_t new_delay_ms = APP_SCHEDULER_STOP_TASK;

    if (m_events.start)
    {
        m_state = JOIN_STATE_START;
        m_retry = 0;
        new_delay_ms = APP_SCHEDULER_SCHEDULE_ASAP;
    }

    App_Scheduler_cancelTask(timeout_task);
    memset(&m_events,0,sizeof(m_events));

    return new_delay_ms;
}

/**
 * \brief   The Start state function.
 * \return  Time in ms to schedule the state machine again.
 */
static uint32_t state_start(void)
{
    uint32_t new_delay_ms = APP_SCHEDULER_STOP_TASK;

    LOG(LVL_INFO, "State START : Scan for joining beacons.");

    app_res_e res;
    app_lib_joining_beacon_rx_param_t param =
    {
        .cb = joining_beacon_rx_cb,
        .max_exec_time_us = JOINING_CB_EXEC_TIME_US,
        .addr = JOINING_NETWORK_ADDRESS,
        .channel = JOINING_NETWORK_CHANNEL,
        .timeout = JOINING_RX_TIMEOUT,
        .max_num_beacons = MAX_NUM_RX_JOINING_BEACONS,
        .buffer = m_beacon_rx_buffer,
        .num_bytes = sizeof(m_beacon_rx_buffer)
    };

    /* Force joining library to be in a known state. */
    lib_joining->stopJoiningBeaconRx();
    /* Zeroes beacon buffer (Or m_beacon_rx_buffer[0]->next = NULL;).
     * This is needed in JOIN_STATE_WAIT_SCAN_END state to parse
     * received beacons.
     */
    memset(m_beacon_rx_buffer,0,sizeof(m_beacon_rx_buffer));
    res = lib_joining->startJoiningBeaconRx(&param);
    if (res != APP_RES_OK)
    {
        LOG(LVL_WARNING, "State START : Error starting scan (res: %u).", res);
        m_retry++;

        if (m_retry > m_conf.nb_retry)
        {
            LOG(LVL_ERROR, "State START : Too many retry; END.");
            reset_joining(true);
            m_conf.end_cb(PROV_RES_ERROR_SCANNING_BEACONS);
        }
        else
        {
            LOG(LVL_WARNING, "State START : Restart scanning in %dms.",
                                                        DELAY_RESTART_SCAN_MS);
            new_delay_ms = DELAY_RESTART_SCAN_MS;
        }
    }
    else
    {
        if (App_Scheduler_addTask_execTime(timeout_task, DELAY_WAIT_END_SCAN_MS, 500) !=
                                                        APP_SCHEDULER_RES_OK)
        {
            reset_joining(true);
            m_conf.end_cb(PROV_RES_ERROR_INTERNAL);
            LOG(LVL_ERROR, "State START : Error adding task.");
        }
        m_state = JOIN_STATE_WAIT_SCAN_END;
    }

    memset(&m_events,0,sizeof(m_events));

    return new_delay_ms;
}

/**
 * \brief   The Wait scan end state function.
 * \return  Time in ms to schedule the state machine again.
 */
static uint32_t state_wait_scan_end(void)
{
    uint32_t new_delay_ms = APP_SCHEDULER_STOP_TASK;

    provisioning_res_e res = PROV_RES_SUCCESS;

    if (m_events.scan_end)
    {
        App_Scheduler_cancelTask(timeout_task);
        memset(&m_events,0,sizeof(m_events));

        if (m_num_beacons > 0)
        {
            const app_lib_joining_received_beacon_t * beacon;

            beacon = sort_provisioning_beacons(m_beacon_rx_buffer);
            beacon = m_conf.joining_cb(beacon);

            if(beacon != NULL)
            {
                app_lib_settings_net_channel_t ch;
                app_lib_settings_net_addr_t addr;

                /* Note: There is no way to make the difference between
                 *  connected and joined to a network. The problem is if the
                 *  node is connected it will get an error when sending data on
                 *  provisioning reserved endpoints. Provisioning will fail in
                 *  send START packet (see provisioning.c) but it would be nice
                 *  to catch it early.
                 */

                /* Test if the node is already joined to the network from joining beacon.
                 * If it is the case, route change callback will never be called so skip
                 * this part
                 */
                if (lib_settings->getNetworkAddress(&addr) == APP_RES_OK &&
                    lib_settings->getNetworkChannel(&ch) == APP_RES_OK &&
                    !(addr == beacon->addr && ch == beacon->channel))
                {
                    LOG(LVL_INFO, "State WAIT_SCAN_END : "
                            "start joining process (address : %d, ch : %d).",
                            beacon->addr, beacon->channel);

                    if (lib_joining->startJoiningProcess(beacon->addr,
                                                            beacon->channel) !=
                                                                    APP_RES_OK)
                    {
                        LOG(LVL_WARNING, "State WAIT_SCAN_END : "
                                         "error starting joining process.");
                        res = PROV_RES_ERROR_JOINING;
                    }
                    else
                    {
                        // Interested by ROUTE changed event
                        Stack_State_addEventCb(route_changed_cb, 1 << APP_LIB_STATE_STACK_EVENT_ROUTE_CHANGED);
                        m_state = JOIN_STATE_WAIT_ROUTE_CHANGE;
                        if (App_Scheduler_addTask_execTime(timeout_task,
                                                  DELAY_WAIT_END_JOINING_MS,
                                                  500) != APP_SCHEDULER_RES_OK)
                        {
                            reset_joining(true);
                            m_conf.end_cb(PROV_RES_ERROR_INTERNAL);
                            LOG(LVL_ERROR, "State WAIT_SCAN_END : "
                                           "error adding task.");
                        }
                    }
                }
                else
                {
                    /* Node already joined to the selected network. */
                    LOG(LVL_WARNING, "State WAIT_SCAN_END : "
                                     "already joined to addr: %d, ch: %d.",
                                     addr, ch);
                    m_events.route_change = 1;
                    m_state = JOIN_STATE_WAIT_ROUTE_CHANGE;
                    new_delay_ms = APP_SCHEDULER_SCHEDULE_ASAP;
                    if (App_Scheduler_addTask_execTime(timeout_task,
                                              DELAY_WAIT_END_JOINING_MS,
                                              500) != APP_SCHEDULER_RES_OK)
                    {
                        reset_joining(true);
                        m_conf.end_cb(PROV_RES_ERROR_INTERNAL);
                        LOG(LVL_ERROR, "State WAIT_SCAN_END : "
                                       "error adding task.");
                    }
                }
            }
            else
            {
                LOG(LVL_WARNING,
                    "State WAIT_SCAN_END : no joining beacon found.");
                res = PROV_RES_ERROR_SCANNING_BEACONS;
            }
        }
        else
        {
            LOG(LVL_WARNING, "State WAIT_SCAN_END : no beacon found.");
            res = PROV_RES_ERROR_SCANNING_BEACONS;
        }
    }
    else if (m_events.timeout)
    {
        LOG(LVL_WARNING, "State WAIT_SCAN_END : timeout");
        res = PROV_RES_TIMEOUT;
    }

    if (res != PROV_RES_SUCCESS)
    {
        m_retry++;

        if (m_retry > m_conf.nb_retry)
        {
            LOG(LVL_ERROR, "State WAIT_SCAN_END : too many retry; END.");
            reset_joining(true);
            m_conf.end_cb(res);
        }
        else
        {
            /* Resend start. */
            LOG(LVL_WARNING,
                "State WAIT_SCAN_END : retry %d/%d, restart scan.",
                m_retry, m_conf.nb_retry);
            m_state = JOIN_STATE_START;

            new_delay_ms = APP_SCHEDULER_SCHEDULE_ASAP;
        }

        memset(&m_events,0,sizeof(m_events));
    }

    return new_delay_ms;
}

/**
 * \brief   The Wait route change state function.
 * \return  Time in ms to schedule the state machine again.
 */
static uint32_t state_wait_route_change(void)
{
    uint32_t new_delay_ms = APP_SCHEDULER_STOP_TASK;

    /* A route change event is not generated when node is already connected
     * to the cluster, so this event is not checked. Only check that we have
     * a valid next hop.
     */
    app_lib_state_route_info_t info;
    lib_state->getRouteInfo(&info);

    LOG(LVL_DEBUG, "State WAIT_ROUTE_CHANGE, route info :");
    LOG(LVL_DEBUG, " - Channel : %u", info.channel);
    LOG(LVL_DEBUG, " - Cost : %u", info.cost);
    LOG(LVL_DEBUG, " - N Hop : %u", info.next_hop);
    LOG(LVL_DEBUG, " - Sink : %u", info.sink);
    LOG(LVL_DEBUG, " - State : %u", info.state);

    if (info.state != APP_LIB_STATE_ROUTE_STATE_VALID)
    {
        /* Only clear route_change event. */
        m_events.route_change = 0;
        LOG(LVL_WARNING, "State WAIT_ROUTE_CHANGE : "
                         "next hop has no route to Sink.");
    }
    else
    {
        App_Scheduler_cancelTask(timeout_task);
        Stack_State_removeEventCb(route_changed_cb);
        LOG(LVL_INFO, "State WAIT_ROUTE_CHANGE : "
                      "network joined successfully (next_hop: %u).",
                      info.next_hop);
        reset_joining(false);
        m_conf.end_cb(PROV_RES_SUCCESS);
    }

    if(m_events.timeout)
    {
        LOG(LVL_WARNING, "State WAIT_ROUTE_CHANGE : timeout.");
        Stack_State_removeEventCb(route_changed_cb);

        m_retry++;

        if (m_retry > m_conf.nb_retry)
        {
            LOG(LVL_ERROR, "State WAIT_ROUTE_CHANGE : too many retry; END.");
            reset_joining(true);
            m_conf.end_cb(PROV_RES_ERROR_NO_ROUTE);
        }
        else
        {
            /* Resend start. */
            LOG(LVL_WARNING, "State WAIT_ROUTE_CHANGE : retry %d/%d, "
                             "restart Joining.",
                             m_retry, m_conf.nb_retry);
            m_state = JOIN_STATE_START;

            new_delay_ms = APP_SCHEDULER_SCHEDULE_ASAP;
        }
    }

    return new_delay_ms;
}

/**
 * \brief   The joining state machine. React to events raised in this module.
 * \note    This function uses the fact the system is not preemptive and that
 *          all events are raised from non interrupt context.
 * \return  Time in ms to schedule this function again.
 */
static uint32_t run_state_machine(void)
{
    uint32_t new_delay_ms = APP_SCHEDULER_STOP_TASK;

    LOG(LVL_DEBUG, "Enter state machine :");
    LOG(LVL_DEBUG, " - state : %d",m_state);
    LOG(LVL_DEBUG, " - events : %s%s%s%s",
                   m_events.timeout ? " timeout" : "",
                   m_events.start ? " start" : "",
                   m_events.scan_end ? " scan end" : "",
                   m_events.route_change ? " route change" : "");

    switch (m_state)
    {
        case JOIN_STATE_IDLE:
        {
            new_delay_ms = state_idle();
            break;
        }

        case JOIN_STATE_START:
        {
            new_delay_ms = state_start();
            break;
        }

        case JOIN_STATE_WAIT_SCAN_END:
        {
            new_delay_ms = state_wait_scan_end();
            break;
        }

        case JOIN_STATE_WAIT_ROUTE_CHANGE:
        {
            new_delay_ms = state_wait_route_change();
            break;
        }

        default:
        {
            break;
        }
    }

    return new_delay_ms;
}

provisioning_ret_e Provisioning_Joining_init(
                                        provisioning_joining_conf_t * conf)
{
    LOG(LVL_DEBUG, "Init configuration :");
    LOG(LVL_DEBUG, " - Nb Retries : %d", conf->nb_retry);

    if (m_state != JOIN_STATE_UNINIT && m_state != JOIN_STATE_IDLE)
    {
        LOG(LVL_ERROR, "%s : PROV_RET_INVALID_STATE, (state:%d).",
                       __func__, m_state);
        return PROV_RET_INVALID_STATE;
    }

    if (conf->joining_cb == NULL ||
        conf->end_cb == NULL)
    {
        LOG(LVL_ERROR, "%s : PROV_RET_INVALID_PARAM.", __func__);
        return PROV_RET_INVALID_PARAM;
    }

    memcpy(&m_conf,conf,sizeof(m_conf));
    m_state = JOIN_STATE_IDLE;

    return PROV_RET_OK;
}

provisioning_ret_e Provisioning_Joining_start()
{
    if (m_state != JOIN_STATE_IDLE)
    {
        LOG(LVL_ERROR, "%s : PROV_RET_INVALID_STATE.", __func__);
        return PROV_RET_INVALID_STATE;
    }

    m_events.start = 1;
    if (App_Scheduler_addTask_execTime(run_state_machine,
                              APP_SCHEDULER_SCHEDULE_ASAP,
                              500) != APP_SCHEDULER_RES_OK)
    {
        reset_joining(true);
        m_conf.end_cb(PROV_RES_ERROR_INTERNAL);
        LOG(LVL_ERROR, "%s : Error adding task.", __func__);
        return PROV_RET_INTERNAL_ERROR;
    }
    else
    {
        LOG(LVL_INFO, "Event : START.");
        return PROV_RET_OK;
    }
}

provisioning_ret_e Provisioning_Joining_stop()
{
    /* Special case for stop event. Whatever the state is, stop
     * the provisioning session.
     */
    reset_joining(true);
    LOG(LVL_INFO, "Joining stopped.");
    m_conf.end_cb(PROV_RES_STOPPED);

    return PROV_RET_OK;
}