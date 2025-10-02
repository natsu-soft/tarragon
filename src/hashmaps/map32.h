#ifndef MAP32_H
#define MAP32_H


#include <stdint.h>

// SoA OwO
// use relative ptrs u mong
typedef struct map32_t
{
	char** keys;
	int32_t* values;
	int16_t* lens;
	int32_t cap;
	int32_t size;
} map32_t;


void map32_init(map32_t* map, const int32_t pow2cap, uint8_t* allocation);

void map32_cache(map32_t* map, const char* key, const int16_t len, const int32_t value);

int32_t map32_fetch(map32_t* map, const char* key, const int16_t len);

void map32_print(map32_t* map, char* key_base);


#endif /* End MAP32_H */
