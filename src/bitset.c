#include "bitset.h"
#include <stdint.h>



void bitset_add(struct bitset* set, const int32_t value)
{
	set->bits[value >> 3] |= 1u << (value & 0x07);
}

uint8_t bitset_has(struct bitset* set, const int32_t value)
{
	return set->bits[value >> 3] & (1u << (value & 0x07));
}
