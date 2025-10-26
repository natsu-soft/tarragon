#ifndef BASED_MAP_H
#define BASED_MAP_H

#include <stdint.h>

typedef struct based_map_t
{
	struct base* base;
	int32_t* offsets;
	int32_t* values;
	int16_t* lens;
	int32_t cap;
	int32_t size;
} based_map_t;

void based_map_init(based_map_t* map, const struct base* base, uint8_t* allocation, const int32_t cap);

int32_t based_map_cache(based_map_t* map, const int32_t, const int16_t len, const int32_t value);

int32_t based_map_fetch(based_map_t* map, const char* key, const int16_t len);

#endif /* End BASED_MAP_H */
