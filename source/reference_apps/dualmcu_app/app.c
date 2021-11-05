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
#include "local_provisioning.h"

// Interface config
const app_interface_config_s m_interface_config =
{
    .interface = APP_UART_INT,
    .baudrate = UART_BAUDRATE,
    .flow_ctrl = UART_FLOWCONTROL
};

/**
 * \brief   Initialization callback for application
 *
 * This function is called after hardware has been initialized but the
 * stack is not yet running.
 *
 */
void App_init(const app_global_functions_t * functions)
{
    Local_provisioning_init(NULL, NULL);

    // Initialize IO's (enable clock and initialize pins)
    Io_init();

    // Initialize the Dual-MCU API protocol
    Waps_init();
}
