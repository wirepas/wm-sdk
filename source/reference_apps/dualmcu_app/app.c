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

#include "local_provisioning.h"
#include "dualmcu_lib.h"


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

    Dualmcu_lib_init(UART_BAUDRATE, UART_FLOWCONTROL);
}
