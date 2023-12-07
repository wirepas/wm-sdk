/* Copyright 2023 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/* Linker script for Wirepas Mesh bootloader, common parts
 */

ENTRY(__startup__)

SECTIONS
{
    .text :
    {
        KEEP(*(.Vectors))
        __bl_version__ = .;
        KEEP(*(.BootloaderVersion))
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
        *(.crc_lut)
        . = ALIGN(8);
    } >BOOTLOADER

    __etext = .;
    __data_src_start__ = .;

    .datastatic :
    {
        __datastatic_start__ = .;
        /* .ramtext may be needed for routines writing or erasing internal Flash */
        *(.ramtext)
        *memoryarea.*o*(.data* .data)
        *scratchpad.*o*(.data* .data)
        *internal_flash*.*o*(.data* .data)
        *external_flash*.*o*(.data* .data)
        *debug_flow*.*o*(.data* .data)
        /* Customer specific external flash driver must be located in
         * board/board_name/bootloader folder.
         */
        *board/*/bootloader/*.*o*(.data* .data)
        *bootloader/bl_hardware*.*o*(.data* .data)
        . = ALIGN(8);
        __datastatic_end__ = .;
    } >BL_STATIC AT >BOOTLOADER

    .bssstatic :
    {
        __bssstatic_start__ = .;
        *memoryarea.*o*(.bss.* .bss)
        *scratchpad.*o*(.bss.* .bss)
        *internal_flash*.*o*(.bss* .bss)
        *external_flash*.*o*(.bss* .bss)
        *debug_flow*.*o*(.bss* .bss)
        /* Customer specific external flash driver must be located in
         * board/board_name/bootloader folder
         */
        *board/*/bootloader/*.*o*(.bss* .bss)
        *bootloader/bl_hardware*.*o*(.bss* .bss)
        /* COMMON symbols here as well */
        *(COMMON)
        . = ALIGN(8);
        __bssstatic_end__ = .;
    } >BL_STATIC

    .data :
    {
        __data_start__ = .;
        *(.data.*)
        *(.data)
        *(.data1)
        . = ALIGN(8);
        __data_end__ = .;
    } >RAM AT >BOOTLOADER

    .bss :
    {
        __bss_start__ = .;
        *(.bss.*)
        *(.bss)
        . = ALIGN(8);
        __bss_end__ = .;
    } >RAM

    .blconfig :
    {
        /* Bootloader configuration (memory areas, keys) */
        __blconfig_start__ = .;
        . += LENGTH(BLCONFIG);
        __blconfig_end__ = .;
    } >BLCONFIG

    /* .stack_dummy section doesn't contain any symbols. It is only
     * used for linker to calculate size of stack sections, and assign
     * values to stack symbols later */
    .stack_dummy (COPY):
    {
        *(.stack*)
    } > RAM

    /* Set stack top to end of RAM, and stack limit move down by
     * size of stack_dummy section */
    __stack_top__ = ORIGIN(RAM) + LENGTH(RAM);

    /* Dummy definition to meet a dependency in SystemInit().
     * Warning: Do not use this to anything! */
    __Vectors = 0;

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
        *(.ARM.extab.*)
        *(.ARM.exidx)
        *(.ARM.exidx.*)
    }
}
