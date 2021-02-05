/* Copyright 2020 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/*
 * \file    app.c
 * \brief   This app is used to update the bootloader.
 *          Please read the documentation or contact Wirepas support before
 *          trying anything.
 */

#include <stdlib.h> /* For NULL */
#include <string.h> /* For memcmp(), strlen() */

#include "flash.h"
#include "api.h"
#include "led.h"

#define BOOTLOADER_SIZE_BYTES 16384
#define BL_SIZE (BOOTLOADER_SIZE_BYTES / sizeof(uint32_t))

static const char * status_message = NULL;
static const uint32_t app_status = 0xffffffff;  /* Unprogrammed */
static const uint32_t bootloader_data[BL_SIZE] __attribute__(( section (".bl_hex"))) = {
#include "bootloader_updater_data.bin"
};

static void erase_bootloader_area(size_t num_bytes)
{
    uint32_t last_page = Flash_getPage((uint32_t)(num_bytes - 1));

    /* Erase bootlaoder area */
    for (uint32_t page_base = 0;
         page_base <= last_page;
         page_base += FLASH_PAGE_SIZE_BYTES)
    {
        Flash_erasePage(page_base);
    }
}

static uint32_t sendStatus(void)
{
    if (status_message != NULL)
    {
        /* Send status message */
        // Create a data packet to send
        app_lib_data_to_send_t data_to_send;
        data_to_send.bytes = (const uint8_t *)status_message;
        data_to_send.num_bytes = strlen(status_message);
        data_to_send.dest_address = APP_ADDR_ANYSINK;
        data_to_send.src_endpoint = 0;
        data_to_send.dest_endpoint = 0;
        data_to_send.qos = APP_LIB_DATA_QOS_HIGH;
        data_to_send.delay = 0;
        data_to_send.flags = APP_LIB_DATA_SEND_FLAG_NONE;
        data_to_send.tracking_id = APP_LIB_DATA_NO_TRACKING_ID;

        // Send the data packet
        lib_data->sendData(&data_to_send);
    }

    /* No more data to send */
    return APP_LIB_SYSTEM_STOP_PERIODIC;
}

static int memcmp_null(const void *s1, const void *s2, size_t n)
{
    int res = 0;
    const uint8_t *p1 = s1;
    const uint8_t *p2 = s2;

    while (n-- > 0 && res == 0)
    {
        res = *(p1++) - *(p2++);
    }

    return res;
}

static void blink(uint32_t num_blinks, uint32_t delay, uint32_t delay_after)
{
    /* Blink LED. */
    for (; num_blinks > 0; num_blinks--)
    {
        Led_set(0, true);
        for (volatile uint32_t d = delay; d > 0; d--) {}
        Led_set(0, false);
        for (volatile uint32_t d = delay; d > 0; d--) {}
    }

    /* Delay before continuing */
    for (volatile uint32_t d = delay_after; d > 0; d--) {}
}

void upgradeBootloader(void)
{
    if (memcmp_null((void *)0, bootloader_data, BOOTLOADER_SIZE_BYTES) != 0)
    {
        /* Existing bootloader is different */
        blink(2, 2000000, 10000000);

        /* Erase old bootloader */
        erase_bootloader_area(BOOTLOADER_SIZE_BYTES);

        blink(3, 2000000, 10000000);

        /* Write new bootloader */
        Flash_write((uint32_t *)0, (void *)bootloader_data, BOOTLOADER_SIZE_BYTES);

        blink(4, 2000000, 10000000);

        /* Verify new bootloader */
        if (memcmp_null((void *)0, bootloader_data, BOOTLOADER_SIZE_BYTES) != 0)
        {
            /* Verify failed */
            blink(20, 1000000, 10000000);
            status_message = "Bootloader Flash write failed";
        }
        else
        {
            blink(5, 2000000, 10000000);
            status_message = "Bootloader upgraded";
        }
    }
    else
    {
        /* Existing bootloader is identical */
        blink(1, 2000000, 10000000);
        status_message = "Bootloader already upgraded";
    }

    /* Write address of status message (string literal) in Flash */
    Flash_write((uint32_t *)&app_status,
                (uint32_t *)&status_message, sizeof(app_status));

    /* Bootloader upgraded, allow interrupts again */
    Sys_exitCriticalSection();

    /* New bootloader processed, reset MCU */
    lib_state->stopStack();
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
    uint32_t status = *(volatile uint32_t *)&app_status; /* GCC is clever... */

    Led_init();
    /* No node configuration. This app should only be used to otap already
     * configured nodes.
     */

    if (status == 0)
    {
        /* New bootloader already processed, do nothing */
    }
    else if (status == 0xffffffff)
    {
        /* Fresh application, upgrade bootloader */
        upgradeBootloader();
    }
    else
    {
        uint32_t zero = 0;

        /* Get status of bootloader upgrade */
        status_message = (const char *)status;

        /* Mark new bootloader as processed */
        Sys_enterCriticalSection();
        Flash_write((uint32_t *)&app_status, &zero, sizeof(app_status));
        Sys_exitCriticalSection();
    }

    /* Send status of bootloader upgrade after one second delay */
    lib_system->setPeriodicCb(sendStatus, 1000000, 1000);

    /*
     * Start the stack.
     * This is really important step, otherwise the stack will stay stopped and
     * will not be part of any network. So the device will not be reachable
     * without reflashing it.
     */
    lib_state->startStack();
}
