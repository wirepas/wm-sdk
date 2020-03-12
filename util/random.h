/* Copyright 2019 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

#ifndef RANDOM_H
#define RANDOM_H

#include <stdint.h>

/**
 * \brief   Initialize the random generator
 * \param   seed
 *          Initialization seed
 */
void Random_init(uint32_t seed);

// Macro to get default random value
#define Random_get()    Random_get8()

/**
 * \brief   Get random number with u8 range
 * \return  Random number between [0...U8_MAX]
 */
uint8_t Random_get8(void);

/**
 * \brief   Get random number with u16 range
 * \return  Random number between [0...U16_MAX]
 */
uint16_t Random_get16(void);

/**
 * \brief   Get random number with u32 range
 * \return  Random number between [0...U32_MAX]
 */
uint32_t Random_get32(void);

/**
 * \brief   Generate random jitter
 * \param   x
 *          Jitter value
 * \return  Jitter between 0 ... x
 */
#define Random_jitter(x) (Random_get32() % (((x) + 1 ) & 0xFFFFFFF))

#endif  // RANDOM_H

