/* Copyright 2018 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

#include "early_init.h"

/* This is the default implementation of early_init and first_boot functions.
 * They are defined as weak symbol and can be redifined in the application.
 */

void __attribute((weak)) early_init(void)
{
}

void __attribute((weak)) first_boot(void)
{
}
