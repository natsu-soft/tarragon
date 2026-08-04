#include "token_id_map.h"
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include "token_ids_types.h"

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

void token_id_map_init(token_id_map_t* map, uint8_t* allocation, const int16_t cap)
{
	assert(cap > 0 && (cap & (cap - 1)) == 0 && "hashmaps should be a power of two!");
	map->cap = cap;
	map->keys = allocation;
	map->lens = map->keys + cap;
	map->values = map->lens + cap;
	memset(map->lens, 0x00, cap * sizeof *map->lens);
	map->size = 0;
}

// THIS ONE uses round-robin caching (think about it--this is just used during initialization, so I want turbofast fetching)
void token_id_map_cache(token_id_map_t* map, char* key, int8_t len, token_id_t value)
{
	const int16_t cap = map->cap;
	assert(map->size <= cap && "gdscript_id map out of space.");
	const uint16_t cap_mask = cap - 1;
	const int16_t index = djb2(key, len) & cap_mask;
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
		else if (lens[j] > len)
		{
			char* temp_key = map->keys[j];
			int8_t temp_len = lens[j];
			token_id_t temp_value = map->values[j];
			map->keys[j] = key;
			lens[j] = len;
			map->values[j] = value;
			key = temp_key;
			len = temp_len;
			value = temp_value;
		}
	}
}

token_id_t token_id_map_fetch(const token_id_map_t* map, const char* key, const int8_t len)
{
	const int16_t cap = map->cap;
	const uint16_t cap_mask = cap - 1;
	const int16_t index = djb2(key, len) & cap_mask;
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
			break;
		}
	}
	return GDSCRIPT_NONE;
}

void token_id_map_print(const token_id_map_t* map)
{
	const int16_t cap_mask = map->cap - 1;
	for (int16_t i = 0; i < map->cap; ++i)
	{
		const int8_t l = map->lens[i];
		if (l > 0)
		{
			printf("%d(%04x): '%.*s'\n", i, djb2(map->keys[i], l) & cap_mask, l, map->keys[i]);
		}
	}
}
