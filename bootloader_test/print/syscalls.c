/* Copyright 2018 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <reent.h>

/*
 * Stub methods for malloc(), free() and realloc() for newlib.
 * The purpose of these stubs is to minimize the amount of code
 * imported from newlib and also to make sure newlib code
 * doesn't secretly call malloc.
 */

void * _malloc_r(size_t size)
{
    (void)size;
    return NULL;
}

void _free_r(void * ptr)
{
    (void)ptr;
    return;
}

void * _realloc_r(struct _reent * a, void * b, size_t c)
{
    (void)a;
    (void)b;
    (void)c;
    return NULL;
}
