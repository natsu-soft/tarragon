#include "index_map.h"
#include <stdint.h>
#include <string.h>
#include <assert.h>

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


void index_map_init(index_map_t* map, uint8_t* allocation, const int32_t cap)
{
	assert(cap > 0 && (cap & (cap - 1)) == 0 && "hashmaps should be a power of two!");
	map->cap = cap;
	map->keys = allocation;
	map->values = map->keys + cap;
	map->lens = map->values + cap;
	map->size = 0;
}

int32_t index_map_cache(index_map_t* map, char* key, const int16_t len, const int32_t value)
{
	const int32_t cap = map->cap;
	assert(map->size <= cap && "index map out of space.");
	const uint32_t cap_mask = cap - 1;
	const int32_t index = djb2(key, len);
	int16_t* lens = map->lens;
	for (int32_t i = 0; i < cap; ++i)
	{
		const int32_t j = (index + i) & cap_mask;
		if (lens[j] == 0)
		{
			map->keys[j] = key;
			lens[j] = len;
			map->values[j] = value;
			map->size++;
			return j;
		}
	}
	return -1;
}

int32_t index_map_fetch(index_map_t* map, char* key, const int16_t len)
{
	const int32_t cap = map->cap;
	const uint32_t cap_mask = cap - 1;
	const int32_t index = djb2(key, len);
	int16_t* lens = map->lens;
	char** keys = map->keys;
	for (int32_t i = 0; i < cap; ++i)
	{
		const int32_t j = (index + i) & cap_mask;
		const int16_t l = lens[j];
		if (l == len && strncmp(key, keys[j], len) == 0)
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
