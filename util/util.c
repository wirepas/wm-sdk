#include <stdint.h>
#include "util.h"
#include "crc.h"

//---------------------------------------------------------------------------
// Comparison functions

bool Util_isLtUint32(uint32_t a, uint32_t b)
{
    return ((int32_t) ((a) - (b)) < 0);
}

bool Util_isSmallest(const uint32_t a, const uint32_t b, const uint32_t c)
{
    return (Util_isLtUint32(a, b) &&
            Util_isLtUint32(b, c));
}

bool Util_inBetween(const uint32_t a, const uint32_t b, const uint32_t c)
{
    return ( Util_isGtOrEqUint32(b, a) &&
             Util_isGtOrEqUint32(c, b));
}

uint8_t Util_bitCountU8(uint8_t value)
{
    uint8_t count = 0;

    for (count = 0; value; value >>= 1)
    {
        count += (value & 1);
    }

    return count;
}
