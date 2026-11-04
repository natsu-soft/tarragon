#include "bitset.h"
#include <stdint.h>
#include <stdatomic.h>



void bitset_add(struct bitset* set, const int32_t value)
{
	atomic_fetch_or_explicit(set->bits + (value >> 3) , 1 << (value & 0x07), memory_order_release);
}

uint8_t bitset_has(struct bitset* set, const int32_t value)
{
	return atomic_load_explicit(set->bits + (value >> 3), memory_order_acquire) & (1u << (value & 0x07));
}
