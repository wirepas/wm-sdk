/**
    @file scheduler.c
    @brief Handles the application state.
    @copyright Wirepas Oy 2019
*/
#include "app_settings.h"
#include "positioning_app.h"
#include "scheduler.h"
#include "ble_beacon.h"
#include "app_scheduler.h"

#define EXECUTION_TIME_US   1000
#define SEC_TO_US 1e6
#define SEC_TO_MS 1e3

static uint32_t m_message_sequence_id;
static scheduler_info_t m_info;

static void Scheduler_disable_acquisition(void);


/**
    @brief      Acts on the current state.

            This is a periodic function whose goal is to evaluate the current
            state of the application and act upon a deadline imposed by each
            state.

            When a deadline occurs (eg, a timeout is met), the scheduler will
            either go to sleep or force a new scan (depends on the application
            mode).

    @param[in]  lib_state  The library state

    @return     Amount of time until the next run should occur.
*/
uint32_t Scheduler_loop(const app_lib_state_t* lib_state)
{
    bool app_config_status = false;
    uint8_t rc = 0;
    uint32_t exec_time = 0;
    uint32_t time_to_next_tick = 0;
    app_lib_time_timestamp_hp_t now = 0;
    positioning_settings_t* app_settings = Pos_get_settings();

    lib_system->enterCriticalSection();
    Scheduler_increment_time();
    switch (Scheduler_get_state())
    {
        case POS_APP_STATE_IDLE:
            Scheduler_reset_time();

            //safety check; we cannot be in IDLE state for AUTOSCAN mode
            switch(app_settings->node_mode)
             {
                case POS_APP_MODE_AUTOSCAN_TAG:
                case POS_APP_MODE_AUTOSCAN_ANCHOR:
                    Pos_init(false);
                    break;
                default:
                    break;
             }

            break;

        case POS_APP_STATE_WAIT_FOR_CONNECTION:
            Scheduler_set_deadline(app_settings->timeout_connection);
            break;

        case POS_APP_STATE_WAIT_FOR_PACKET_ACK:
            Scheduler_set_deadline(app_settings->timeout_ack);
            Scheduler_disable_acquisition();
            break;

        case POS_APP_STATE_WAIT_FOR_APP_CONFIG:
            Scheduler_set_deadline(app_settings->timeout_connection);

            if( Scheduler_get_appconfig_reception()
                    && Scheduler_get_message_dispatch())
            {
                Scheduler_set_state(POS_APP_STATE_DOWNTIME);
                Scheduler_set_next(APP_SCHEDULER_SCHEDULE_ASAP);
            }
            break;

        case POS_APP_STATE_REBOOT:
            Scheduler_reset_time();
            lib_state->stopStack(); // reboots the device
            break;

        case POS_APP_STATE_PERFORM_SCAN:
            app_config_status = Scheduler_get_appconfig_reception();
            Pos_init(false);
            Scheduler_set_appconfig_reception(app_config_status);
            lib_state->startScanNbors();
            break;

        case POS_APP_STATE_DOWNTIME:
            Ble_Beacon_check_monitoring(MON_CHECK);
            Scheduler_reset_time();
            Scheduler_disable_acquisition();

            // calculates elapsed time and adjust the scan period
            now = lib_time->getTimestampHp();
            exec_time = lib_time->getTimeDiffUs(now, m_info.time_since_start) / SEC_TO_US;


            if(app_settings->scan_period > exec_time)  // result shall be always > 0
            {
                time_to_next_tick = app_settings->scan_period - exec_time;
            }
            else if(app_settings->scan_period > 0)
            {
                time_to_next_tick = app_settings->scan_period;
            }
            else
            {
                time_to_next_tick = CONF_SCAN_PERIOD_S; // for safety only
            }


            switch(app_settings->node_mode)
            {
                case POS_APP_MODE_NRLS_TAG:
                    rc = lib_sleep->sleepStackforTime(time_to_next_tick, 0);
                    if (rc == APP_RES_OK)
                    {
                        Scheduler_set_state(POS_APP_STATE_IDLE);
                        Scheduler_set_next(APP_SCHEDULER_STOP_TASK);
                    }
                    else
                    {
                        Scheduler_set_state(POS_APP_STATE_IDLE);
                    }

                    break;

                case POS_APP_MODE_AUTOSCAN_TAG:
                case POS_APP_MODE_AUTOSCAN_ANCHOR:
                    // reset the callback and triggers the scan
                    // when the scan is disabled, check for new state occasionaly
                    if(app_settings->scan_period == 0)
                    {
                        Scheduler_set_state(POS_APP_STATE_IDLE);
                        Scheduler_set_next(SCHEDULER_PERIODIC_CHECK * SEC_TO_MS);
                    }
                    else
                    {
                        Scheduler_set_state(POS_APP_STATE_PERFORM_SCAN);
                        Scheduler_set_next(time_to_next_tick * SEC_TO_MS);
                    }
                    break;

                case POS_APP_MODE_OPPORTUNISTIC_ANCHOR:
                    // periodically checks if there is anything to do
                    Pos_init(false);
                    Scheduler_set_state(POS_APP_STATE_IDLE);
                    Scheduler_set_next(SCHEDULER_PERIODIC_CHECK * SEC_TO_MS);
                    break;
            }
    }

    lib_system->exitCriticalSection();


    // evaluate timeout and reschedule it immediately
    if (Scheduler_timeout())
    {
        Scheduler_set_state(POS_APP_STATE_DOWNTIME);
        Scheduler_set_next(APP_SCHEDULER_SCHEDULE_ASAP);

    }

    return Scheduler_get_next();
}


