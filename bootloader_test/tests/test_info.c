/* Copyright 2018 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

#include <string.h>
#include "print.h"
#include "test.h"

static const char * type_lut[] = {"BOOTLOADER ",
                                  "STACK      ",
                                  "APPLICATION",
                                  "PERSISTENT ",
                                  "SCRATCHPAD ",
                                  "USER       "};

bool info_areas(const memory_area_services_t * mem_area_services)
{
    bl_interface_res_e bl_res;
    bl_memory_area_id_t areas[BL_MEMORY_AREA_MAX_AREAS];
    uint8_t num_areas = BL_MEMORY_AREA_MAX_AREAS;
    bool res = true;

    START_TEST(INFO, List all areas);

    mem_area_services->getAreaList(areas, &num_areas);

    Print_printf("Found %d areas\n", num_areas);
    Print_printf("[ID        ][ADDRESS   ][LENGTH][TYPE       ][Ext/Int]\n");


    for (uint8_t i = 0; i < num_areas; i++)
    {
        bl_memory_area_info_t info;
        bl_res = mem_area_services->getAreaInfo(areas[i], &info);
        if (bl_res != BL_RES_OK)
        {
            Print_printf("ERROR: cannot get area info res=%d\n",areas[i]);
            res = false;
            continue;
        }
        Print_printf("[0x%08x][0x%08x][%6d][%s][%s]\n",
                     info.area_id,
                     info.area_physical_address,
                     info.area_size,
                     type_lut[info.type],
                     info.external_flash?"External":"Internal");
    }

    END_TEST(List all areas, res);
    return res;
}

/** \brief  AES-128 key length */
#define KEY_LENGTH          (16)
/** \brief  Maximum number of key pairs (as many as fits in BLCONFIG section) */
#define NUM_KEY_PAIRS       29

bool info_keys(const memory_area_services_t * mem_area_services)
{
    bool res = true;
#ifdef NRF52
    uint8_t * keys = (uint8_t *)(0x3C80);
#else //EFR32
    uint8_t * keys = (uint8_t *)(0x3880);
#endif

    START_TEST(INFO, List all keys stored in bootloader);
    for(uint32_t i=0; i < NUM_KEY_PAIRS; i++)
    {
        /* Exit loop early if the keys are not set */
        if(*(keys + (2*i)*KEY_LENGTH) == 0xFF)
        {
            break;
        }

        Print_printf("Authentication key n° %02d : ",i);
        for(uint32_t j=0; j < KEY_LENGTH; j++)
        {
            Print_printf("[%02X]",*(keys + (2*i)*KEY_LENGTH+j));
        }
        Print_printf("\n");

        Print_printf("Encryption     key n° %02d : ",i);
        for(uint32_t j=0; j < KEY_LENGTH; j++)
        {
            Print_printf("[%02X]",*(keys + (2*i+1)*KEY_LENGTH+j));
        }
        Print_printf("\n");

    }
    END_TEST(List all keys stored in bootloader, res);

    return res;
}

bool info_scratchpad(const scratchpad_services_t * scrat_services)
{
    bool res = true;
    bl_interface_res_e bl_res;
    bl_scrat_info_t scrat_info;

    START_TEST(INFO, List scratchpad info);
    bl_res = scrat_services->getInfo(&scrat_info);
    if (bl_res != BL_RES_OK)
    {
        Print_printf("ERROR: cannot get scratchpad info res=%d\n",bl_res);
        res = false;
    }
    else
    {
        Print_printf("Scratchpad info: \n");
        Print_printf("\t[Max size]: %lu\n", scrat_info.area_length);
        Print_printf("\t[dedicated]: %s\n", scrat_info.dedicated?"true":"false");
        Print_printf("\t[Erase time]: %lu\n", scrat_info.erase_time);
    }

    END_TEST(List scratchpad info, res);
    return res;
}

bool info_flash(const memory_area_services_t * mem_area_services)
{
    bool res = true;
    bl_interface_res_e bl_res;
    bl_memory_area_info_t info;
    bl_memory_area_id_t areas[BL_MEMORY_AREA_MAX_AREAS];
    uint8_t num_areas = BL_MEMORY_AREA_MAX_AREAS;
    bl_memory_area_id_t id;

    //Internal flash info
    START_TEST(INFO, Print internal flash info);

    mem_area_services->getIdfromType(&id,
                                     BL_MEM_AREA_TYPE_STACK);

    if(id == BL_MEMORY_AREA_UNDEFINED)
    {
        res = false;
        Print_printf("ERROR: can't get Stack area id.\n");
    }
    else
    {
        bl_res = mem_area_services->getAreaInfo(id, &info);

        if(bl_res != BL_RES_OK)
        {
            res = false;
            Print_printf("ERROR: can't get internal flash info.\n");
        }
        else
        {
            Print_printf("\t[flash_size]: %u bytes\n", info.flash.flash_size);
            Print_printf("\t[write_page_size]: %u bytes\n",
                                                info.flash.write_page_size);
            Print_printf("\t[erase_sector_size]: %u bytes\n",
                                                info.flash.erase_sector_size);
            Print_printf("\t[write_alignment]: %u bytes\n",
                                                info.flash.write_alignment);
        }
    }
    END_TEST(Print internal flash info, res);

    START_TEST(INFO, Print external flash info);

    id = BL_MEMORY_AREA_UNDEFINED;

    /* Search an area in external flash */
    mem_area_services->getAreaList(areas, &num_areas);
    for (uint8_t i = 0; i < num_areas; i++)
    {
        bl_res = mem_area_services->getAreaInfo(areas[i], &info);
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
        Print_printf("\t[flash_size]: %u\n", info.flash.flash_size);
        Print_printf("\t[write_page_size]: %u\n", info.flash.write_page_size);
        Print_printf("\t[erase_sector_size]: %u\n",
                                                info.flash.erase_sector_size);
        Print_printf("\t[write_alignment]: %u\n", info.flash.write_alignment);
    }
    END_TEST(Print external flash info, res);

    res = true;
    return res;
}

bool info_hardware(bl_interface_t * interface)
{
    bool res = true;
    const hardware_services_t * hardware_services =
                                                interface->hardware_services_p;

    START_TEST(INFO, Get hardware capabilities);

    if (interface->version >= 7)
    {
        Print_printf("Bootloader hardware capabilities:\n");
        Print_printf("\t[crystal_32k]: %s\n",
                hardware_services->getCapabilities()->crystal_32k?"yes":"no");
        Print_printf("\t[dcdc]: %s\n",
                hardware_services->getCapabilities()->dcdc?"yes":"no");
    }
    else
    {
        Print_printf("INFO: bootloader version is %d. Bootloader hardware "
                     "capabilities introduced in version 7.\n",
                     interface->version);
    }

    END_TEST(Get hardware capabilities, res);

    return res;
}

bool Tests_info(bl_interface_t * interface)
{
    bool res = true;
    res &= info_areas(interface->memory_area_services_p);
    res &= info_flash(interface->memory_area_services_p);
    res &= info_keys(interface->memory_area_services_p);
    res &= info_scratchpad(interface->scratchpad_services_p);
    res &= info_hardware(interface);

    return res;
}
