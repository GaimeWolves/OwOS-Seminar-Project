#include "bitmap.h"
// Sets a bit inside the bitmap
void setBit(uint32_t bit, uint32_t bitmap[])
{
    bitmap[bit / 32] |= (1 << (bit % 32));
}

// Clears a bit inside the bitmap
void clearBit(uint32_t bit, uint32_t bitmap[])
{
    bitmap[bit / 32] &= ~(1 << (bit % 32));
}

// Read a bit from the bitmap
bool getBit(uint32_t bit, uint32_t bitmap[])
{
    return bitmap[bit / 32] & (1 << (bit % 32));
}
