/* Copyright 2019 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/**
 * \file wms_sleep.h
 *
 * The Non-Router Long Sleep (NRLS) Library is used to sleep Wirepas Mesh stack
 * for time periods. Once waking-up from the sleep, Wirepas Mesh stack wakes up
 * from the sleep without system reset. During Wirepas Mesh stack sleep,
 * Single-MCU application can be running using the libraries which does not have
 * dependency to Wirepas Mesh stack functionalities. Before entering to NRLS
 * sleep, Wirepas Mesh stack needs to be running.
 *
 * Non-Router Long Sleep libraries can be used with Wirepas Mesh stack which is
 * configured to be running as Non-Router mode without any role extensions
 * (@ref APP_LIB_SETTINGS_ROLE_SUBNODE in @ref app_lib_settings_base_role_e and
 * @ref APP_LIB_SETTINGS_ROLE_FLAG_RESV in @ref app_lib_settings_flag_role_e).
 * Any other @ref app_lib_settings_base_role_e and @ref
 * app_lib_settings_flag_role_e definitions do not allow to use NRLS
 * functionality.
 *
 * Library services are accessed via @ref app_lib_sleep_t "lib_sleep"
 * handle.
 *
 */
#ifndef APP_LIB_SLEEP_H_
#define APP_LIB_SLEEP_H_

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "wms_app.h"

/** @brief Library symbolic name */
#define APP_LIB_LONGSLEEP_NAME    0x02f20818 //!< "SLEEP"

/** @brief Maximum supported library version */
#define APP_LIB_LONGSLEEP_VERSION 0x203

/**
 * @brief   Stack state flags
 */
typedef enum
{
    // @brief No sleep ongoing
    APP_LIB_SLEEP_STOPPED = 0,
    // @brief Sleep is started
    APP_LIB_SLEEP_STARTED,
    // @brief Sleep is pending, i.e. requested but device has not yet
    // disconnected from the network
    APP_LIB_SLEEP_PENDING,
} app_lib_sleep_stack_state_e;

/**
 * @brief   Start stack sleeping period for a given time
 * @param   seconds
 *          Wake up time. Device will wake up after this many seconds. Sleep
 *          starts when node has disconnected from Mesh network.
 *          If set to 0 - there is @ref APP_RES_INVALID_VALUE response and
 *          sleep is not started.
 * @param   appconf_wait_s
 *          Maximum wait time in seconds to wait for app config data from
 *          network before NRLS stack sleep. If device has already acquired
 *          app config, this argument has no impact.
 *          * 0 = App config is not awaited before going to sleep.
 *          * Other values = Time used to wait that app config data is received
 *            from network. Minimum allowed value is 4 seconds and maximum 600
 *            seconds.
 * @return  @ref APP_RES_INVALID_CONFIGURATION if node role is not correctly
 *          configured. Returns @ref APP_RES_INVALID_VALUE if @p seconds exceeds
 *          the maximum value or @p appconf_wait_s is illegal value. Returns
 *          @ref APP_RES_INVALID_STACK_STATE if stack is not started and node
 *          role and seconds is correct. Otherwisereturns @ref APP_RES_OK when
 *          sleep process is started.
 * @note    If the time when waking up from NRLS sleep and going back to NRLS
 *          sleep is very short and app config is used to signal to the network
 *          e.g. overhaul state, good practice is to have \p appconf_wait_s
 *          long enough (minimum 4 seconds) to make sure that app config data is
 *          received before going to NRLS sleep
 * @note    Sleep starts when device has disconnected to network. In addition
 *          to appconfig wait (if desired by @p appconf_wait_s), there may be
 *          additional delay up to 40 seconds before sleep actually starts. If
 *          device has not yet connected to network, this is not happening.
 */
typedef app_res_e (*app_lib_sleep_stack_for_time_f)(uint32_t seconds,
                                                    uint32_t appconf_wait_s);

/**
 * @brief   Wakeup stack from sleep.
 *
 * Starts the stack from sleep if application wants to
 * wake-up the stack from NRLS sleep before NRLS sleep time has
 * elapsed.
 *
 * @return  @ref APP_RES_OK when stack wakeup is started from sleep. Returns
 *          @ref APP_RES_INVALID_STACK_STATE if wakeup called when stack is
 *          running.
 * @note    If sleep process has started but sleep is not yet started (i.e.
 *          service @ref app_lib_sleep_stack_for_time_f
 *          "lib_sleep->sleepStackforTime()" has called but device has not yet
 *          disconnected from the network, wakeup occurs immediately after
 *          disconnction of network has happened.
 */
typedef app_res_e (*app_lib_sleep_wakeup_stack_f)(void);

/**
 * @brief   Get sleep state of stack
 * @return  State of the sleep operation
 */
typedef app_lib_sleep_stack_state_e (*app_lib_sleep_get_sleep_state_f)(void);

/**
 * @brief   Returns remaining sleep time in seconds
 *
 * @return  Remaining sleep time in seconds. 0 if no sleep ongoing.
 */
typedef uint32_t (*app_lib_sleep_get_wakeup_f)(void);

/**
 * @brief   Callback called before stack wakes up
 *
 * Used with @ref app_lib_sleep_wakeup_callback_f "lib_sleep->setOnWakeupCb()"
 * service
 */
typedef void (*applib_wakeup_callback_f)(void);

/**
 * @brief   Set callback function to be called before stack wakes up
 *
 * @param   callback
 *          The callback function to set
 */
typedef
    void (*app_lib_sleep_wakeup_callback_f)(applib_wakeup_callback_f callback);

/**
 * @brief   Get latest time used before stack enters to sleep after NRLS sleep
 *          request given
 *
 * Returns time in seconds which was used in previous NRLS sleep request
 * starting from application request @ref app_lib_sleep_stack_for_time_f
 * "lib_sleep->sleepStackforTime()" until stack enters to NRLS sleep (i.e.
 * device has disconnected from the network). This value may be used to estimate
 * needed parameter configuration for appconf_wait_s in @ref
 * app_lib_sleep_stack_for_time_f "lib_sleep->sleepStackforTime()" function.
 *
 * @return  Total time used including application callback during that period in
 *          seconds
 */
typedef uint32_t (*app_lib_sleep_get_gotosleep)(void);

/**
 * @brief   Callback called when stack starts sleep
 *
 * Used with @ref app_lib_sleep_sleep_callback_f "lib_sleep->setOnsleepCb()"
 * service
 */
typedef void (*applib_on_sleep_callback_f)(void);

/**
 * @brief   Set callback function to be called when stack starts sleep
 *
 * @param   callback
 *          The callback function to set
 */
typedef
void (*app_lib_sleep_on_sleep_callback_f)(applib_on_sleep_callback_f callback);



/**
 * @brief       List of library functions
 */
typedef struct
{
    app_lib_sleep_stack_for_time_f          sleepStackforTime;
    app_lib_sleep_wakeup_stack_f            wakeupStack;
    app_lib_sleep_get_sleep_state_f         getSleepState;
    app_lib_sleep_get_wakeup_f              getStackWakeup;
    app_lib_sleep_wakeup_callback_f         setOnWakeupCb;
    app_lib_sleep_get_gotosleep             getSleepLatestGotosleep;
    app_lib_sleep_on_sleep_callback_f       setOnSleepCb;
} app_lib_sleep_t;

#endif /* APP_LIB_SLEEP_H_ */
