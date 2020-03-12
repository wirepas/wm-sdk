/* Copyright 2017 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

#include <stdint.h>
#include <stdbool.h>
#include "api.h"

/* All the libraries */
const app_global_functions_t *  global_func;
const app_lib_data_t *          lib_data;
const app_lib_otap_t *          lib_otap;
const app_lib_settings_t *      lib_settings;
const app_lib_state_t *         lib_state;
const app_lib_storage_t *       lib_storage;
const app_lib_system_t *        lib_system;
const app_lib_time_t *          lib_time;
const app_lib_hardware_t *      lib_hw;
const app_lib_beacon_tx_t *     lib_beacon_tx;
const app_lib_beacon_rx_t *     lib_beacon_rx;
const app_lib_advertiser_t *    lib_advertiser;
const app_lib_sleep_t *         lib_sleep;
const app_lib_memory_area_t *   lib_memory_area;
const app_lib_radio_fem_t *     lib_radio_fem;
const app_lib_joining_t *       lib_joining;
#ifdef TEST_LIB_SUPPORT
const app_lib_test_t *          lib_test;
#endif

bool API_Open(const app_global_functions_t * functions)
{
    // The root library
    global_func = functions;

    // Open the libraries
    lib_data = functions->openLibrary(APP_LIB_DATA_NAME,
                                      APP_LIB_DATA_VERSION);

    lib_settings = functions->openLibrary(APP_LIB_SETTINGS_NAME,
                                          APP_LIB_SETTINGS_VERSION);

    lib_state = functions->openLibrary(APP_LIB_STATE_NAME,
                                       APP_LIB_STATE_VERSION);

    lib_system = functions->openLibrary(APP_LIB_SYSTEM_NAME,
                                        APP_LIB_SYSTEM_VERSION);

    lib_time = functions->openLibrary(APP_LIB_TIME_NAME,
                                      APP_LIB_TIME_VERSION);

    lib_hw = functions->openLibrary(APP_LIB_HARDWARE_NAME,
                                    APP_LIB_HARDWARE_VERSION);

    lib_storage = functions->openLibrary(APP_LIB_STORAGE_NAME,
                                         APP_LIB_STORAGE_VERSION);

    lib_otap = functions->openLibrary(APP_LIB_OTAP_NAME,
                                      APP_LIB_OTAP_VERSION);

    lib_beacon_tx = functions->openLibrary(APP_LIB_BEACON_TX_NAME,
                                           APP_LIB_BEACON_TX_VERSION);

    lib_beacon_rx = functions->openLibrary(APP_LIB_BEACON_RX_NAME,
                                           APP_LIB_BEACON_RX_VERSION);

    lib_advertiser = functions->openLibrary(APP_LIB_ADVERTISER_NAME,
                                            APP_LIB_ADVERTISER_VERSION);

    lib_sleep = functions->openLibrary(APP_LIB_LONGSLEEP_NAME,
                                       APP_LIB_LONGSLEEP_VERSION);

    lib_memory_area = functions->openLibrary(APP_LIB_MEMORY_AREA_NAME,
                                             APP_LIB_MEMORY_AREA_VERSION);

    lib_radio_fem = functions->openLibrary(APP_LIB_RADIO_FEM_NAME,
                                           APP_LIB_RADIO_FEM_VERSION);

    lib_joining = functions->openLibrary(APP_LIB_JOINING_NAME,
                                         APP_LIB_JOINING_VERSION);

#ifdef TEST_LIB_SUPPORT
    lib_test = functions->openLibrary(APP_LIB_TEST_NAME,
                                         APP_LIB_TEST_VERSION);
#endif
    // Currently the status is a success if libs present on all platform are opened
    // TODO: add an api.mk / config file that tells which libraries are to be
    // opened and if a library fails to open if it should be handled as an
    // error or not
    return (lib_data &&
            lib_settings &&
            lib_state &&
            lib_system &&
            lib_time &&
            lib_hw &&
            lib_storage &&
            lib_otap &&
#ifdef TEST_LIB_SUPPORT
            lib_test &&
#endif
            lib_sleep &&
            lib_radio_fem &&
            lib_memory_area);
}
