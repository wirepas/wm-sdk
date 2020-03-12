/* Copyright 2020 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

SEARCH_DIR(.)

/* Flash is 1024 kB and RAM is 128 kB
 *   - 40 kB of Flash can be used by the app
 *   - 8 kB (minus 8 bytes) of RAM can be used by the app
 */
MEMORY
{
    BOOTLOADER (rx)   : ORIGIN = 0x00000000,                LENGTH = 16K - 2K
    BLCONFIG (rx)     : ORIGIN = 0x00000000 + 14K,          LENGTH = 2K
    GLUE (rx)         : ORIGIN = 0x00000000 + 16K,          LENGTH = 2K
    STACK (rx)        : ORIGIN = 0x00000000 + 16K,          LENGTH = 240K
    FIRST_PAGE (rx)   : ORIGIN = 0x00000000 + 234K,         LENGTH = 2K
    BL_HEX (rx)       : ORIGIN = 0x00000000 + 236K,         LENGTH = 16K
    UPDATER (rx)      : ORIGIN = 0x00000000 + 252K,         LENGTH = 4K
    APPLICATION (rx)  : ORIGIN = 0x00040000,                LENGTH = 40K
    RAM (rwx)         : ORIGIN = 0x20000000 + 1k,           LENGTH = 127K - 24
    STACK_GUARD (rwx) : ORIGIN = 0x20000000 + 124K - 8,     LENGTH = 8
    /* Stack is here: 4 kB - 24 bytes */
    INVALID (rwx)     : ORIGIN = 0xFFFFFFFF,                LENGTH = 0
}

__flash_page_size_bytes__   = 2K;
__bootloader_v3_size__ = LENGTH(BL_HEX);

SECTIONS
{
    .bootloader :
    {
        /* Bootloader area */
        __bootloader_start__ = .;
        . += LENGTH(BOOTLOADER);
        __bootloader_end__ = .;
    } >BOOTLOADER

    .blconfig :
    {
        /* Bootloader configuration (memory areas, keys) */
        __blconfig_start__ = .;
        . += LENGTH(BLCONFIG);
        __blconfig_end__ = .;
    } >BLCONFIG

    .glue :
    {
        __glue_start__ = .;
        KEEP(*(.entrypoint))
        . = ALIGN(16);
        KEEP(*(.bl_info_header))
    } >GLUE

    .first_page :
    {
        /* Location of the first page of the stack */
        __first_page_start__ = .;
        . += LENGTH(FIRST_PAGE);
        __first_page_end__ = .;
    } >FIRST_PAGE

    .bl_hex :
    {
        /* New Bootloader bytes */
        __bl_hex_start__ = .;
        . += LENGTH(BL_HEX);
        __bl_hex_end__ = .;
    } >BL_HEX

    .text :
    {
        __text_start__ = .;
        *(.romtext)
        *(.text.*)
        *(.text)
        *(.glue_7)
        *(.glue_7t)
        *(.vfp11_veneer)
        *(.v4_bx)
        *(.rodata.*)
        *(.rodata)
        *(.rodata1)
        . = ALIGN(8);
        __text_end__ = .;
    } >UPDATER

    __data_src_start__ = .;

    .stack :
    {
        /* Firmware stack */
        __stack_start__ = .;
        . += LENGTH(STACK);
        __stack_end__ = .;
    } >STACK

    .application :
    {
        /* Application */
        __application_start__ = .;
        . += LENGTH(APPLICATION);
        __application_end__ = .;
    } >APPLICATION

    .data :
    {
        __data_start__ = .;
        *(.ramtext)
        *(.data.*)
        *(.data)
        *(.data1)
        . = ALIGN(8);
        __data_end__ = .;
    } >RAM AT >UPDATER

    .bss :
    {
        __bss_start__ = .;
        *(.bss.*)
        *(.bss)
        *(COMMON)
        . = ALIGN(8);
        __bootloader_buffer__ = .;
        . += __bootloader_v3_size__;
        . = ALIGN(8);
        __bss_end__ = .;
    } >RAM

.invalid :
    {
        *(.init)
        *(.fini)
        *(.preinit_array)
        *(.init_array)
        *(.fini_array)
        *(.ctors)
        *(.dtors)
        *(.jcr)
        *(.eh_frame)
        *(.eh_frame_hdr)
        *(.heap*)
        *(.tbss)
        *(.tdata)
        *(.tdata1)
        *(.got)
        *(.got.plt)
        *(.igot.plt)
        *(.iplt)
        *(.rel.dyn)
        *(.rel.iplt)
    } >INVALID  /* Linking fails if any of these sections have contents. */

    /* C++ exception unwinding information is silently discarded. */
    /DISCARD/ :
    {
        *(.ARM.extab)
        *(.ARM.exidx)
    }
}
