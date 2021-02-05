/**
 * @file    app.c
 * @brief   Application to acquire network data for the positioning use
 *          case.
 *
 *          This is the main file describing the operational flow of the
 *          positioning application. The application consists of a state
 *          machine which is updated according to what is observed in each
 *          call back or procedure.
 *
 * @copyright  Wirepas Ltd. 2020
 */

#define DEBUG_LOG_MODULE_NAME "POSAPP"
#ifdef DEBUG_POSLIB_LOG_MAX_LEVEL
#define DEBUG_LOG_MAX_LEVEL DEBUG_POSLIB_LOG_MAX_LEVEL
#else
#define DEBUG_LOG_MAX_LEVEL LVL_NOLOG
#endif
#include "debug_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "api.h"
#include "bitfield.h"
#include "app_scheduler.h"
#include "random.h"
#include "poslib.h"
#include "shared_neighbors.h"
#include "shared_shutdown.h"
#include "shared_data.h"
#include "shared_appconfig.h"
#include "shared_offline.h"
#include "shared_beacon.h"
#include "posapp_settings.h"
#include "power.h"
#include "button.h"
#include "led.h"

/** Appconfig TLV LED id control type, check for own network use */
#define APP_TLV_LED_TYPE                0x42

/**
 *  In this example led can be controled according to received configuration data.
 *  Configuration data structure is application dependent, in this example it
 *  is structured as followed:
 *      A sequence number
 *      A led id to control period
 *      A led control - 1 to set on - 0 to set off
 * */
typedef struct
{
   /** app specific sequence number */
   uint8_t seq;
   /** led id which to control */
   uint8_t led_id;
   /** led control - 1 to switch on led - 0 switch off led */
   uint8_t led_control;
} ledcontrol_app_config_t;

/** used to share led id to led control task */
static uint8_t m_led_id;
/** used to share led control to led control task */
static uint8_t m_led_control;
/** Filter id */
static uint16_t m_filter_id;
/** Registered id shared offline */
static uint8_t m_shared_offline_id;
/** Status if already registered */
static bool m_shared_offline_registered;

/** Period used in app if PosLib is not set to NRLS and app wants to sleep */
#define PERIOD_TO_SEND_DATA_S   75

/** Maximum time after stack is in online to send message to stack */
#define PRE_SENT_WINDOW_S       40

/** This is an example data sending when both PosLib and application
 *  are using shared_offline (stack nrls) sleep. Example data is sent to
 *  router and offline state (sleep) is requested when ack is received to
 *  message sent.
 */
#ifdef POSLIBAPP_SLEEP_DATA_SEND_EXAMPLE

/** Example endpoint used with test message */
#define POS_APP_EXAMPLE_ENDPOINT    10

/** Sequence to test data sent out */
static uint8_t m_seq = 0;

/** Test data where is added sequence id when sent out */
static uint8_t test_data[9] =
{
    'T', 'e', 's','t','-','i','d',':'
};

/**
 * @brief   Task controlling shared offline library to enter sleep.
 *          This task is scheduled when app is ready to enter to sleep.
 */
static uint32_t App_offline(void);

/**
 * @brief   Wrapper for reception of a data sent status callback
 * @param   status  The status
 */
static void app_cb_data_ack(const app_lib_data_sent_status_t * status)
{
    LOG(LVL_INFO, "");
    App_Scheduler_addTask_execTime(App_offline, 10*1000, 100);

}

/**
 * @brief   Used to send an example payload to router. There is attached to
 *          payload sequence id to follow messages in backend.
 */
static void send_example_data(void)
{
    app_lib_data_to_send_t payload;
    app_lib_data_send_res_e rc;

    /** add payload to be sent */
    test_data[8] = m_seq++;
    payload.bytes = test_data;
    payload.num_bytes = 9;
    payload.dest_address = APP_ADDR_ANYSINK;
    payload.src_endpoint = POS_APP_EXAMPLE_ENDPOINT;
    payload.dest_endpoint = POS_APP_EXAMPLE_ENDPOINT;
    payload.qos = APP_LIB_DATA_QOS_NORMAL;
    payload.delay = 0;
    payload.flags = APP_LIB_DATA_SEND_FLAG_TRACK;
    payload.tracking_id = m_seq;

    rc = Shared_Data_sendData(&payload, app_cb_data_ack);

    if (rc != APP_LIB_DATA_SEND_RES_SUCCESS)
    {
        LOG(LVL_ERROR, "\"failed_data_send\":%d - m_seq: %d", rc, m_seq)
    }
    else
    {
        LOG(LVL_DEBUG, "\"success_data_send\":%d - m_seq: %d", rc, m_seq)
    }
}
#endif /** POSLIBAPP_SLEEP_DATA_SEND_EXAMPLE */

