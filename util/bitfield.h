#ifndef BITFIELD_H
#define BITFIELD_H

typedef uint8_t bitfield_t;

#define BITFIELD_T_SIZE (sizeof(bitfield_t) * 8)

#define BITFIELD_SET(arr, bit) \
  ((arr)[ ((bit) / BITFIELD_T_SIZE ) ]) |= (1 << ((bit) % BITFIELD_T_SIZE))

// BitField_clear(table,0) -> clear the 1st bit in array "table"
#define BITFIELD_CLEAR(arr, bit) \
  ((arr)[ ((bit) / BITFIELD_T_SIZE ) ]) &= ~(1 << ((bit) % BITFIELD_T_SIZE))

#define BITFIELD_GET(arr, bit) \
  (((arr)[ ((bit) / BITFIELD_T_SIZE) ]) & (1 << ((bit) % BITFIELD_T_SIZE)))

#endif
