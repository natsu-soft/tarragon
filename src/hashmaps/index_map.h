#ifndef INDEX_MAP_H
#define INDEX_MAP_H

// yes, it's map32 but with a much better name

#include <stdint.h>

typedef struct index_map_t
{
	char** keys;
	int32_t* values;
	int16_t* lens;
	int32_t cap;
	int32_t size;
} index_map_t;

void index_map_init(index_map_t* map, uint8_t* allocation, const int32_t cap);

int32_t index_map_cache(index_map_t* map, char* key, const int16_t len, const int32_t value);

int32_t index_map_fetch(index_map_t* map, char* key, const int16_t len);


#endif /* End INDEX_MAP_H */
