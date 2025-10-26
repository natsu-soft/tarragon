#include "based_map.h"
#include <assert.h>
#include <string.h>

struct base
{
	uint8_t* addr;
};

// it's lite, it's fast; it'll do.
// and if it dont, just use murmur3
static inline int32_t djb2(const char* str, const int16_t len)
{
	int64_t hash = 5381;
	for (int16_t i = 0; i < len; ++i)
	{
		hash += ((hash << 5) + hash) ^ str[i];
	}
	return hash & INT32_MAX;
}



void based_map_init(based_map_t* map, const struct base* base, uint8_t* allocation, const int32_t cap)
{
	map->base = base;
	map->cap = cap;
	map->offsets = allocation;
	map->values = map->offsets + cap;
	map->lens = map->values + cap;
	map->size = 0;
}

int32_t based_map_cache(based_map_t* map, const int32_t offset, const int16_t len, const int32_t value)
{
	const int32_t cap = map->cap;
	assert(map->size <= cap && "index map out of space.");
	const uint32_t cap_mask = cap - 1;
	const char* key = map->base->addr + offset;
	const int32_t index = djb2(key, len);
	int16_t* lens = map->lens;
	for (int32_t i = 0; i < cap; ++i)
	{
		const int32_t j = (index + i) & cap_mask;
		if (lens[j] == 0)
		{
			map->offsets[j] = offset;
			lens[j] = len;
			map->values[j] = value;
			map->size++;
			return j;
		}
	}
	return -1;
}

int32_t based_map_fetch(based_map_t* map, const char* key, const int16_t len)
{
	const int32_t cap = map->cap;
	const uint32_t cap_mask = cap - 1;
	const int32_t index = djb2(key, len);
	int16_t* lens = map->lens;
	int32_t* offsets = map->offsets;
	const char* addr0 = map->base->addr;
	for (int32_t i = 0; i < cap; ++i)
	{
		const int32_t j = (index + i) & cap_mask;
		const int16_t l = lens[j];
		const char* idx_key = addr0 + offsets[j];
		if (l == len && strncmp(key, idx_key, len) == 0)
		{
			return map->values[j];
		}
		else if (l == 0)
		{
			return -1;
		}
	}
	return -1;
}
