/* Copyright 2021 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/*
 * \file    app.c
 * \brief   This file is an example on how to use the application
 *          persistent library and test it in trivial cases.
 *             - Read to see if content is valid
 *             - Write
 *             - Read it back
 *             - Check content
 */

#include <stdlib.h>
#include <string.h>

#include "api.h"
#include "node_configuration.h"
#include "led.h"
#include "app_persistent.h"

#define DEBUG_LOG_MODULE_NAME "APP_PERS"
#define DEBUG_LOG_MAX_LEVEL LVL_INFO
#include "debug_log.h"

static char my_data[500];

#define PATTERN_BYTE 0xAB

void App_init(const app_global_functions_t * functions)
{
    app_persistent_res_e res;

    // Basic configuration of the node with a unique node address
    if (configureNodeFromBuildParameters() != APP_RES_OK)
    {
        // Could not configure the node
        // It should not happen except if one of the config value is invalid
        return;
    }

    LOG_INIT();

    Led_set(0, true);

    LOG(LVL_INFO, "App_persistent example started\n");

    res = App_Persistent_read((uint8_t *) my_data, sizeof(my_data));
    if (res == APP_PERSISTENT_RES_INVALID_CONTENT)
    {
        LOG(LVL_INFO, "First initialization\n");
    }
    else if (res == APP_PERSISTENT_RES_UNINITIALIZED)
    {
        LOG(LVL_ERROR, "Persistent unitialized => Data area not present\n");
    }

    // Fill our data with PATTERN_BYTE
    memset(my_data, PATTERN_BYTE, sizeof(my_data));

    res = App_Persistent_write((uint8_t *) my_data, sizeof(my_data));
    if (res != APP_PERSISTENT_RES_OK)
    {
        LOG(LVL_ERROR, "Cannot write\n");
    }

    // Reset our data in Ram
    memset(my_data, 0, sizeof(my_data));

    // Read it back
    res = App_Persistent_read((uint8_t *) my_data, sizeof(my_data));
    if (res != APP_PERSISTENT_RES_OK)
    {
        LOG(LVL_ERROR, "Cannot read back %d\n", res);
    }

    // Check data
    for (uint16_t i = 0; i < sizeof(my_data); i++)
    {
        if (my_data[i] != PATTERN_BYTE)
        {
            LOG(LVL_ERROR, "Data at index %d is wrong\n", i);
        }
    }

    LOG(LVL_INFO, "After Read back: %x\n", my_data[0]);

    // Start the stack
    lib_state->startStack();
}
