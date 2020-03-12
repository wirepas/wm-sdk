/* Copyright 2020 Wirepas Ltd. All Rights Reserved.
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
extern unsigned int __first_page_start__;

#define BLCONFIG_SIZE      ((uint32_t)&__blconfig_end__ - \
                            (uint32_t)&__blconfig_start__)
#define BOOTLOADER_SIZE_V3 ((uint32_t)(&__bootloader_v3_size__))
#define BLCONFIG_OFFSET    ((uint32_t)(&__blconfig_start__))
#define BLCONFIG_OFFSET_V3 (BOOTLOADER_SIZE_V3 - BLCONFIG_SIZE)
//#define BLCONFIG_START     ((uint32_t)(&__blconfig_start__))
//#define APPLICATION_START  ((uint32_t)(&__application_start__))
#define GLUE_START         ((uint32_t)(&__glue_start__))
#define FIRMWARE_START     ((uint32_t)(&__stack_start__))
//#define FIRMWARE_END       ((uint32_t)(&__stack_end__))
//#define FIRMWARE_SIZE      (FIRMWARE_END - FIRMWARE_START)
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


/* This function copies the blconfig of the actual bootloader to the new
   bootloader.
 */
static void update_blconfig(void)
{
    memory_area_t * old_areas = (memory_area_t *)BLCONFIG_OFFSET;
    memory_area_t * new_areas =
       (memory_area_t *)((uint32_t)&__bootloader_buffer__ + BLCONFIG_OFFSET_V3);

    /* Copy old bootloader config to RAM buffer */
    memcpy(new_areas, old_areas, BLCONFIG_SIZE);
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

int main(void)
{
    uint8_t * ptr_hex = (uint8_t * )BL_HEX_START;
    unsigned int * ram_src, * ram_dst;

    /* Copy .data from flash to RAM. */
    for (ram_src = &__data_src_start__,
         ram_dst = &__data_start__;
         ram_dst != &__data_end__;)
    {
        *ram_dst++ = *ram_src++;
    }

    /* Initialize the .bss section. */
    for (ram_dst = &__bss_start__; ram_dst != &__bss_end__;)
    {
        *ram_dst++ = 0;
    }

    Flash_init();

    /* Copy new bootlaoder to RAM buffer */
    memcpy(&__bootloader_buffer__[0], ptr_hex, BOOTLOADER_SIZE_V3);

    /* Update firmware areas and keys */
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
