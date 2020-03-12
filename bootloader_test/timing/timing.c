/* Copyright 2018 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

#include "timing.h"
#include "string.h"
#include "mcu.h"

void Timing_init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

void Timing_reset(timing_handle_t * h)
{
    memset(h,0,sizeof(timing_handle_t));
    h->min = UINT32_MAX;
}

void Timing_start(timing_handle_t * h)
{
    h->start = DWT->CYCCNT;
}

void Timing_stop(timing_handle_t * h)
{
    uint32_t cycles;

    h->stop = DWT->CYCCNT;
    h->cnt++;

    if(h->stop > h->start)
    {
        cycles = h->stop - h->start;
    }
    else
    {
        cycles = UINT32_MAX - h->start + h->stop;
    }

    if(cycles < h->min)
    {
        h->min = cycles;
    }

    if(cycles > h->max)
    {
        h->max = cycles;
    }

    h->sum += cycles;
    h->avg = h->sum / h->cnt;
}