/**
    @brief      Resets the scheduler.

    @param[in]  initial_state   The initial state
    @param[in]  access_cycle_s  The access cycle in seconds
    @param[in]  timestamp       The timestamp
    @param[in]  on_boot         True if called after a boot sequence
*/
void Scheduler_init(scheduler_state_e initial_state,
                    uint32_t access_cycle_s,
                    app_lib_time_timestamp_hp_t timestamp,
                    bool on_boot)
{
    m_info.period = access_cycle_s;
    m_info.state = initial_state;
    m_info.state_time = access_cycle_s;
    m_info.app_config_received = false;
    m_info.message_sent = false;
    m_info.time_since_start = timestamp;
    m_info.deadline = 0xFFFFFF;
    m_info.schedule_next = access_cycle_s * SEC_TO_MS;

    if(on_boot)
    {
        Scheduler_init_message_sequence_id();
    }

}


/**
    @brief      Disables the network call callbacks and beacon callbacks
*/
static void Scheduler_disable_acquisition(void)
{
    lib_state->setOnScanNborsCb(NULL, 0); // dont care about the filter value
    lib_state->setOnBeaconCb(NULL);
}


/**
    @brief      True when current time matches or exceeds a deadline.

    @return     True if a tiemout occured
*/
bool Scheduler_timeout(void)
{
    bool flag = false;
    uint32_t state_time = m_info.state_time;
    uint8_t deadline = m_info.deadline;

    if (state_time >= deadline)
    {
        flag = true;
    }

    return flag;
}

/**
    @brief      Sets the scheduler time back to the initial period.
*/
void Scheduler_reset_time(void)
{
    m_info.state_time = m_info.period;
}

/**
    @brief      Increments the scheduler time by its period.
*/
void Scheduler_increment_time(void)
{
    m_info.state_time += m_info.period;
}

/**
    @brief      Sets the scheduler state if not already in it.

    @param[in]  next_state  The next state
*/
void Scheduler_set_state(scheduler_state_e next_state)
{

    if (m_info.state != next_state)
    {
        m_info.state_time = m_info.period;
        m_info.state = next_state;
    }
}

/**
    @brief      Sets the time until the next scheduler run.

    @param[in]  schedule_next  The schedule next
*/
void Scheduler_set_next(uint32_t schedule_next)
{
    m_info.schedule_next = schedule_next;
}

/**
    @brief      Sets the deadline the scheduler is tracking.

    @param[in]  deadline  The deadline
*/
void Scheduler_set_deadline(uint32_t deadline)
{
    m_info.deadline = deadline;
}

/**
    @brief      Sets the appconfig reception flag.

    @param[in]  flag  The flag value
*/
void Scheduler_set_appconfig_reception(bool flag)
{
    m_info.app_config_received = flag;
}

/**
    @brief      Sets the message dispatch flag.

    @param[in]  flag  The flag value
*/
void Scheduler_set_message_dispatch(bool flag)
{
    m_info.message_sent = flag;
}

/**
    @brief      Increments sent message counter.
*/
void Scheduler_increment_message_sequence_id(void)
{
    m_message_sequence_id++;
}

/**
    @brief      Initializes the message counter.
*/
void Scheduler_init_message_sequence_id(void)
{
    m_message_sequence_id = 1;
}

/**
    @brief      Getter for the message sequence counter.

    @return     Returns the number of messages sent.
*/
uint32_t Scheduler_get_message_sequence_id(void)
{
    return m_message_sequence_id;
}

/**
    @brief      Retrieves the current scheduler state.

    @return     The current scheduler state.
*/
uint32_t Scheduler_get_state(void)
{
    return m_info.state;
}

/**
    @brief      Retrieves the time until the next run.

    @return     Time in us when the next loop should run.
*/
uint32_t Scheduler_get_next(void)
{
    return m_info.schedule_next;
}

/**
    @brief      Retrieves the scheduler inner time.

    @return     The number of periods
*/
uint32_t Scheduler_get_time(void)
{
    return m_info.state_time;
}

/**
    @brief      Retrieves current deadline.

    @return     Amount of periods to elapse until a timeout should occur.
*/
uint32_t Scheduler_get_deadline(void)
{
    return m_info.schedule_next;
}

/**
    @brief      Retrieves the status of the appconfig reception.

    @return     True if an appconfig message has been received.
*/
bool Scheduler_get_appconfig_reception(void)
{
    return m_info.app_config_received;
}

/**
    @brief      Retrieves the status of the message dispatching.

    @return     True if a message was requested to be sent.
*/
bool Scheduler_get_message_dispatch(void)
{
    return m_info.message_sent;
}

/**
    @brief      Retrieves the message tracking id.

    @return     The tracking id of the last message to be sent out.

    uint32_t Scheduler_get_message_tracking_id(void)
    {
        return m_info.message_id;
    }
*/



