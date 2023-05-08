/* Copyright 2018 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

#include "../../bootloader_test/api/bl_interface.h"

#ifndef _TEST_H_
#define _TEST_H_

/* Number of maximum supported memory areas in tests
 *
 * The bootloader used to only support eight memory areas, but that was later
 * increased to 16. Remember to increase this, if the bootloader supports more
 * areas in the future.
 */
#define MAX_TESTED_MEMORY_AREAS 16

/* Macro to print test startup */
#define START_TEST(_name_, _desc_) Print_printf("\nTEST [%s]: %s\n", \
                                                #_name_, #_desc_);

/* Macro to print test result */
#define END_TEST(_name_, _res_)    Print_printf("TEST [%s]: %s\n", \
                                            #_name_, _res_ ? "OK" : "FAIL");

/**
 * \brief   Tests that prints informations about memory area and flash.
 */
bool Tests_info(bl_interface_t * interface);

/**
 * \brief   Test read/write/erase in memory areas.
 */
bool Tests_areas(bl_interface_t * interface);

/**
 * \brief   Test flash read/write/erase timings.
 */
bool Tests_timings(bl_interface_t * interface);

/**
 * \brief   Tests scratchpad library.
 */
bool Tests_test_scratchpad(const scratchpad_services_t * scrat_services);

#endif /* _TEST_H_ */
