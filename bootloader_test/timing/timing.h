/* Copyright 2018 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

#ifndef _TIMING_H_
#define _TIMING_H_

#include "stdint.h"

/** \brief  Handle containing timing informations */
typedef struct
{
    uint32_t min;   /** Minimum time measured since reset (in CPU ticks) */
    uint32_t max;   /** Maximum time measured since reset (in CPU ticks) */
    uint32_t avg;   /** Average time measured since reset (in CPU ticks) */
    uint32_t cnt;   /** Number of measurement done since reset (in CPU ticks) */
    uint32_t start; /** Internal use */
    uint32_t stop;  /** Internal use */
    uint32_t sum;  /** Internal use */
} timing_handle_t;

/**
 * \brief   Initialize the timing module
 */
void Timing_init(void);

/**
 * \brief   Resets measurements of the handle
 * \param   h
 *          pointer to the timing handle
 */
void Timing_reset(timing_handle_t * h);

/**
 * \brief   Starts a timing measurement for the specified handle.
 * \param   h
 *          pointer to the timing handle
 */
void Timing_start(timing_handle_t * h);

/**
 * \brief   Stopts a timing measurement for the specified handle.
 * \param   h
 *          pointer to the timing handle
 */
void Timing_stop(timing_handle_t * h);

#endif //_TIMING_H_
