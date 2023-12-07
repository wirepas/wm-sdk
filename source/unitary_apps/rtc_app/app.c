/* Copyright 2023 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

#include <stdlib.h>
#include <stdint.h>

#include "api.h"
#include "app_scheduler.h"
#include "node_configuration.h"
#include "shared_data.h"
#include "rtc.h"


#define DEBUG_LOG_MODULE_NAME "RTC_APP"
#define DEBUG_LOG_MAX_LEVEL LVL_DEBUG

#include "debug_log.h"


/** Endpoints of the RTC messages. */
#define RTC_SOURCE_EP (79u)
#define RTC_DEST_EP (78u)

/** Interval of time in ms between two calls of the periodic function sending RTC time. */
#define SENDING_RTC_PERIOD_MS (300000u)
/** Maximal time of execution in us of the function sending RTC time. */
#define SENDING_RTC_TIME_EXECUTION_US (100u)


/**
 * \brief   Function sending rtc time.
 * \param   timer_ms
 *          Timestamp to be sent to the network.
 * \return  Result code, \ref app_lib_data_send_res_e
 */
static app_lib_data_send_res_e sent_rtc_time(uint64_t timer_ms)
{
    /* Create a data packet to send. */
    app_lib_data_to_send_t data_to_send = {
        .bytes = (const uint8_t *) &timer_ms,
        .num_bytes = sizeof(timer_ms),
        .dest_address = APP_ADDR_ANYSINK,
        .src_endpoint = RTC_SOURCE_EP,
        .dest_endpoint = RTC_DEST_EP};

    Shared_Data_sendData(&data_to_send, NULL);

    LOG(LVL_INFO, "Expected RTC time has been sent");
    return APP_LIB_DATA_SEND_RES_SUCCESS;
}

/**
 * Function sending periodically the expected rtc time.
 * \return  Period in ms for the next execution of the function.
 */
static uint32_t send_periodical_rtc_time(void)
{
    rtc_timestamp_t rtc_ms;
    rtc_res_e res = RTC_getUTCTime(&rtc_ms);
    if (res != RTC_RES_OK)
    {
        LOG(LVL_WARNING, "An error occured when getting rtc time: %s!", res);
    }
    else
    {
        sent_rtc_time(rtc_ms);
    }
    return SENDING_RTC_PERIOD_MS;
}

/**
 * \brief   Function which adds send_periodical_rtc time to the App Scheduler.
 */
static void add_periodic_get_rtc_time_cb(void)
{
    App_Scheduler_addTask_execTime(send_periodical_rtc_time,
                                   APP_SCHEDULER_SCHEDULE_ASAP,
                                   SENDING_RTC_TIME_EXECUTION_US);
}

/**
 * \brief   Initialization callback for application
 *
 * This function is called after hardware has been initialized but the
 * stack is not yet running.
 */
void App_init(const app_global_functions_t * functions)
{
    LOG_INIT();
    LOG(LVL_INFO, "App_init");
    /* Basic configuration of the node with a unique node address */
    if (configureNodeFromBuildParameters() != APP_RES_OK)
    {
        /*
         * Could not configure the node.
         * It should not happen except if one of the config value is invalid.
         */
        return;
    }
    // We need to wait rtc time to call the periodic function sending local expectation of rtc
    RTC_addInitializeCb(add_periodic_get_rtc_time_cb);

    lib_state->startStack();
}
