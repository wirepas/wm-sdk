/* Copyright 2023 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/* Linker script for Wirepas Mesh bootloader on EFR32xG21xxxxF768
 *
 * This file is for the 768 kB Flash, 64 kB RAM variant
 *
 * Flash is split into:
 *  - Bootloader executable code (BOOTLOADER)
 *  - Bootloader config: memory areas, auth. and encryption keys (BLCONFIG)
 *
 * RAM is split into:
 *  - RAM for bootloader variables, to support runtime services (BL_STATIC)
 *  - Rest of the RAM the bootloader can use for variables during booting (RAM)
 *
 * Dedicated 16 kB bootloader Flash area is not used.
 */

MEMORY
{
    BOOTLOADER (rx)   : ORIGIN = 0x00000000,            LENGTH = 16K - 1K
    BLCONFIG (rx)     : ORIGIN = 0x00000000 + 15K,      LENGTH = 1K
    BL_STATIC (rwx)   : ORIGIN = 0x20000000 + 24,       LENGTH = 1024
    RAM (rwx)         : ORIGIN = 0x20001500,            LENGTH = 64K - 0x1500
    INVALID (rwx)     : ORIGIN = 0xFFFFFFFF,            LENGTH = 0
}

INCLUDE gcc_bl_common.ld
