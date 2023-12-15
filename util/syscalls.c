/* Copyright 2019 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <reent.h>


/*
 * Implements the _sbrk syscall to enable malloc()
 * The heap memory area is just a static buffer.
 */

#include <errno.h>


#ifdef HEAP_MEM_SIZE

static uint8_t  _heap_mem[HEAP_MEM_SIZE];
static int32_t  _heap_mem_size;

#endif

void * _sbrk (ptrdiff_t incr) {
#ifdef HEAP_MEM_SIZE
    const uint8_t * prev_heap_end = &_heap_mem[_heap_mem_size];

    /* sanity checks
     * - if incr >= 0, malloc requires heap memory, check there is enough free memory.
     * - if incr < 0, malloc free heap memory, check there is no bug that would release too much memory.
     *
     * Note that the newlib-nano malloc allocator never free heap memory.
     */
    if ( (incr >= 0) && ((_heap_mem_size+incr) > HEAP_MEM_SIZE)) {
        errno = ENOMEM;
        return (void *) -1;

    } else if ((incr < 0) && ((_heap_mem_size - incr) < 0)) {
        _heap_mem_size = 0;

        errno = EFAULT;
        return (void *) -1;
    }

    /* Update allocated mem size, and return pointer to the chunck */
    _heap_mem_size += incr;
    return (void *) prev_heap_end;

#else

    errno = ENOMEM;
    return (void *) -1;

#endif
}

