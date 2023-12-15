/* Copyright 2023 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/* Linker script for Wirepas Mesh application on nRF9161
 *
 * This file is for the 1024 kB Flash, 256 kB RAM variant
 *
 * Flash memory areas:
 *  - Application executable code (FLASH)
 *      - By default, 256 kB of Flash memory is allocated for the application
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
 *      - The application is allocated 155 kB of RAM
 */

MEMORY
{
  /* See the note above, before changing FLASH length */
  FLASH (rx)      : ORIGIN = 0x00044000, LENGTH = 256K
  RAM (rwx)       : ORIGIN = 0x20019400, LENGTH = 155K
  INVALID (rwx)   : ORIGIN = 0xFFFFFFFF, LENGTH = 0 /* Sanity check */
}

INCLUDE gcc_app_common.ld
