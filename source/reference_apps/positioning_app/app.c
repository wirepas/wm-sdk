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
        LOG(LVL_INFO, "Led set failed with led_id:  %d\n", m_led_id)
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
    LOG(LVL_INFO, "App_posLib_event: %d, time: %u\n",
            msg->event_id,
            msg->time_hp)

    /** if new Appconfig settings change to PosLib persistent check is needed */
    if (msg->event_id == POSLIB_APPCONFIG_NEW_SETTINGS)
    {
        PosApp_Settings_persistent_datacheck();
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
    pos_settings.poslib_ble_settings.ble_mode = POSLIB_BLEBEACON_SETUP;
    pos_settings.poslib_ble_settings.ble_type = POSLIB_BLEBEACON_SELECTION;
    pos_settings.poslib_ble_settings.ble_eddystone.ble_channels =
        APP_LIB_BEACON_TX_CHANNELS_ALL;
    pos_settings.poslib_ble_settings.ble_eddystone.ble_tx_interval_s =
        POSLIB_BLETX_INTERVAL_S;
    pos_settings.poslib_ble_settings.ble_eddystone.ble_tx_power =
        POSLIB_BLETX_POWER;

    /** Stack offline information using BLE after timeout defined here.
     *  Feature enabled when period defined is other than 0 and
     *  PosLib is started.
     */
    pos_settings.update_period_bleon_offline_s = POSLIB_BLE_PERIOD_OFFLINE_S;

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
        LOG(LVL_ERROR, "PosLib configuration fail: %d\n", poslib_res);
    }

    if (poslib_res == POS_RET_OK)
    {
        /** PosLib events callbacks set*/
        PosLib_event_setCb(POSLIB_UPDATE_ALL, App_posLib_event);
        PosLib_event_setCb(POSLIB_NRLS_ALL, App_posLib_event);
        PosLib_event_setCb(POSLIB_BLE_ALL, App_posLib_event);
        PosLib_event_setCb(POSLIB_APPCONFIG_NEW_SETTINGS, App_posLib_event);
    }
    else
    {
        LOG(LVL_ERROR, "PosLib start fail: %d\n", poslib_res);
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
    LOG(LVL_DEBUG, "APP_DEBUG_LEVEL=%d\n", DEBUG_LOG_MAX_LEVEL);
    PosApp_Settings_configureNode();
    App_Scheduler_init();

    /** Initialization of shared libraries */
    Shared_Data_init();
    Shared_Appconfig_init();
    Shared_Neighbors_init();
    Shared_Shutdown_init();

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
        LOG(LVL_INFO, "Button_register_for_event fail w id:  %u\n", button_id)
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
