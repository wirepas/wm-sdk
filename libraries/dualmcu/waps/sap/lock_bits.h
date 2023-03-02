/* Copyright 2017 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/**
 * \file lock_bits.h
 *
 * App side handling of feature lock bits
 */
#ifndef LOCK_BITS_H_
#define LOCK_BITS_H_

#include <stdint.h>
#include <stdbool.h>

/**
 * \brief   Feature lock bits
 *
 * \note    A set bit means that the feature is not locked (all set by default)
 * \note    These bits try to follow the order and gaps of
 *          Dual-MCU API primitive IDs
 */
typedef enum
{
    /** Prevent sending data with DATA_TX and DATA_TX_TT requests */
    LOCK_BITS_DSAP_DATA_TX              = 0x00000001,
    /** Prevent starting stack*/
    LOCK_BITS_MSAP_STACK_START          = 0x00000004,
    /** Prevent stopping stack */
    LOCK_BITS_MSAP_STACK_STOP           = 0x00000008,
    /** Prevent setting app config data */
    LOCK_BITS_MSAP_APP_CONFIG_WRITE     = 0x00000010,
    /** Prevent reading app config data */
    LOCK_BITS_MSAP_APP_CONFIG_READ      = 0x00000020,
    /** Prevent writing MSAP attributes */
    LOCK_BITS_MSAP_ATTR_WRITE           = 0x00000040,
    /** Prevent reading certain MSAP attributes */
    LOCK_BITS_MSAP_ATTR_READ            = 0x00000080,
    /** Prevent writing CSAP attributes (except feature lock key) */
    LOCK_BITS_CSAP_ATTR_WRITE           = 0x00000100,
    /** Prevent reading certain CSAP attributes */
    LOCK_BITS_CSAP_ATTR_READ            = 0x00000200,
    /** Prevent performing factory reset */
    LOCK_BITS_CSAP_FACTORY_RESET        = 0x00001000,
    /** Prevent scratchpad write and read operations */
    LOCK_BITS_MSAP_SCRATCHPAD_START     = 0x00002000,
    /** Prevent reading scratchpad status (includes CSAP and MSAP attributes) */
    LOCK_BITS_MSAP_SCRATCHPAD_STATUS    = 0x00008000,
    /** Prevent reading neighbor information (includes MSAP attributes) */
    LOCK_BITS_MSAP_GET_NBORS            = 0x00200000,
    LOCK_BITS_MSAP_SCAN_NBORS           = 0x00400000,
    /** Prevent affecting the sink cost */
    LOCK_BITS_MSAP_SINK_COST_WRITE      = 0x02000000,
    /** Prevent reading the sink cost */
    LOCK_BITS_MSAP_SINK_COST_READ       = 0x04000000,
    /** Prevent sending Remote API requests */
    LOCK_BITS_REMOTE_API_TX             = 0x20000000,
    /** Prevent participating in OTAP operations */
    LOCK_BITS_OTAP                      = 0x80000000,
} lock_bits_e;

/**
 * \brief   Check if a feature lock key is set
 * \return  True if feature lock key is set, false otherwise
 */
bool LockBits_isKeySet(void);

/**
 * \brief   Check if a feature is permitted
 * \param   lock_bits
 *          One or more lock bits to check
 * \return  True if none of the lock bits are locked, false otherwise
 */
bool LockBits_isFeaturePermitted(uint32_t lock_bits);

#endif  /* LOCK_BITS_H_ */
