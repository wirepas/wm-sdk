/* Copyright 2017 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

#include <stdlib.h> // For NULL
#include <stdint.h> // For uint32_t

#include "lock_bits.h"
#include "api.h"

bool LockBits_isKeySet(void)
{
    return (lib_settings->getFeatureLockKey(NULL)) == APP_RES_OK;
}

bool LockBits_isFeaturePermitted(uint32_t lock_bits)
{
    if (!LockBits_isKeySet())
    {
        // No key set, so all requested features unlocked
        return true;
    }

    uint32_t active_lock_bits = UINT32_MAX;
    lib_settings->getFeatureLockBits(&active_lock_bits);

    if ((active_lock_bits & lock_bits) == lock_bits)
    {
        // All requested features unlocked
        return true;
    }

    // One or more requested features locked
    return false;
}
