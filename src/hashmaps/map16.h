#ifndef MAP16_H
#define MAP16_H

#include<stdint.h>

typedef struct map16_t
{
	char** keys;
	int16_t* values;
	int16_t* lens;
	int32_t cap;
	int32_t size;
} map16_t;

void map16_init(map16_t* map, const int32_t pow2cap, uint8_t* allocation);

void map16_cache(map16_t* map, const char* key, const int16_t len, const int32_t value);

int16_t map16_fetch(map16_t* map, const char* key, const int16_t len);

void map16_print(const map16_t* map);

#endif /* End MAP16_H */
