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
#include "api.h"
#include "app_scheduler.h"
#include "poslib.h"
#include "shared_neighbors.h"
#include "stack_state.h"
#include "shared_data.h"
#include "shared_appconfig.h"
#include "shared_offline.h"
#include "shared_beacon.h"
#include "posapp_settings.h"
#include "board.h"
#include "led.h"

#include "gpio.h"
#include "voltage.h"
#ifdef MOTION_SUPPORTED
#include "motion.h"
#endif
#ifdef CONF_USE_PERSISTENT_MEMORY
#include "app_persistent.h"
#endif

#ifdef CONF_USE_LED_NOTIFICATION
#define LED_ON true
#define LED_OFF false

/** used to share led id to led control task */
#define LED_ID 0
#endif

/** PosLib event subscriber ID */
static uint8_t m_event_config_change_id = POSLIB_FLAG_EVENT_SUBSCRIBERS_MAX;
static uint8_t m_event_led_on_id = POSLIB_FLAG_EVENT_SUBSCRIBERS_MAX;
static uint8_t m_event_led_off_id = POSLIB_FLAG_EVENT_SUBSCRIBERS_MAX;

/**
 * @brief   Example function used with PosLib_oneshot().
 *          This function called from HAL button as callback when requested
 *          button is pressed.
 */
#ifdef BUTTON_ENABLED
static void button_pressed_cb(uint8_t pin, gpio_event_e event)
{
    LOG(LVL_DEBUG, "Start oneshot. Button event %u button pin %u", event, pin);
    PosLib_startOneshot();
}

/**
 * @brief  Enables the button and it's callback
 */
static void enable_button(void)
{
    gpio_res_e res;
    uint8_t buttons_pins[] = BOARD_BUTTON_PIN_LIST;
    gpio_pull_e pull = BOARD_BUTTON_INTERNAL_PULL ? GPIO_PULLUP: GPIO_PULLDOWN;
    gpio_event_e event = BOARD_BUTTON_ACTIVE_LOW ? GPIO_EVENT_HL : GPIO_EVENT_LH;
    #define DEBOUNCE_MS 100  //FixME: move to general definitions
    #define BUTTON_ID 0 //FixME: move to general definitions

    res = GPIO_register_for_event(buttons_pins[BUTTON_ID], 
                                pull,
                                event,
                                DEBOUNCE_MS,
                                button_pressed_cb);
    if (res == GPIO_RES_OK)
    {
        LOG(LVL_INFO, "Oneshot request active on button %u", BUTTON_ID);
    }
    else
    {
        LOG(LVL_ERROR, "Registration for button id: %u failed.", BUTTON_ID)
    }
}
#else
static void enable_button(void) {}
#endif

/**
 * @brief   Task which is used to check need for persistent data area
 *          update after PoLib parameters are updated.
 */
#ifdef CONF_USE_PERSISTENT_MEMORY
static uint32_t App_persistent_data_write(void)
{
    poslib_settings_t settings;

    PosLib_getConfig(&settings);
    PosApp_Settings_store(&settings);
    return APP_SCHEDULER_STOP_TASK;
}
#endif

#ifdef MOTION_SUPPORTED
static void motion_cb(poslib_motion_mode_e mode)
{
    LOG(LVL_DEBUG, "Motion state: %u", mode);
    PosLib_motion(mode);
}

bool update_motion(poslib_motion_mon_settings_t * motion)
{
    if (motion->enabled)
    {
        posapp_motion_ret_e res_mon;
        posapp_motion_mon_settings_t motion_cfg = {
            .threshold_mg = motion->threshold_mg,
            .duration_ms = motion->duration_ms,
            .cb = motion_cb
        };

        res_mon = PosAppMotion_startMonitoring(&motion_cfg);

        if (res_mon != POSAPP_MOTION_RET_OK)
        {
            LOG(LVL_ERROR, "Cannot activate motion monitoring! res: %u", res_mon);
            return false;
        }
    }
    else
    {
        PosAppMotion_stopMonitoring();
    }

    return true;
}
#endif

/**
 * @brief   Example function used with POSLIB_FLAG_EVENT_setCb.
 */
