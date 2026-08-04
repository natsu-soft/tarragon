#include "ptr_set.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
/* #include <stdlib.h> */

// it's lite, it's fast; it'll do.
static inline int32_t djb2(uint64_t ptr)
{
	int64_t hash = 5381;
	union
	{
		uint8_t byte[8];
		uint64_t full;
	} pointer;
	pointer.full = ptr;
	for (int8_t i = 7; i >= 0; --i)
	{
		hash += ((hash << 5) + hash) + pointer.byte[i];
	}
	return hash & INT32_MAX;
}

const int32_t ptr_hash(const void* ptr, const int32_t cap)
{
	return djb2(ptr) & (cap - 1);
}


void ptr_set_init(struct ptr_set* set, uint8_t* allocation, const int32_t cap)
{
	assert(cap > 0 && (cap & (cap - 1)) == 0 && "ptrset should be a power of two!");
	set->cap = cap;
	set->pointers = allocation;
	set->bits = set->pointers + cap;
	memset(set->bits, 0x00, cap);
	set->size = 0;
}

void ptr_set_add(struct ptr_set* set, void* ptr)
{
	const int32_t cap = set->cap;
	assert(set->size <= cap);
	const uint32_t cap_mask = cap - 1;
	const int32_t idx = djb2((uint64_t)ptr);
	for (int32_t i = 0; i < cap; ++i)
	{
		const int32_t j = (idx + i) & cap_mask;
		if ((set->bits[j >> 6] & (1 << (j & 0x3f))) == 0)
		{
			set->pointers[j] = ptr;
			assert(ptr == set->pointers[j]);
			set->bits[j >> 6] |= (1 << (j & 0x3f));
			printf(" - bitset[%d]: %#018lx\n", j >> 6, set->bits[j >> 6]);
			++set->size;
			return;
		}
	}
	assert(0 && "ptr_set overflow!");
}

uint64_t ptr_set_has(struct ptr_set* set, const void* ptr)
{
	const int32_t cap = set->cap;
	assert(set->size <= cap);
	const uint32_t cap_mask = cap - 1;
	const int32_t idx = djb2((uint64_t)ptr);
	/* printf("> djb2: %x\n", idx); */
	for (int32_t i = 0; i < cap; ++i)
	{
		const int32_t j = (idx + i) & cap_mask;
		/* if ((set->bits[j >> 6] & (1u << (j & 0x3f))) && set->pointers[j] == ptr) */
		if ((set->bits[j >> 6] & (1 << (j & 0x3f))) && set->pointers[j] == ptr)
		{
			return set->pointers[j] == ptr;
		}
		else if (set->bits[j >> 6] ^ (1 << (j & 0x3f)))
		{
			return 0;
		}
	}
	return 0;
}