static uint32_t App_offline(void)
{
    poslib_settings_t settings;
    LOG(LVL_INFO, "App_offline-offline");

    PosLib_getConfig(&settings);

    /** Update period is used if PosLib configured to use NRLS - shared offline */
    if (settings.node_mode == POSLIB_MODE_NRLS_TAG &&
        settings.update_period_static_s != 0)
    {
        Shared_Offline_enter_offline_state(m_shared_offline_id,
                                           settings.update_period_static_s);

    }

    return APP_SCHEDULER_STOP_TASK;
}

/**
 * @brief   Shared offline library callback when entering to offline.
 *          Starts backup task if application does not enter to sleep.
 * @param   delay_s
 *          Delay the node will stay offline if none of the registered module
 *          ask to enter online state asynchronously
 */
static void on_online_cb(uint32_t delay_from_deadline_s)
{
    LOG(LVL_INFO, "App: callback on_online, deadline in for %d",
        delay_from_deadline_s);

    if (delay_from_deadline_s < PRE_SENT_WINDOW_S)
    {
#ifdef POSLIBAPP_SLEEP_DATA_SEND_EXAMPLE
        // We are close enough to our deadline to send our data
        LOG(LVL_INFO, "App: Sending packet");
        send_example_data();
#else
        App_Scheduler_addTask_execTime(App_offline, 1*1000, 100);
#endif

    }
    else
    {
        poslib_settings_t settings;

        PosLib_getConfig(&settings);

        /** Update period is used if PosLib configured to use NRLS - shared offline */
        if (settings.node_mode == POSLIB_MODE_NRLS_TAG &&
            settings.update_period_static_s != 0)
        {
            Shared_Offline_enter_offline_state(m_shared_offline_id,
                                               settings.update_period_static_s);
        }
    }
}

/**
 * @brief   Shared offline library callback when entering to online.
 *          Cancels back task execution as sleep is activated.
 * @param   delay_from_deadline_s
 *          Delay in second per module relative to the initially requested time.
 */
static void on_offline_cb(uint32_t delay_s)
{
    LOG(LVL_INFO, "App: callback on_offline for %d", delay_s);
}

/** @brief  Shared_offline callbacks */
static offline_setting_conf_t m_shared_offline_cbs =
{
    .on_offline_event = on_offline_cb,
    .on_online_event = on_online_cb
};

/**
 * @brief   Task controlling hal led interface based on receive appconfig
 *          control.
 */
#ifdef HAL_LED
static uint32_t App_led_set()
{
    led_res_e led_set_resp = 0;

    led_set_resp = Led_set(m_led_id, m_led_control);

    if (led_set_resp != LED_RES_OK)
    {
        LOG(LVL_INFO, "Led set failed with led_id:  %d", m_led_id)
    }

    return APP_SCHEDULER_STOP_TASK;
}
#endif

/**
 * @brief   Example led control with control data received from app_config.
 *          F67E01 APP_TLV_LED_TYPE 03010101 example shared appconfig
 *          for control led.
 *          seq in ledcontrol_app_config_t not used in this led control example.
 * @param   types     The shared appconfig type
 * @param   length    The shared appconfig length
 * @param   bytes     The appconfig data
 */
static void App_appconfig_received(uint16_t types,
                                   uint8_t length,
                                   uint8_t * bytes)
{
    ledcontrol_app_config_t * config;

    if (length == sizeof(ledcontrol_app_config_t))
    {
        config = (ledcontrol_app_config_t *) bytes;
        m_led_id = config->led_id;
        m_led_control = config->led_control;
#ifdef HAL_LED
        /** Own task for led control */
        App_Scheduler_addTask(App_led_set, APP_SCHEDULER_SCHEDULE_ASAP);
#endif
    }
}

/**
 * @brief   Example function used with PosLib_oneshot().
 *          This function called from HAL button as callback when requested
 *          button is pressed.
 */
