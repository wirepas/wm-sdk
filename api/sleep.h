/* Copyright 2019 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/**
 * \file sleep.h
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

#include "app/app.h"

/** @brief Library symbolic name */
#define APP_LIB_LONGSLEEP_NAME    0x02f20818 //!< "SLEEP"

/** @brief Maximum supported library version */
#define APP_LIB_LONGSLEEP_VERSION 0x201

/**
 * @brief   Stack state flags
 */
typedef enum
{
    APP_LIB_SLEEP_STOPPED = 0,                  //!< Sleep is stopped
    APP_LIB_SLEEP_STARTED                       //!< Sleep is started
} app_lib_sleep_stack_state_e;

/**
 * @brief   Start stack sleeping period for a given time
 * @param   seconds
 *          sleep time in seconds.
 *          Sleep time starts when the
 *          node gets disconnected from Mesh network. To disconnect from the
 *          network, some time is needed for signaling before disconnection is
 *          completed (the sleep time does not include this time needed for
 *          signaling before going to sleep). Signaling before actual stack
 *          sleep start might take time up to 30 seconds or more depending of
 *          used radio.
 *          If set to 0 - there is APP_RES_INVALID_VALUE response and sleep
 *          is not started.
 * @param   appconf_wait_s
 *          Wait time in seconds for app config data from network before NRLS
 *          stack sleep.
 *          * 0 = App config is not awaited before going to sleep.
 *          * Other values = Time used to wait that app config data is received
 *            from network.
 * @return  @ref APP_RES_INVALID_CONFIGURATION if node role is not correctly
 *          configured. Returns @ref APP_RES_INVALID_VALUE if sleep time exceeds
 *          the maximum value. Returns @ref APP_RES_INVALID_STACK_STATE if stack
 *          is not started and node role and seconds is correct. Returns @ref
 *          APP_RES_OK when stack sleep is started.
 * @note    If the time when waking up from NRLS sleep and going back to NRLS
 *          sleep is very short and app config is used to signal to the network
 *          e.g. overhaul state, good practice is to have \p appconf_wait_s
 *          long enough (minimum 4 seconds) to make sure that app config data is
 *          received before going to NRLS sleep
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
 */
typedef app_res_e (*app_lib_sleep_wakeup_stack_f)(void);

/**
 * @brief   Get sleep state of stack
 * @return  @ref APP_LIB_SLEEP_STOPPED is returned until actual NRLS sleep is
 *          started. This is happening when network disconnection ongoing before
 *          NRLS sleep is started. Signaling before actual stack sleep start
 *          might take time up to 30 seconds or more depending of used radio.
 */
typedef app_lib_sleep_stack_state_e (*app_lib_sleep_get_sleep_state_f)(void);

/**
 * @brief   Returns remaining sleep time
 *
 * Returns remaining sleep time in seconds. Time period
 * is updated every 3 second. The NRLS sleep time is not decreased while the
 * network disconnection ongoing, once network disconnection is completed, the
 * NRLS sleep time count down is started.
 *
 * @return  Remaining sleep time in seconds
 */
typedef uint32_t (*app_lib_sleep_get_wakeup_f)(void);

/**
 * @brief   Callback called before stack wakes up
 *
 * Used with @ref app_lib_sleep_wakeup_callback_f "lib_sleep->setOnWakeupCb()" service
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
 * @brief   Get latest time used before stack enters to sleep after NRLS sleep request given
 *
 * Returns time in seconds which was used in previous NRLS sleep request
 * starting from application request @ref app_lib_sleep_stack_for_time_f
 * "lib_sleep->sleepStackforTime()" until stack enters to NRLS sleep. This value
 * can be used to estimate needed parameter configuration for appconf_wait_s in
 * @ref app_lib_sleep_stack_for_time_f "lib_sleep->sleepStackforTime()" function.
 *
 * @return  Total time used including application callback during that period in
 *          seconds
 */
typedef uint32_t (*app_lib_sleep_get_gotosleep)(void);

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
} app_lib_sleep_t;

#endif /* APP_LIB_SLEEP_H_ */
