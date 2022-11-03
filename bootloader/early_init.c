/* Copyright 2018 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

#include "early_init.h"
#include "power.h"


/* Declaration of a weak custom early init that can be overwritten for each
 * board under board/<board_name>/bootloader/custom_early_init.c
 */
void custom_early_init(void) __attribute__((weak));

/* Declaration of a weak custom early init that can be overwritten for each
 * app under $(APP_SRCS_PATH)/bootloader/app_early_init.c
 */
void app_early_init(void) __attribute__((weak));

void early_init()
{
    Power_enableDCDC();

    custom_early_init();

    app_early_init();
}

/* This is the default implementation of the first_boot function. It is defined
 * as weak symbol and can be redifined in the application.
 */
void __attribute((weak)) first_boot(void)
{
}
