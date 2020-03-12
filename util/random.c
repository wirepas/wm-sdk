/* Copyright 2019 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

#include <stdint.h>
#include "random.h"
#include "api.h"

typedef union
{
    uint32_t    raw_value;      /**< Full 32bit random value */
    uint16_t    word_array[2];  /**< 32bit random value as 16 bit integers */
    uint8_t     byte_array[4];  /**< 32bit random value as 8 bit integers */
} random_value_t;

// Seed for PRNG (entropy is not accumulated to this)
static uint32_t m_seed;

// Internal random value
static random_value_t m_value;

// Macro to remove seed = 0
#define REMOVE_ZERO(_seed)  ((uint32_t) 1) ^ _seed

// Updates internal random value
static void random_update(void);

void Random_init(uint32_t seed)
{
    m_seed = REMOVE_ZERO(seed);
}

uint32_t Random_get32(void)
{
    random_update();            // update random value
    return m_value.raw_value;     // return full long word
}

uint8_t Random_get8(void)
{
    random_update();            // update random value
    return m_value.byte_array[3]; // return highest byte
}

uint16_t Random_get16(void)
{
    random_update();            // update random value
    return m_value.word_array[1]; // return highest word
}

static void random_update(void)
{
    // "Quick and dirty" linear congruential data generator
    // (See Numerical Recipes in C)
    // I_(j + 1) = (a * I_j + c) % 2^32
    m_seed = ((uint32_t) 1664525) * m_seed + ((uint32_t) 1013904223);
    // Try to collect some more entropy from timers
    m_value.raw_value = lib_time->getTimestampHp();
    m_value.raw_value += m_seed;
}
