#include "op_map.h"
/* #include <stdlib.h> */
#include <stdint.h>
#include <assert.h>
#include <string.h>


static inline int16_t hash(const int16_t value)
{
	return ((value << 5) + value) & INT16_MAX;
}

void op_map_init(op_map_t* map, uint8_t* allocation, const int16_t cap)
{
	assert(cap > 0 && (cap & (cap - 1)) == 0 && "hashmaps should be a power of two!");
	map->cap = cap;
	map->values = allocation;
	map->keys = map->values + cap;
	memset(map->keys, 0x00, cap * sizeof *map->keys);
	map->size = 0;
}

void op_map_cache(op_map_t* map, const gdscript_id_t key, const void* value)
{
	const int16_t cap = map->cap;
	assert(map->size <= cap && "pointer map out of space.");
	const uint16_t cap_mask = cap - 1;
	const int16_t index = hash(key);
	gdscript_id_t* keys = map->keys;
	for (int16_t i = 0; i < cap; ++i)
	{
		const int16_t j = (index + i) & cap_mask;
		if (keys[j] == GDSCRIPT_NONE)
		{
			map->keys[j] = key;
			map->values[j] = value;
			map->size++;
			return;
		}
	}
}

void* op_map_fetch(op_map_t* map, const gdscript_id_t key)
{
	const int16_t cap = map->cap;
	const uint16_t cap_mask = cap - 1;
	const int16_t index = hash(key);
	gdscript_id_t* keys = map->keys;
	for (int16_t i = 0; i < cap; ++i)
	{
		const int16_t j = (index + i) & cap_mask;
		if (keys[j] == key)
		{
			return map->values[j];
		}
		else if (keys[j] == GDSCRIPT_NONE)
		{
			return NULL;
		}
	}
	return NULL;
}
