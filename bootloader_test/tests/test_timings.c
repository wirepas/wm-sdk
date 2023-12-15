/* Copyright 2018 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

#include <string.h>
#include "print.h"
#include "test.h"
#include "timing.h"

/* MCU Clock frequency for tick to uS conversion */
#ifdef NRF52
    #define MCU_FREQ_MHZ 64
#else //EFR32
    #define MCU_FREQ_MHZ 38
#endif

static const memory_area_services_t * m_mem_services_p;

static bool test_timings(bl_memory_area_id_t id,
                         bl_memory_area_info_t * area_info_p)
{
    timing_handle_t timing_erase;
    timing_handle_t timing_byte;
    timing_handle_t timing_page;
    timing_handle_t timing_erase_call;
    timing_handle_t timing_byte_call;
    timing_handle_t timing_page_call;
    timing_handle_t timing_busy_call;

    Timing_reset(&timing_erase);
    Timing_reset(&timing_byte);
    Timing_reset(&timing_page);
    Timing_reset(&timing_erase_call);
    Timing_reset(&timing_byte_call);
    Timing_reset(&timing_page_call);
    Timing_reset(&timing_busy_call);

    /* Erase timings */
    uint32_t sector_base = 0;
    size_t number_of_sector =
                area_info_p->area_size / area_info_p->flash.erase_sector_size;

    /* sector erase timings */
    while(number_of_sector > 0)
    {
        bool busy = true;

        Timing_start(&timing_erase_call);
        m_mem_services_p->startErase(id, &sector_base, &number_of_sector);
        Timing_stop(&timing_erase_call);
        Timing_start(&timing_erase);

        while(busy)
        {
            Timing_start(&timing_busy_call);
            busy = m_mem_services_p->isBusy(id);
            Timing_stop(&timing_busy_call);
        }
        Timing_stop(&timing_erase);
    }


    /* byte write timings */
    uint8_t * buffer = (uint8_t *) 0x20000000;

    for(uint32_t i = 0;
        i < area_info_p->flash.write_page_size;
        i += area_info_p->flash.write_alignment)
    {
        bool busy = true;

        Timing_start(&timing_byte_call);
        m_mem_services_p->startWrite(id,
                                     i,
                                     buffer,
                                     area_info_p->flash.write_alignment);
        Timing_stop(&timing_byte_call);
        Timing_start(&timing_byte);

        while(busy)
        {
            Timing_start(&timing_busy_call);
            busy = m_mem_services_p->isBusy(id);
            Timing_stop(&timing_busy_call);
        }
        Timing_stop(&timing_byte);
    }

    /* page write timings */
    uint32_t number_of_page =
                area_info_p->area_size / area_info_p->flash.write_page_size;

    for(uint32_t i = 1; i < number_of_page; i++)
    {
        bool busy = true;

        Timing_start(&timing_page_call);
        m_mem_services_p->startWrite(id,
                                     i * area_info_p->flash.write_page_size,
                                     buffer,
                                     area_info_p->flash.write_page_size);
        Timing_stop(&timing_page_call);
        Timing_start(&timing_page);

        while(busy)
        {
            Timing_start(&timing_busy_call);
            busy = m_mem_services_p->isBusy(id);
            Timing_stop(&timing_busy_call);
        }
        Timing_stop(&timing_page);
    }

    Print_printf("[                      ][Theorical (uS)][Measured (uS)][# samples]\n");
    Print_printf("[byte_write_time       ][        %6u][       %6u][   %6d]\n", area_info_p->flash.byte_write_time, (timing_byte.avg - timing_busy_call.avg) / MCU_FREQ_MHZ, timing_byte.cnt);
    Print_printf("[page_write_time       ][        %6u][       %6u][   %6d]\n", area_info_p->flash.page_write_time, (timing_page.avg - timing_busy_call.avg) / MCU_FREQ_MHZ, timing_page.cnt);
    Print_printf("[sector_erase_time     ][        %6u][       %6u][   %6d]\n", area_info_p->flash.sector_erase_time, (timing_erase.avg - timing_busy_call.avg) / MCU_FREQ_MHZ, timing_erase.cnt);
    Print_printf("[byte_write_call_time  ][        %6u][       %6u][   %6d]\n", area_info_p->flash.byte_write_call_time, timing_byte_call.avg / MCU_FREQ_MHZ, timing_byte_call.cnt);
    Print_printf("[page_write_call_time  ][        %6u][       %6u][   %6d]\n", area_info_p->flash.page_write_call_time, timing_page_call.avg / MCU_FREQ_MHZ, timing_page_call.cnt);
    Print_printf("[sector_erase_call_time][        %6u][       %6u][   %6d]\n", area_info_p->flash.sector_erase_call_time, timing_erase_call.avg / MCU_FREQ_MHZ, timing_erase_call.cnt);
    Print_printf("[is_busy_call_time     ][        %6u][       %6u][   %6d]\n", area_info_p->flash.is_busy_call_time, timing_busy_call.avg / MCU_FREQ_MHZ, timing_busy_call.cnt);

    return true;
}

bool Tests_timings(bl_interface_t * interface)
{
    bool res = true;
    bl_interface_res_e bl_res;
    bl_memory_area_info_t info;
    bl_memory_area_id_t areas[MAX_TESTED_MEMORY_AREAS];
    uint8_t num_areas = MAX_TESTED_MEMORY_AREAS;
    bl_memory_area_id_t id;

    /* Store the pointer globally. */
    m_mem_services_p =  interface->memory_area_services_p;

    /* Internal flash info */
    START_TEST(TIMINGS, Verify internal flash Read/Write/Erase timings);

    m_mem_services_p->getIdfromType(&id, BL_MEM_AREA_TYPE_APPLICATION);

    if(id == BL_MEMORY_AREA_UNDEFINED)
    {
        res = false;
        Print_printf("ERROR: can't get Stack area id.\n");
    }
    else
    {
        bl_res = m_mem_services_p->getAreaInfo(id, &info);

        if(bl_res != BL_RES_OK)
        {
            res = false;
            Print_printf("ERROR: can't get internal flash info.\n");
        }
        else
        {
            res |= test_timings(id, &info);
        }
    }
    END_TEST(TIMINGS, res);

    START_TEST(TIMINGS, Verify external flash Read/Write/Erase timings);

    id = BL_MEMORY_AREA_UNDEFINED;

    /* Search an area in external flash */
    m_mem_services_p->getAreaList(areas, &num_areas);
    for (uint8_t i = 0; i < num_areas; i++)
    {
        bl_res = m_mem_services_p->getAreaInfo(areas[i], &info);
        if (bl_res != BL_RES_OK)
        {
            res = false;
            Print_printf("ERROR: can't get external flash info.\n");
        }
        else if(info.external_flash == true)
        {
            id = areas[i];
            break;
        }
    }

    if(id == BL_MEMORY_AREA_UNDEFINED)
    {
        Print_printf("INFO: There is no areas located in external flash.\n");
    }
    else
    {
        res |= test_timings(id, &info);
    }
    END_TEST(Print external flash info, res);

    return res;
}
