/* Copyright 2018 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

#include <string.h>
#include <stdint.h>
#include "flash.h"
#include "updater.h"
#include "mcu.h"

/** Addresses determined by the linker */
extern unsigned int __data_src_start__;
extern unsigned int __data_start__;
extern unsigned int __data_end__;
extern unsigned int __bss_start__;
extern unsigned int __bss_end__;

extern unsigned int __bl_hex_start__;
extern unsigned int __bl_hex_end__;
extern unsigned int __blconfig_start__;
extern unsigned int __blconfig_end__;
extern unsigned int __glue_start__;
extern unsigned int __stack_start__;
extern unsigned int __stack_end__;
extern unsigned int __application_start__;
extern unsigned int __application_end__;
extern unsigned int __bootloader_v3_size__;
extern unsigned char __bootloader_buffer__[];

#define BLCONFIG_SIZE      ((uint32_t)&__blconfig_end__ - \
                            (uint32_t)&__blconfig_start__)
#define BOOTLOADER_SIZE_V3 ((uint32_t)(&__bootloader_v3_size__))
#define BLCONFIG_OFFSET    ((uint32_t)(&__blconfig_start__))
#define BLCONFIG_OFFSET_V3 (BOOTLOADER_SIZE_V3 - BLCONFIG_SIZE)
#define BLCONFIG_START     ((uint32_t)(&__blconfig_start__))
#define APPLICATION_START  ((uint32_t)(&__application_start__))
#define GLUE_START         ((uint32_t)(&__glue_start__))
#define FIRMWARE_START     ((uint32_t)(&__stack_start__))
#define FIRMWARE_END       ((uint32_t)(&__stack_end__))
#define FIRMWARE_SIZE      (FIRMWARE_END - FIRMWARE_START)
#define BL_HEX_START       ((uint32_t)(&__bl_hex_start__))

/* This is needed to reserve some space for the bootloader to write the
 * bl_info_header.
 */
const bl_info_header_t info_hdr __attribute__(( section (".bl_info_header"))) =
{
    .length = 0xFFFFFFFF,
    .crc = 0xFFFF,
    .seq = 0xFF,
    .flags = 0xFF,
    .id = 0xFFFFFFFF,
    .major = 0xFF,
    .minor = 0xFF,
    .maint = 0xFF,
    .devel = 0xFF,
    .written_size = 0xFFFFFFFF,
};

bl_info_header_t m_stack_header;
bl_info_header_t m_app_header;

/* This function update the areas definitions to be compatible with the
 * bootlaoder v3. It does the following :
 *  - Remove the Firmware no application Area
 *  - Update the flag fields.
 *  - Change area id for Firmware and bootloader (|0x100)
 *  - Update size of the bootlodaer
 *  - Update size and start address of the stack
 *  - Add dedicated scratchpad area if needed.
 */
static void update_blconfig(void)
{
    uint8_t new_idx = 0;
    uint32_t persistent_begin = 0;
    uint32_t last_area = 0;
    uint8_t hw_magic = 0 ;

    memory_area_t * old_areas = (memory_area_t *)BLCONFIG_OFFSET;
    memory_area_t * new_areas =
       (memory_area_t *)((uint32_t)&__bootloader_buffer__ + BLCONFIG_OFFSET_V3);

    /* Get app area id */
    bl_info_header_t * app_info_hdr =
                (bl_info_header_t *)(APPLICATION_START + BL_INFO_HEADER_OFFSET);

    /* Copy old bootloader config to RAM buffer */
    memcpy(new_areas, old_areas, BLCONFIG_SIZE);

    /* Set areas fields to 0xFFFFFFFF */
    memset(new_areas, 0xFF, NUM_MEMORY_AREAS * sizeof(memory_area_t));

    for(int i = 0; i < NUM_MEMORY_AREAS; i++)
    {
        /* Undifined Area */
        if(old_areas[i].id == 0xFFFFFFFF )
        {

        }

        /* Bootloader area */
        else if((old_areas[i].id & 0xFFFFFF00) == 0x10000000)
        {
            new_areas[new_idx].length = BOOTLOADER_SIZE_V3;
            new_areas[new_idx].address = old_areas[i].address;
            /* Modify bootloader area id 0x000000xx -> 0x000001xx */
            new_areas[new_idx].id = old_areas[i].id | 0x100;
            /* Overwrite flags */
            /* Don't store version; internal flash; bootloader */
            new_areas[new_idx].flags = 0x00000000;
            new_idx++;
        }

        /* Firmware Area */
        else if((old_areas[i].id & 0xFFFFFF00) == 0x00000000)
        {
            new_areas[new_idx].length = FIRMWARE_SIZE;
            new_areas[new_idx].address = FIRMWARE_START;
            /* Modify firmware area id 0x000000xx -> 0x000001xx */
            new_areas[new_idx].id = old_areas[i].id | 0x100;
            /* Overwrite flags */
            /* Store version; internal flash; stack */
            new_areas[new_idx].flags = 0x00000005;
            new_idx++;
        }

        /* Persitant area */
        else if((old_areas[i].id & 0xFFFFFF00) == 0x20000000)
        {
            new_areas[new_idx].length = old_areas[i].length;
            new_areas[new_idx].address = old_areas[i].address;
            new_areas[new_idx].id = old_areas[i].id;
            /* Overwrite flags */
            /* Don't store version; internal flash; persistent */
            new_areas[new_idx].flags = 0x0000000C;
            new_idx++;
        }

        /* Application area */
        else if(old_areas[i].id == app_info_hdr->id)
        {
            new_areas[new_idx].length = old_areas[i].length;
            new_areas[new_idx].address = old_areas[i].address;
            new_areas[new_idx].id = old_areas[i].id;
            /* Overwrite flags */
            /* Store version; internal flash; application */
            new_areas[new_idx].flags = 0x00000009;
            new_idx++;
        }

        /* Firmware no application Area */
        else if((old_areas[i].id & 0xFFFFFF00) == 0x30000000)
        {
            /* Not used anymore, skip */
        }

        /* This is a User defined area */
        else
        {
            new_areas[new_idx].length = old_areas[i].length;
            new_areas[new_idx].address = old_areas[i].address;
            new_areas[new_idx].id = old_areas[i].id;
            /* Don't store version; internal flash; user */
            new_areas[new_idx].flags = 0x00000014;
            new_idx++;
        }
    }

    /* Find if scratchpad is in a dedicated area just before persistent area. */
    for(int i = 0; i < NUM_MEMORY_AREAS; i++)
    {
        /* Persitant area */
        if((new_areas[i].id & 0xFFFFFF00) == 0x20000000)
        {
            persistent_begin = new_areas[i].address;
            hw_magic = new_areas[i].id & 0xFF;
        }
        else if(new_areas[i].id != 0xFFFFFFFF )
        {
            uint32_t end_area = new_areas[i].address + new_areas[i].length;

            if(end_area > last_area)
            {
                last_area = end_area;
            }
        }
    }

    /* Declare scratchpad dedicated area */
    if(last_area < persistent_begin && new_idx < NUM_MEMORY_AREAS)
    {
        new_areas[new_idx].length = persistent_begin - last_area;
        new_areas[new_idx].address = last_area;
        new_areas[new_idx].id = 0x30000000 | hw_magic;
        new_areas[new_idx].flags = 0x00000010;
    }
}

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