#ifdef HAL_BUTTON
static void App_button_pressed(uint8_t button_id, button_event_e event)
{
    PosLib_oneshot();
}
#endif

/**
 * @brief   Example function used with PosLib_event_setCb
 */
void App_posLib_event(poslib_events_info_t * msg)
{
    LOG(LVL_INFO, "App_posLib_event: %d, time: %u",
            msg->event_id,
            msg->time_hp)

    if (msg->event_id == POSLIB_APPCONFIG_NEW_SETTINGS)
    {
        poslib_settings_t settings;

        PosLib_getConfig(&settings);

        /** Registration is needed if PosLib supports NRLS and app not yet done that */
        if (!m_shared_offline_registered && settings.node_mode == POSLIB_MODE_NRLS_TAG)
        {
            if (Shared_Offline_register(&m_shared_offline_id, m_shared_offline_cbs) !=
                    SHARED_OFFLINE_RES_OK)
            {
                LOG(LVL_ERROR, "Cannot register PosApp shared offline module");
            }
            else
            {
                LOG(LVL_DEBUG, "Register PosApp shared offline module");
                m_shared_offline_registered = true;
            }
        }
        /** When configuration with non nrls PosLib mode there is needed to unregister */
        else if (m_shared_offline_registered && settings.node_mode != POSLIB_MODE_NRLS_TAG)
        {
            if (Shared_Offline_unregister(m_shared_offline_id) != SHARED_OFFLINE_RES_OK)
            {
                LOG(LVL_ERROR, "Cannot unregister PosApp shared offline module");
            }
            else
            {
                LOG(LVL_DEBUG, "Unregister PosApp shared offline module");
                m_shared_offline_registered = false;
            }
        }
        /** App needs to enter to offline in order PosLib to enter offline when
         *  PosLib mode changes from non-nrls to nrls mode
         */
        if(settings.node_mode == POSLIB_MODE_NRLS_TAG)
        {
            if (Shared_Offline_enter_offline_state(m_shared_offline_id,
                settings.update_period_static_s) != SHARED_OFFLINE_RES_OK)
            {
                LOG(LVL_ERROR, "Cannot enter PosApp shared offline to offline");
            }
            else
            {
                LOG(LVL_DEBUG, "PosApp enter to offline");
            }
        }


        /** if new Appconfig settings change to PosLib persistent check is needed */
#ifdef CONF_USE_PERSISTENT_MEMORY
    /** if new Appconfig settings change to PosLib persistent check is needed */
    PosApp_Settings_persistent_datacheck();
#endif
    }
}

/**
 * @brief   PosLib initialization
 */
