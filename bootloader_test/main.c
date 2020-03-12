/* Copyright 2018 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

#include <string.h>
#include "../bootloader_test/api/bl_interface.h"
#include "../bootloader_test/print/print.h"
#include "../bootloader_test/tests/test.h"
#include "../bootloader_test/timing/timing.h"

/** Addresses determined by the linker */
extern unsigned int __data_src_start__;
extern unsigned int __data_start__;
extern unsigned int __data_end__;
extern unsigned int __bss_start__;
extern unsigned int __bss_end__;

/* We need to reserve some space for the application header. It is not used but
 * otherwise genScratchpad.py would overwrite this area.
 */
const uint32_t info_hdr[8] __attribute__(( section (".app_header")));

/**
 * \brief   The bootloader test application.
 */
void bootloader_test(bl_interface_t * interface)
{
    unsigned int * src, * dst;
    bool final_res = true;

    Print_init();
    Timing_init();

    /* Copy data from flash to RAM */
    for(src = &__data_src_start__,
        dst = &__data_start__;
        dst != &__data_end__;)
    {
        *dst++ = *src++;
    }

    /* Initialize the .bss section */
    for(dst = &__bss_start__; dst != &__bss_end__;)
    {
        *dst++ = 0;
    }

    Print_printf("\n\n #######################################\n");
    Print_printf(    " #                                     #\n");
    Print_printf(    " #      Starting bootloader tests      #\n");
    Print_printf(    " #                                     #\n");
    Print_printf(    " #######################################\n\n");

    Print_printf("Bootloader version is %d\n", interface->version);

    final_res &= Tests_info(interface);
    final_res &= Tests_areas(interface);
    final_res &= Tests_timings(interface);

    Print_printf("\n\n #######################################\n");
    Print_printf(    " #                                     #\n");
    Print_printf(    " #         Final result is %s        #\n",
                                                final_res ? "PASS" : "FAIL");
    Print_printf(    " #                                     #\n");
    Print_printf(    " #######################################\n\n");

    while(1);
}

/**
 * \brief   Entrypoint from bootloader
 */
void __attribute__ ((noreturn, section (".entrypoint")))
                                        entrypoint(bl_interface_t * interface)
{
    bootloader_test(interface);

    while(1);
}
