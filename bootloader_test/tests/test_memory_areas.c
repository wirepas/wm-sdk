/* Copyright 2018 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

#include <string.h>
#include "print.h"
#include "test.h"

static const memory_area_services_t * m_mem_services_p;


#define READ_BUFFER_SIZE 512
static uint8_t m_read_buffer[READ_BUFFER_SIZE];

#define WRITE_BUFFER_SIZE 512
static uint8_t m_write_buffer[WRITE_BUFFER_SIZE];


static bool check_memory_pattern(bl_memory_area_info_t * area_info_p,
                                 uint8_t pattern)
{
    bl_interface_res_e res;
    size_t size_to_read;
    size_t read_chunk_size = READ_BUFFER_SIZE;
    uint32_t pos = 0;

    Print_printf("check_memory_pattern id:%6X\n", area_info_p->area_id);

    /* Read chunk size is less than a page and odd. */
    do
    {
        read_chunk_size /= 2;
        read_chunk_size |= 0x01;
    } while(read_chunk_size >= area_info_p->flash.write_page_size);

    size_to_read = area_info_p->area_size;

    Print_printf("INFO: Verifying %d bytes by %d bytes chunks\n",
                                                        size_to_read,
                                                        read_chunk_size);

    while (size_to_read > 0)
    {
        size_t to_read = size_to_read > read_chunk_size ?
                                                read_chunk_size : size_to_read;

        /* Read full memory and compare against given pattern. */
        res = m_mem_services_p->startRead(area_info_p->area_id,
                                          m_read_buffer,
                                          pos,
                                          to_read);
        if (res != BL_RES_OK)
        {
            Print_printf("ERROR: cannot startRead for area id %x "
                         "res=%d pos=%d to_read=%d\n",
                         area_info_p->area_id,
                         res,
                         pos,
                         to_read);
            return false;
        }

        while (m_mem_services_p->isBusy(area_info_p->area_id))
        {
        }

        for (uint16_t i = 0; i < to_read; i++)
        {
            if (m_read_buffer[i] != pattern)
            {
                Print_printf("ERROR: Wrong pattern in flash in memory area %x "
                             "at offset %d (%d, %d): 0x%x vs 0x%x\n",
                             area_info_p->area_id,
                             pos + i, pos, i,
                             m_read_buffer[i],
                             pattern);

                for (uint16_t j = 0; j < to_read; j++)
                {
                    Print_printf("0x%x ", m_read_buffer[j]);
                }
                return false;
            }
        }

        pos += to_read;
        size_to_read -= to_read;
    }

    return true;
}

static bool erase_memory(bl_memory_area_info_t * area_info_p)
{
    bl_interface_res_e res;
    uint32_t sector_base = 0;
    size_t number_of_sector;
    Print_printf("erase_memory id:%6X\n",area_info_p->area_id);
    number_of_sector = area_info_p->area_size /
                                        area_info_p->flash.erase_sector_size;
    Print_printf("INFO: Erasing %d sectors of %d bytes\n",
                 number_of_sector,
                 area_info_p->flash.erase_sector_size);

    while (number_of_sector > 0)
    {
        res = m_mem_services_p->startErase(area_info_p->area_id,
                                           &sector_base,
                                           &number_of_sector);
        if (res != BL_RES_OK)
        {
            Print_printf("ERROR: cannot startErase for area id %x "
                         "res=%d number_of_sector=%d\n",
                         area_info_p->area_id,
                         res,
                         number_of_sector);
            return false;
        }

        while (m_mem_services_p->isBusy(area_info_p->area_id))
        {
        }
    }

    return true;
}

static bool write_memory_pattern(bl_memory_area_info_t * area_info_p,
                                 uint8_t pattern)
{
    bl_interface_res_e res;
    size_t size_to_write;
    size_t write_chunk_size = WRITE_BUFFER_SIZE;
    uint32_t pos = 0;

    /* Initialize our write buffer. */
    memset(m_write_buffer, pattern, sizeof(m_write_buffer));

    /* Write chunk size is less than a page and a multiple of
     * write_alignment.
     */
    do
    {
        write_chunk_size /= 2;
        write_chunk_size |= area_info_p->flash.write_alignment;
    } while(write_chunk_size >= area_info_p->flash.write_page_size);


    size_to_write = area_info_p->area_size;

    Print_printf("INFO: Writing %d bytes by %d bytes chunks\n",
                 size_to_write,
                 write_chunk_size);

    while (size_to_write > 0)
    {
        size_t to_write = size_to_write > write_chunk_size ?
                                            write_chunk_size : size_to_write;

        /* Don't write over page boundary */
        uint32_t next_page = ((uint32_t)pos &
                              (0xFFFFFFFF -
                               (area_info_p->flash.write_page_size - 1))) +
                             area_info_p->flash.write_page_size;

        if((uint32_t)(pos + to_write) > next_page)
        {
            to_write = next_page - pos;
        }

        /* Write memory */
        res = m_mem_services_p->startWrite(area_info_p->area_id,
                                           pos,
                                           m_write_buffer,
                                           to_write);
        if (res != BL_RES_OK)
        {
            Print_printf("ERROR: cannot startWrite for area id %x "
                         "res=%d pos=%d to_write=%d\n",
                         area_info_p->area_id,
                         res,
                         pos,
                         to_write);
            return false;
        }

        while (m_mem_services_p->isBusy(area_info_p->area_id));

        pos += to_write;
        size_to_write -= to_write;
    }

    return true;
}

static bool test_area(bl_memory_area_info_t * area_info_p)
{
    bool res;
    if (area_info_p->type == BL_MEM_AREA_TYPE_BOOTLOADER
        || area_info_p->type == BL_MEM_AREA_TYPE_STACK)
    {
        Print_printf("\nINFO : Area is not tested as type is not relevant "
                                                    "(BOOTLOADER or STACK)\n");
        return true;
    }

    START_TEST(Erase, Erase the memory area and check it back);

    Print_printf("before test id:%6X\n",area_info_p->area_id);
    res = erase_memory(area_info_p);
    Print_printf("after erase id:%6X\n",area_info_p->area_id);
    res &= check_memory_pattern(area_info_p, 0xff);
    Print_printf("after read id:%6X\n",area_info_p->area_id);

    END_TEST(Erase, res);

    if (!res)
    {
        return false;
    }

    START_TEST(Write and read back, Write pattern and check it back);

    res = write_memory_pattern(area_info_p, 0x15) &&
          check_memory_pattern(area_info_p, 0x15);

    END_TEST(Write and read back, res);

    if (!res)
    {
        return false;
    }

    return true;
}


bool Tests_areas(bl_interface_t * interface)
{
    // Test areas one by one
    bl_interface_res_e bl_res;
    bl_memory_area_id_t areas[MAX_TESTED_MEMORY_AREAS];
    uint8_t num_areas = MAX_TESTED_MEMORY_AREAS;
    bool res = true;

    /* Store the pointer globally. */
    m_mem_services_p = interface->memory_area_services_p;

    /* Get list of areas */
    m_mem_services_p->getAreaList(areas, &num_areas);

    for (uint8_t i = 0; i < num_areas; i++)
    {
        Print_printf("\nINFO: Checking area with id 0x%x:", areas[i]);

        bl_memory_area_info_t info;
        bl_res = m_mem_services_p->getAreaInfo(areas[i], &info);
        if (bl_res != BL_RES_OK)
        {
            Print_printf("\nERROR: cannot get area info res=%d\n",
                         areas[i],
                         bl_res);
            res = false;
            continue;
        }
        res &= test_area(&info);
    }

    return res;
}
