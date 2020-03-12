/**
    @file  app.c
    @brief  Application to acquire network data for the positioning use
            case.

            This is the main file describing the operational flow of the
            positioning application. The application consists of a state
            machine which is updated according to what is observed in each
            call back or procedure.

    @copyright  Wirepas Oy 2019
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "api.h"
#include "app_settings.h"
#include "ble_beacon.h"
#include "event.h"
#include "measurements.h"
#include "positioning_app.h"
#include "overhaul.h"
#include "power.h"
#include "app_scheduler.h"


/**
    @brief      Wrapper for the periodic task callback.

    @return     number of microseconds until the next call should happen
*/
static uint32_t cb_periodic(void)
{
    return Scheduler_loop(lib_state);
}

/**
    @brief      Wrapper for a new beacon reception callback

    @param[in]  beacon  The network beacon
*/
static void cb_beacon_reception(const app_lib_state_beacon_rx_t* beacon)
{
    scheduler_state_e next_state;
    next_state = Event_beacon_reception(beacon);
    Scheduler_set_state(next_state);
}


/**
    @brief      Wrapper for the network scan end callback
*/
static void cb_network_scan_end(void)
{
    scheduler_state_e next_state;
    next_state = Event_network_scan_end();
    Scheduler_set_state(next_state);
}


/**
    @brief      Wrapper for reception of a data sent status callback

    @param[in]  status  The status
*/
static void cb_data_ack(const app_lib_data_sent_status_t* status)
{
    scheduler_state_e next_state = Scheduler_get_state();
    next_state = Event_data_ack(status);
    Scheduler_set_state(next_state);
}


/**
    @brief      Wrapper for the reception of an appconfig

    @param[in]  bytes     The appconfig bytes
    @param[in]  seq       The appconfig sequence number
    @param[in]  interval  The appconfig interval rate (in seconds)
*/
static void cb_app_config(const uint8_t* bytes, uint8_t seq, uint16_t interval)
{
    scheduler_state_e next_state;
    Scheduler_set_appconfig_reception(true);
    next_state = Event_app_config(bytes,
                                  lib_data->getAppConfigNumBytes(),
                                  seq);
    Scheduler_set_state(next_state);
}


/**
    @brief      Wrapper for wakeup callback
*/
static void cb_wakeup(void)
{
    bool on_boot = false;
    Pos_init(on_boot);
}


/**
    @brief      Ensures the callback behaviour is set according to the device
               method.

    @param[in]  on_boot  On boot
*/
void Pos_init(bool on_boot)
{

    scheduler_state_e initial_state = POS_APP_STATE_IDLE;
    positioning_settings_t* app_settings = Pos_get_settings();
    if(on_boot)
    {
        lib_sleep->setOnWakeupCb(cb_wakeup);
        lib_data->setDataSentCb(cb_data_ack);
        lib_data->setNewAppConfigCb(cb_app_config);
        App_Scheduler_init();
    }

    // (re) enable callbacks
    lib_state->setOnBeaconCb(cb_beacon_reception);
    lib_state->setOnScanNborsCb(cb_network_scan_end,
                                app_settings->scan_filter);

    // NRLS vs NON-NRLS
    switch(app_settings->node_mode)
    {
        case POS_APP_MODE_NRLS_TAG:
            initial_state = POS_APP_STATE_IDLE;
            break;

        case POS_APP_MODE_AUTOSCAN_TAG:
        case POS_APP_MODE_AUTOSCAN_ANCHOR:
            initial_state = POS_APP_STATE_PERFORM_SCAN;
            break;

        case POS_APP_MODE_OPPORTUNISTIC_ANCHOR:
            initial_state = POS_APP_STATE_IDLE;
            break;
    }

    // (re)initialize the modules
    Overhaul_init();
    Scheduler_init(initial_state,
                   app_settings->access_cycle,
                   lib_time->getTimestampHp(),
                   on_boot);

    Measurements_table_reset();

    // Beacon(s) configration(s) need to be done after stack is running.
    Ble_Beacon_set_configuration();

    // Beacon(s) monitoring when connection is offline
    Ble_Beacon_check_monitoring(MON_CHECK);

    // restart machine
    App_Scheduler_addTask(cb_periodic, Scheduler_get_next());
}

/**
    @brief  Initialization callback for application.

            This function is called after hardware has been initialized but
            the stack is not yet running.

    @param[in]  functions  WM functions
*/
void App_init(const app_global_functions_t* functions)
{
    bool on_boot = true;


    Pos_settings_init(on_boot);
    App_Settings_configureNode(on_boot);
    App_Settings_initBleBeacon();
    Pos_init(on_boot);

    // start the stack
    lib_state->startStack();
}
