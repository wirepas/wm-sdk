/* Copyright 2023 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/* Linker script for Wirepas Mesh bootloader on nRF9161
 *
 * This file is for the 1024 kB Flash, 256 kB RAM variant
 *
 * Flash is split into:
 *  - Bootloader executable code (BOOTLOADER)
 *  - Bootloader config: memory areas, auth. and encryption keys (BLCONFIG)
 *
 * RAM is split into:
 *  - RAM for bootloader variables, to support runtime services (BL_STATIC)
 *  - RAM for Wirepas Mesh stack that is retained over boots (RETAINED_RAM)
 *      - This area is not to be altered by the bootloader
 *  - Rest of the RAM the bootloader can use for variables during booting (RAM)
 *      - Limited to 29 kB here, in case a smaller RAM variant is introduced
 *  - Stack for the bootloader to use only during booting (BL_STACK)
 *      - Separate from regular stack, to protect stack traces during development
 */

MEMORY
{
    BOOTLOADER (rx)   : ORIGIN = 0x00000000,                LENGTH = 32K - 1K
    BLCONFIG (rx)     : ORIGIN = 0x00000000 + 31K,          LENGTH = 1K
    BL_STATIC (rwx)   : ORIGIN = 0x20000000,                LENGTH = 1K
    RETAINED_RAM (r)  : ORIGIN = 0x20000000 + 1K,           LENGTH = 30K
    RAM (rwx)         : ORIGIN = 0x20000000 + 1K + 30K,     LENGTH = 29K
    BL_STACK (rwx)    : ORIGIN = 0x20000000 + 31K + 29K,    LENGTH = 4K
    INVALID (rwx)     : ORIGIN = 0xFFFFFFFF,                LENGTH = 0
}

INCLUDE gcc_bl_common.ld

/* nRF9161 bootloader uses a separate RAM area (BL_STACK) as boot-time stack */
__stack_top__ = ORIGIN(BL_STACK) + LENGTH(BL_STACK);
