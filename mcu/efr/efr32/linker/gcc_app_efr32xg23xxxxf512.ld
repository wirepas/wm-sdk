/* Copyright 2023 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/* Linker script for Wirepas Mesh application on EFR32xG23xxxxF512
 *
 * This file is for the 512 kB Flash, 64 kB RAM variant
 *
 * Flash memory areas:
 *  - Application executable code (FLASH)
 *      - By default, 40 kB of Flash memory is allocated for the application
 *      - It's possible to allocate more space for the application, but this
 *        will reduce the available space for the stored scratchpad
 *      - The scratchpad is typically stored in the internal Flash memory and
 *        occupies the same memory area as the application
 *      - If the application is too large, there may not be sufficient
 *        space for Over-The-Air Programming (OTAP)
 *      - For more details, refer to the OTAP documentation
 *
 * RAM memory areas:
 *  - RAM for application static variables (RAM)
 *      - By default, the application is allocated 12 kB of RAM, minus 8 bytes
 *      - The reserved RAM for the application can range from 1 to 16 kB (minus
 *        8 bytes) by adjusting the MEMORY definition. For example, to reserve
 *        16 kB of RAM, the definition would be:
 *
 *        RAM (rwx)       : ORIGIN = 0x2000C000, LENGTH = 16K - 8
 *
 *      - The mesh stack performs better with more RAM, so it's recommended not
 *        to increase the RAM allocation for the application unless it's
 *        critical
 */

MEMORY
{
  /* See the notes above, before changing FLASH or RAM length */
  FLASH (rx)      : ORIGIN = 0x08044000, LENGTH = 40K
  RAM (rwx)       : ORIGIN = 0x2000D000, LENGTH = 12K - 8
  INVALID (rwx)   : ORIGIN = 0xFFFFFFFF, LENGTH = 0 /* Sanity check */
}

/* Sanity check in case RAM reservation is modified */
ASSERT(ORIGIN(RAM) >= (0x20000000 + 64K - 16K), "RAM origin is too low")
ASSERT(LENGTH(RAM) <= 16K, "RAM area is too big")
ASSERT((ORIGIN(RAM) + LENGTH(RAM)) == (0x20000000 + 64K - 8), "RAM end is wrong")

INCLUDE gcc_app_common.ld
