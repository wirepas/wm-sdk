/* Copyright 2022 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

SEARCH_DIR(.)

/* Flash is 512 kB and RAM is 64 kB RAM */
MEMORY
{
    FLASH (rx)        : ORIGIN = 0x08000000 + 32K,          LENGTH = 480K
    RAM (rwx)         : ORIGIN = 0x20000000 + 512           LENGTH = 64K - 512
    STACK_GUARD (rwx) : ORIGIN = 0x20000000 + 28K - 8,      LENGTH = 8
    /* Stack is here: 4 kB */
    INVALID (rwx)     : ORIGIN = 0xFFFFFFFF,                LENGTH = 0
}

SECTIONS
{
    .text :
    {
        __text_start__ = .;
        KEEP(*(.entrypoint))
        . = ALIGN(16);
        KEEP(*(.app_header))
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
    } >FLASH

    __data_src_start__ = .;

    .data :
    {
        __data_start__ = .;
        *(.ramtext)
        *(.data.*)
        *(.data)
        *(.data1)
        . = ALIGN(8);
        __data_end__ = .;
    } >RAM AT >FLASH

    __rom_end__ = __data_src_start__ + SIZEOF(.data);

    /* Total size includes persistent data, when implemented */
    __total_size_bytes__ = __rom_end__ - __text_start__;

    .bss :
    {
        __bss_start__ = .;
        *(.bss.*)
        *(.bss)
        *(COMMON)
        . = ALIGN(8);
        __bss_end__ = .;
    } >RAM

    /* Symbols for the RAM */
    __ram_end__ = ORIGIN(RAM) + LENGTH(RAM);

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
