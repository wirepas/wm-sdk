/* Copyright 2023 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/* Linker script for Wirepas Mesh bootloader on nRF52832
 *
 * This file is for the 512 kB Flash, 64 kB RAM variant
 *
 * Flash is split into:
 *  - Bootloader executable code (BOOTLOADER)
 *  - Bootloader config: memory areas, auth. and encryption keys (BLCONFIG)
 *
 * RAM is split into:
 *  - RAM for bootloader variables, to support runtime services (BL_STATIC)
 *  - Rest of the RAM the bootloader can use for variables during booting (RAM)
 */

MEMORY
{
    BOOTLOADER (rx)   : ORIGIN = 0x00000000,            LENGTH = 16K - 1K
    BLCONFIG (rx)     : ORIGIN = 0x00000000 + 15K,      LENGTH = 1K
    BL_STATIC (rwx)   : ORIGIN = 0x20000000,            LENGTH = 1K
    RAM (rwx)         : ORIGIN = 0x20000000 + 1K,       LENGTH = 63K - 24
    INVALID (rwx)     : ORIGIN = 0xFFFFFFFF,            LENGTH = 0
}

INCLUDE gcc_bl_common.ld
