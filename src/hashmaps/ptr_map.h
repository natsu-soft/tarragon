#ifndef PTR_MAP_H
#define PTR_MAP_H

#include <stdint.h>


typedef struct ptr_map_t
{
	void** values;
	char** keys;
	int8_t* lens;
	int16_t cap;
	int16_t size;
} ptr_map_t;

void ptr_map_init(ptr_map_t* map, uint8_t* allocation, const int16_t cap);

void ptr_map_cache(ptr_map_t* map, char* key, const int8_t len, const void* value);

void* ptr_map_fetch(const ptr_map_t* map, const char* key, const int8_t len);

#endif /* End PTR_MAP_H */
