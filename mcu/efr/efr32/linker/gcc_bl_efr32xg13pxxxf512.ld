/* Copyright 2023 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/* Linker script for Wirepas Mesh bootloader on EFR32xG13PxxxF512
 *
 * This file is for the 512 kB Flash, 64 kB RAM variant
 *
 * Flash is split into:
 *  - Bootloader executable code (BOOTLOADER)
 *  - Bootloader config: memory areas, auth. and encryption keys (BLCONFIG)
 *  - Lock bits area (LOCKBITS)
 *
 * RAM is split into:
 *  - RAM for bootloader variables, to support runtime services (BL_STATIC)
 *  - Rest of the RAM the bootloader can use for variables during booting (RAM)
 *
 * Dedicated 32 kB bootloader Flash area is not used.
 */

MEMORY
{
    BOOTLOADER (rx)   : ORIGIN = 0x00000000,            LENGTH = 32K - 1K
    BLCONFIG (rx)     : ORIGIN = 0x00000000 + 31K,      LENGTH = 1K
    LOCKBITS (rx)     : ORIGIN = 0x0FE04000,            LENGTH = 512
    BL_STATIC (rwx)   : ORIGIN = 0x20000000 + 24,       LENGTH = 1024
    RAM (rwx)         : ORIGIN = 0x20001500,            LENGTH = 64K - 0x1500
    INVALID (rwx)     : ORIGIN = 0xFFFFFFFF,            LENGTH = 0
}

SECTIONS
{
    .lockbits :
    {
        KEEP(*(.lockbits))
    } >LOCKBITS
}

INCLUDE gcc_bl_common.ld
