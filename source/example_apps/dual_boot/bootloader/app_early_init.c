/* Copyright 2020 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

#include <stdint.h>
#include <mcu.h>

/* 
 * Address where the second application is stored
 * It should be alligned with what is define in memory area definition
 * (nrf52840_app.ini).
 * Unfortunately, we cannot use the lib_memory_area that is not available
 * at this stage of boot
 */
#define SECOND_APP_START_ADDRESS    0xBA000

#define SWITCH_CONDITION_ADDRESS    0x000FA000

void start_app(unsigned long app_loc)
{
    asm(" ldr r1, [r0,#0]");    // SP from the reset vector
    asm(" mov sp, r1");         // copy to the SP
    asm(" ldr r0, [r0,#4]");    // PC from reset vector
    asm(" blx r0");             // jump there!
}

void app_early_init(void) 
{
    // Switching between first and second app depends
    // on the first word of the shared area.
    // In this example, second app is booted when the first word
    // is -1 (unitialized flash value)
    // Any logic can be implemented on top like pressing a button during boot
    if (*((uint32_t *) SWITCH_CONDITION_ADDRESS) == 0xFFFFFFFF)
    {
        start_app(SECOND_APP_START_ADDRESS);
    }
}