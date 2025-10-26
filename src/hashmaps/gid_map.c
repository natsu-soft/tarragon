#include "gid_map.h"
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

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

void gid_map_init(gid_map_t* map, uint8_t* allocation, const int16_t cap)
{
	assert(cap > 0 && (cap & (cap - 1)) == 0 && "hashmaps should be a power of two!");
	map->cap = cap;
	map->keys = allocation;
	map->lens = map->keys + cap;
	map->values = map->lens + cap;
	memset(map->lens, 0x00, cap);
	map->size = 0;
}

void gid_map_cache(gid_map_t* map, char* key, const int8_t len, const gdscript_id_t value)
{
	const int16_t cap = map->cap;
	assert(map->size <= cap && "gdscript_id map out of space.");
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

gdscript_id_t gid_map_fetch(const gid_map_t* map, const char* key, const int8_t len)
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
			return GDSCRIPT_NONE;
		}
	}
	return GDSCRIPT_NONE;
}

void gid_map_print(const gid_map_t* map)
{
	char temp[32];
	for (int16_t i = 0; i < map->cap; ++i)
	{
		const int8_t l = map->lens[i];
		if (l > 0)
		{
			memcpy(temp, map->keys[i], l);
			temp[l] = '\0';
			printf("%d: %s\n", i, temp);
		}
	}
}
