#ifndef ARENA_TYPES_H
#define ARENA_TYPES_H

#include <stdint.h>

typedef struct arena_t
{
	uint8_t* addr;
	int32_t size;
	int32_t cap;
} arena_t;


#endif /* End ARENA_TYPES_H */
