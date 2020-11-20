/* Copyright 2017 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/*
 * \file    app.c
 * \brief   This file is a template to Dual MCU API app for all paltforms
 */
#include <stdlib.h>
#include "hal_api.h"
#include "io.h"
#include "waps.h"
#include "api.h"
#include "waps/sap/multicast.h"
#include "app_scheduler.h"

#if defined(USE_FEM)
#include "fem_driver.h"
#endif


// Interface config
const app_interface_config_s m_interface_config =
{
    .interface = APP_UART_INT,
    .baudrate = UART_BAUDRATE,
    .flow_ctrl = UART_FLOWCONTROL
};

void newAppConfigCb(const uint8_t * bytes,
                    uint8_t seq,
                    uint16_t interval)
{
    Waps_sinkUpdated(seq, bytes, interval);
}

static void dataSentCb(const app_lib_data_sent_status_t * status)
{
    Waps_packetSent(status->tracking_id,
                    status->src_endpoint,
                    status->dest_endpoint,
                    status->queue_time,
                    status->dest_address,
                    status->success);
}

void onScannedNborsCb(void)
{
    Waps_onScannedNbors();
}

/**
 * \brief   Initialization callback for application
 *
 * This function is called after hardware has been initialized but the
 * stack is not yet running.
 *
 */
void App_init(const app_global_functions_t * functions)
{
    // Open HAL
    HAL_Open();

    // Initialize Scheduler
    App_Scheduler_init();


    // Initialize IO's (enable clock and initialize pins)
    Io_init();

    // Initialize the Dual-MCU API protocol
    Waps_init();

    //register callbacks
    lib_data->setDataReceivedCb(Waps_receiveUnicast);
    lib_data->setBcastDataReceivedCb(Waps_receiveBcast);
    lib_data->setDataSentCb(dataSentCb);
    lib_data->setNewAppConfigCb(newAppConfigCb);
    lib_settings->registerGroupQuery(Multicast_isGroupCb);
    lib_state->setOnScanNborsCb(onScannedNborsCb,
                                APP_LIB_STATE_SCAN_NBORS_ONLY_REQUESTED);

}
