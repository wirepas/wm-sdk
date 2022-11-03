/* Copyright 2017 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/* Application entry point */

    .syntax unified
    .arch armv6-m

    .section .entrypoint
    .thumb

entrypoint:
    /* Branch directly to _start(), in start.c */
    push    {r0, r1}
    ldr     r0, 1f
    str     r0, [sp, #4]
    pop     {r0, pc}
    .align  2
1:
    .word   _start
    .word   0

bl_info_header:
    /* Filled in by the bootloader */
    .long   0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff

__for_future_use__:
    .long   0xffffffff, 0xffffffff, 0xffffffff

/*
 * The linker will place the application tag and information
 * header (app_header in start.c) 48 bytes after the entrypoint
 */