static poslib_ret_e App_start_positioning(void)
{
    poslib_settings_t pos_settings;
    poslib_ret_e poslib_res;

    memset(&pos_settings, 0, sizeof(poslib_settings_t));

    /** Default positioning settings from config.mk */
    pos_settings.node_class = POSLIB_DEVICE_CLASS;
    pos_settings.node_mode = POSLIB_DEVICE_MODE;
    pos_settings.update_period_static_s = POSLIB_UPDATE_PERIOD_S;
    pos_settings.update_period_autoscan_offline_s = POSLIB_UPDATE_PERIOD_OFFLINE_S;
    pos_settings.update_period_dynamic_s = POSLIB_UPDATE_PERIOD_DYNAMIC_S;

    /** Default ble settings from config.mk */

    pos_settings.poslib_ble_settings.ble_type = POSLIB_BLEBEACON_SELECTION;
    pos_settings.poslib_ble_settings.ble_mode = POSLIB_BLEBEACON_SETUP;

    if (pos_settings.poslib_ble_settings.ble_type == POSLIB_EDDYSTONE ||
            pos_settings.poslib_ble_settings.ble_type == POSLIB_BEACON_ALL)
    {
        pos_settings.poslib_ble_settings.ble_eddystone.ble_channels =
                APP_LIB_BEACON_TX_CHANNELS_ALL;
        pos_settings.poslib_ble_settings.ble_eddystone.ble_tx_interval_s =
                POSLIB_BLETX_INTERVAL_S;
        pos_settings.poslib_ble_settings.ble_eddystone.ble_tx_power =
                POSLIB_BLETX_POWER;
    }

    if (pos_settings.poslib_ble_settings.ble_type == POSLIB_IBEACON ||
        pos_settings.poslib_ble_settings.ble_type == POSLIB_BEACON_ALL)
    {
        pos_settings.poslib_ble_settings.ble_ibeacon.ble_channels =
                APP_LIB_BEACON_TX_CHANNELS_ALL;
        pos_settings.poslib_ble_settings.ble_ibeacon.ble_tx_interval_s =
                POSLIB_BLETX_INTERVAL_S;
        pos_settings.poslib_ble_settings.ble_ibeacon.ble_tx_power =
                POSLIB_BLETX_POWER;
    }

    /** Stack offline information using BLE after timeout defined here.
     *  Feature enabled when period defined is other than 0 and
     *  PosLib is started.
     */
    pos_settings.poslib_ble_settings.update_period_bleon_offline_s =
        POSLIB_BLE_PERIOD_OFFLINE_S;

    /** Configuration set to library */
    poslib_res = PosLib_setConfig(&pos_settings);

    /** Start the PosLib with the requested setting.
     *  Note that the settings can be changed at run time through app-config */
    if (poslib_res == POS_RET_OK)
    {
        poslib_res = PosLib_start();
    }
    else
    {
        LOG(LVL_ERROR, "PosLib configuration fail: %d", poslib_res);
    }

    if (poslib_res == POS_RET_OK)
    {
        /** PosLib events callbacks set*/
        PosLib_event_setCb(POSLIB_UPDATE_ALL, App_posLib_event);
        PosLib_event_setCb(POSLIB_NRLS_ALL, App_posLib_event);
        PosLib_event_setCb(POSLIB_BLE_ALL, App_posLib_event);
        PosLib_event_setCb(POSLIB_APPCONFIG_NEW_SETTINGS, App_posLib_event);

        /** Start shared_offline use in application */
        if (pos_settings.node_mode == POSLIB_MODE_NRLS_TAG)
        {
            if (Shared_Offline_register(&m_shared_offline_id, m_shared_offline_cbs) !=
                SHARED_OFFLINE_RES_OK)
            {
                LOG(LVL_ERROR, "Cannot register PosApp shared offline module");
            }

            /** Own task for request to enter offline/sleep */
            App_Scheduler_addTask(App_offline, 1*1000);
        }
    }
    else
    {
        LOG(LVL_ERROR, "PosLib start fail: %d", poslib_res);
    }

    return poslib_res;
}

/**
 * @brief   Initialization callback for application.
 *          This function is called after hardware has been initialized but
 *          the stack is not yet running.
 *
 * @param   functions  WM functions
 */
void App_init(const app_global_functions_t* functions)
{
    shared_app_config_filter_t app_config_filter;

    LOG_INIT();
    LOG(LVL_DEBUG, "APP_DEBUG_LEVEL=%d", DEBUG_LOG_MAX_LEVEL);
    PosApp_Settings_configureNode();
    App_Scheduler_init();

    /** Initialization of shared libraries */
    Shared_Data_init();
    Shared_Appconfig_init();
    Shared_Neighbors_init();
    Shared_Shutdown_init();
    Shared_Beacon_init();
    Shared_Offline_init();
    m_shared_offline_registered = false;

#ifdef HAL_BUTTON
    button_res_e button_resp;
    /** See hal/button.c for how button id:es are mapped */
    uint8_t button_id = 1;
    /** Initialization of hal button services */
    Button_init();
    /** Callback for button pressed for button id defined */
    button_resp = Button_register_for_event(button_id,
                                            BUTTON_PRESSED,
                                            App_button_pressed);

    if (button_resp != BUTTON_RES_OK)
    {
        LOG(LVL_INFO, "Button_register_for_event fail w id:  %u", button_id)
    }
#endif
#ifdef HAL_LED
    /** Initialization of hal led services */
    Led_init();
#endif
    /** Initialization of shared appconfig library services */
    /** In shared appconfig library there is filtered requested appconfig */
    /** to correct receiver. PosLib library uses shared appconfig and use of */
    /** shared appconfig library is must if PosLib service is initialized */

    app_config_filter.type = APP_TLV_LED_TYPE;
    app_config_filter.cb =
            (shared_app_config_received_cb_f)App_appconfig_received;
    Shared_Appconfig_addFilter(&app_config_filter, &m_filter_id);
    App_start_positioning();
    // start the stack
    lib_state->startStack();
}
