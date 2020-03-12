/* Copyright 2019 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

#include "api.h"
#include <string.h>
#include "fifo.h"


 bool fifo_init(fifo_t * fifo, void * buf, uint16_t size, uint16_t max)
{
    if (buf == NULL || size == 0 || max == 0)
    {
        return false;
    }

    fifo->buf = (uint8_t *) buf;
    fifo->head = 0;
    fifo->tail = 0;
    fifo->size = size;
    fifo->max = max;
    fifo->full = false;
    return true;
}


 uint16_t fifo_push(fifo_t * f, void * data)
{

    if (!f->full)
    {
        uint32_t offset = f->size * f->head;
        memcpy(f->buf + offset, data, f->size);

        f->head = (f->head + 1 ) % f->max;
        f->full = (f->tail == f->head);
        return f->size;
    }
    return 0;
}

 uint16_t fifo_pop(fifo_t * f, void * data)
{

    bool empty = (f->head == f->tail) && !f->full;

    if (!empty)
    {
        uint32_t offset = f->size * f->tail;
        memcpy(data, f->buf + offset, f->size);

        f->tail = (f->tail + 1 ) % f->max;
        f->full = false;
        return f->size;
    }
    return 0;
}

 uint16_t fifo_size(fifo_t * f)
{

    if(f->head < f->tail)
    {
        return (f->max - f->tail + f->head);
    }
    else if (!f->full)
    {
        return (f->head - f->tail);
    }
    else
    {
        return f->max;
    }
}

void * fifo_get_tail(fifo_t * f)
{

 bool empty = (f->head == f->tail) && !f->full;

    if (!empty)
    {
        uint32_t offset = f->size * f->tail;
        return (void *) (f->buf + offset);
    }
    return NULL;
}