static void App_posLib_event(POSLIB_FLAG_EVENT_info_t * msg)
{
    LOG(LVL_INFO, "PosLib event: %u, time: %u", msg->event_id, msg->time_hp)

    if (msg->event_id & POSLIB_FLAG_EVENT_CONFIG_CHANGE)
    {
        poslib_settings_t settings;
        PosLib_getConfig(&settings);

#ifdef MOTION_SUPPORTED
    update_motion(&settings.motion);
#endif
 
#ifdef CONF_USE_PERSISTENT_MEMORY
    /** Writing to memory takes time, do it in own task - not in callback */
    App_Scheduler_addTask_execTime(App_persistent_data_write, APP_SCHEDULER_SCHEDULE_ASAP, 500);
#endif
    }
}

/**
 * @brief   Cb Function for POSLIB_FLAG_EVENT_LED_ON event to set the led on
 */
static void App_posLib_led_on_event(POSLIB_FLAG_EVENT_info_t * msg)
{
    LOG(LVL_INFO, "PosLib led on public event. Id: %u, time: %u", msg->event_id, msg->time_hp)
#ifdef CONF_USE_LED_NOTIFICATION
    led_res_e res = Led_set(LED_ID, LED_ON);

    if (res != LED_RES_OK)
    {
        LOG(LVL_ERROR, "Led set failed with led_id:  %d", LED_ID)
    }
#endif
}

/**
 * @brief   Cb Function for POSLIB_FLAG_EVENT_LED_OFF event to set the led off
 */
static void App_posLib_led_off_event(POSLIB_FLAG_EVENT_info_t * msg)
{
    LOG(LVL_INFO, "PosLib led off public event. Id: %u, time: %u", msg->event_id, msg->time_hp)
#ifdef CONF_USE_LED_NOTIFICATION
    led_res_e res = Led_set(LED_ID, LED_OFF);

    if (res != LED_RES_OK)
    {
        LOG(LVL_ERROR, "Led set failed with led_id:  %d", LED_ID)
    }
#endif
}

/**
 * @brief   PosLib initialization
 */
static poslib_ret_e App_start_positioning(void)
{
    poslib_settings_t settings;
    poslib_ret_e res;

    /* Retrieve the default | persistent settings */

    PosApp_Settings_get(&settings);

    /** Configuration set to library */
    res = PosLib_setConfig(&settings);
    
    if (res != POS_RET_OK)
    {
        LOG(LVL_ERROR, "PosLib configuration fail. res: %d", res);
        return res;
    }

    /** PosLib events callbacks set*/
    poslib_events_e event = POSLIB_FLAG_EVENT_CONFIG_CHANGE;
    res = PosLib_eventRegister(event, App_posLib_event, &m_event_config_change_id);

    if (res != POS_RET_OK)
    {
        LOG(LVL_ERROR, "PosLib cannot register event %u failed. res: %d", event, res);
        return res;
    }

    /** Register for Led on/off events and callback set */
    poslib_events_e event_led_on = POSLIB_FLAG_EVENT_LED_ON;
    res = PosLib_eventRegister(event_led_on, App_posLib_led_on_event, &m_event_led_on_id);
    if (res != POS_RET_OK)
    {
        LOG(LVL_ERROR, "PosLib cannot register event %u failed. res: %d", event, res);
        return res;
    }
    poslib_events_e event_led_off = POSLIB_FLAG_EVENT_LED_OFF;
    res = PosLib_eventRegister(event_led_off, App_posLib_led_off_event, &m_event_led_off_id);
    if (res != POS_RET_OK)
    {
        LOG(LVL_ERROR, "PosLib cannot register event %u failed. res: %d", event, res);
        return res;
    }

    /** Update motion motitoring */
#ifdef MOTION_SUPPORTED
    update_motion(&settings.motion);
#endif

    /** Start the PosLib with the requested setting.
     *  Note that the settings can be changed at run time through app-config */

    res = PosLib_startPeriodic();

    if (res !=POS_RET_OK)
    {
        LOG(LVL_ERROR, "PosLib start periodic failed. res: %d", res);
    }

    return res;
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
    LOG_INIT();
    PosApp_Settings_configureNode();

    /** Initialization of shared libraries */
    enable_button();

#ifdef CONF_USE_LED_NOTIFICATION
    /** Initialization of hal led services */
    Led_init();
#endif

    /* Initialize motion sensor if enabled */
    #ifdef MOTION_SUPPORTED
    PosAppMotion_init();
    #endif

    #ifdef CONF_VOLTAGE_REPORT
    /* Initialize voltage measurement. */
    Mcu_voltageInit();
    #endif

    // start the stack
    lib_state->startStack();

    //Start positioning
    App_start_positioning();
}