/* Update the bl_info_header of the stack.
 * written_size has been added to v4.0 and was not written by v3.x bootloader.
 * Also the header is moved because the stack area start at 16k now.
 */
static void update_stack_info_header(void)
{
    uint32_t * ptr;

    /* firmware area id has changed between v3.x and v4.0*/
    m_stack_header.id |= 0x100;

    /* Calculate written size */
    ptr = (uint32_t *)(FIRMWARE_END) - 1;

    while(*ptr == 0xFFFFFFFF && (uint32_t)ptr > FIRMWARE_START)
    {
        ptr--;
    }

    m_stack_header.written_size = Flash_getPage((uint32_t)ptr) +
                                 FLASH_PAGE_SIZE_BYTES -
                                 FIRMWARE_START;
}


/* Update the bl_info_header of the application.
 * written_size has been added to v4.0 and was not written by v3.x bootloader.
 */
static void update_app_info_header(void)
{
    const app_information_header_t * hdr =
        (const app_information_header_t *)(APPLICATION_START +
                                           APP_V2_TAG_OFFSET +
                                           APP_V2_TAG_LENGTH);

    memcpy(&m_app_header,
           (void *)(APPLICATION_START + BL_INFO_HEADER_OFFSET),
           sizeof(bl_info_header_t));

    m_app_header.written_size = Flash_getPage(hdr->length) +
                              FLASH_PAGE_SIZE_BYTES;

    /* Program only written size field to app info header */
    Flash_write(&(((bl_info_header_t *)
                   (APPLICATION_START + BL_INFO_HEADER_OFFSET))->written_size),
                &m_app_header.written_size,
                sizeof(m_app_header.written_size));
}

int main(void)
{
    uint8_t * ptr_hex = (uint8_t * )BL_HEX_START;

    unsigned int * src, * dst;

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

    Flash_init();

    /* Copy new bootlaoder to RAM buffer */
    memcpy(&__bootloader_buffer__[0], ptr_hex, BOOTLOADER_SIZE_V3);

    /* Update firmware area id and area flags */
    update_blconfig();

    /* Backup stack header before erasing area */
    /* GLUE Start is the actual location of the firmware */
    memcpy(&m_stack_header,
    	   (void *)(GLUE_START + BL_INFO_HEADER_OFFSET),
           sizeof(bl_info_header_t));


    /* Erase bootloader area */
    erase_bootloader_area(BOOTLOADER_SIZE_V3);

    /* Program new bootloader to flash */
    Flash_write(0, __bootloader_buffer__, BOOTLOADER_SIZE_V3);


    /* Update firmware bl_info_header */
    update_stack_info_header();

    /* Update application bl_info_header */
    update_app_info_header();

#ifdef NRF52
    /* Program full info header at the new location*/
    Flash_write((void *)(FIRMWARE_START + BL_INFO_HEADER_OFFSET),
                &m_stack_header,
                sizeof(bl_info_header_t));

#endif

#ifdef EFR32FG12
    extern unsigned int __first_page_start__;

    /* Erase first page of stack, at this point it contains the updater startup.
     */
    Flash_erasePage(FIRMWARE_START);

    /* Copy first page of stack to RAM buffer */
    memcpy(&__bootloader_buffer__[0],
           (void *)(&__first_page_start__),
           FLASH_PAGE_SIZE_BYTES);

    /* Copy updated bl_info_header to RAM buffer */
    memcpy((void*)(__bootloader_buffer__ + BL_INFO_HEADER_OFFSET),
           &m_stack_header,
           sizeof(bl_info_header_t));

    /* Write the first page to flash */
    Flash_write((void* )FIRMWARE_START,
                __bootloader_buffer__,
                FLASH_PAGE_SIZE_BYTES);
#endif

    /* Reset to start with new bootloader and stack */
    NVIC_SystemReset();
}

/* Updater entrypoint from the legacy bootloader.
 * This section must be less than 16bytes.
 */
void __attribute__ ((noreturn, section (".entrypoint"))) entrypoint(void)
{
    main();

    while(1);
}
