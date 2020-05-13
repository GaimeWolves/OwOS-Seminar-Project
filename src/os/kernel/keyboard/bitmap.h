#pragma once
#include <stdint.h>
#include <stdbool.h>
void setBit(uint32_t bit, uint32_t bitmap[]);
void clearBit(uint32_t bit, uint32_t bitmap[]);
bool getBit(uint32_t bit, uint32_t bitmap[]);
