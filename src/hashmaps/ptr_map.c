#include "ptr_map.h"
/* #include <stdlib.h> */
#include <stdint.h>
#include <assert.h>
#include <string.h>

// it's lite, it's fast; it'll do.
static inline int16_t djb2(const char* str, const int16_t len)
{
	int64_t hash = 5381;
	for (int16_t i = 0; i < len; ++i)
	{
		hash += ((hash << 5) + hash) + str[i];
	}
	return hash & INT16_MAX;
}

void ptr_map_init(ptr_map_t* map, uint8_t* allocation, const int16_t cap)
{
	assert(cap > 0 && (cap & (cap - 1)) == 0 && "hashmaps should be a power of two!");
	map->cap = cap;
	map->keys = allocation;
	map->lens = map->keys + cap;
	map->values = map->lens + cap;
	memset(map->lens, 0x00, cap * sizeof *map->lens);
	map->size = 0;
}



void ptr_map_cache(ptr_map_t* map, char* key, const int8_t len, const void* value)
{
	const int16_t cap = map->cap;
	assert(map->size <= cap && "pointer map out of space.");
	const uint16_t cap_mask = cap - 1;
	const int16_t index = djb2(key, len);
	int8_t* lens = map->lens;
	for (int16_t i = 0; i < cap; ++i)
	{
		const int16_t j = (index + i) & cap_mask;
		if (lens[j] == 0)
		{
			map->keys[j] = key;
			lens[j] = len;
			map->values[j] = value;
			map->size++;
			return;
		}
	}
}

void* ptr_map_fetch(const ptr_map_t* map, const char* key, const int8_t len)
{
	const int16_t cap = map->cap;
	const uint16_t cap_mask = cap - 1;
	const int16_t index = djb2(key, len);
	int8_t* lens = map->lens;
	char** keys = map->keys;
	for (int16_t i = 0; i < cap; ++i)
	{
		const int16_t j = (index + i) & cap_mask;
		const int8_t l = lens[j];
		if (l == len && strncmp(key, keys[j], len) == 0)
		{
			return map->values[j];
		}
		else if (l == 0)
		{
			return NULL;
		}
	}
	return NULL;
}
