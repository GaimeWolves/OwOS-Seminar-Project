#include "bitmap.h"
// Sets a bit inside the bitmap
static inline void setBit(uint32_t bit, uint32_t bitmap[])
{
    bitmap[bit / 32] |= (1 << (bit % 32));
}

// Clears a bit inside the bitmap
static inline void clearBit(uint32_t bit, uint32_t bitmap[])
{
    bitmap[bit / 32] &= ~(1 << (bit % 32));
}

// Clears a bit inside the bitmap
static inline bool getBit(uint32_t bit, uint32_t bitmap[])
{
    return bitmap[bit / 32] & (1 << (bit % 32));
}
