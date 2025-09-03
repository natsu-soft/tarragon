#ifndef MAP32_H
#define MAP32_H


#include <stdint.h>

// SoA OwO
// use relative ptrs u mong
typedef struct map32_t
{
	int32_t* rkeys;
	int32_t* values;
	int16_t* lens;
	int32_t cap;
	int32_t size;
} map32_t;


void map32_init(map32_t* map, const int32_t pow2cap);

void map32_cache(map32_t* map, const char* key, const int32_t len, const int32_t value, const char* key_base);

int32_t map32_fetch(map32_t* map, const char* key, const int32_t len, char* key_base);

void map32_print(map32_t* map, char* key_base);


#endif /* End MAP32_H */
