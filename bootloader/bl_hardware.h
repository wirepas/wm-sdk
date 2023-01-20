/* Copyright 2020 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

#ifndef HARDWARE_H_
#define HARDWARE_H_

#include <stdbool.h>

#if defined(NRF91_PLATFORM)
/** \brief  Platform specific descriptions for nRF91. */
typedef struct
{
    /** Pointer to platform specific modem initialization AT commands.
     *  AT commands are separated from each other with null character ('\0'),
     *  the end of the list is indicated with double null characters ("\0\0").
     *  (introduced in bootloader v9).
     */
    const char * at_commands;
} platform_nrf91_t;
#endif

#if defined(EFR32_PLATFORM)
#include "em_cmu.h"

/** \brief  Platform specific descriptions for EFR32. */
typedef struct
{
    /** Pointer to platform specific HFXO crystal description
     *  (introduced in bootloader v8).
     */
    const CMU_HFXOInit_TypeDef * hfxoInit;
    /** Pointer to platform specific LFXO crystal description
     *  (introduced in bootloader v8).
     */
    const CMU_LFXOInit_TypeDef * lfxoInit;
} platform_efr32_t;
#endif

/** \brief  Platform specific descriptions. */
typedef union
{
#if defined(NRF52_PLATFORM)
    /** Platform specific descriptions for nRF52.
     *  (dummy, introduced in bootloader v8).
     */
    const void * nrf52;
#elif defined(NRF91_PLATFORM)
    /** Platform specific descriptions for nRF91.
     *  (dummy, introduced in bootloader v8).
     */
    const void * nrf91;
#elif defined(EFR32_PLATFORM)
    /** Platform specific descriptions for EFR32.
     *  (introduced in bootloader v8).
     */
    const platform_efr32_t * efr32;
#endif
} platform_t;

/** \brief  Hardware features that can be installed on a board. */
typedef struct
{
    /** True if 32kHz crystal is present; default:true
     *  (introduced in bootloader v7).
     */
    bool crystal_32k;
    /** True if DCDC converter is enabled; default:true
     * (introduced in bootloader v7).
     */
    bool dcdc;
    /** Platform specific descriptions
     *  (introduced in bootloader v8).
     */
    platform_t platform;
} hardware_capabilities_t;

/**
 * \brief   Returns board hardware capabilities.
 * \return  Return a structure \ref bl_hardware_capabilities_t with
 *          hardware features installed on the board.
 * \note    There is no need to define this function, it is integrated in the
 *          SDK and is controlled from each board/<board_name>/config.mk.
 */
const hardware_capabilities_t * hardware_getCapabilities(void);

#endif //HARDWARE_H_
