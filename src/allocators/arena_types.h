#ifndef ARENA_TYPES_H
#define ARENA_TYPES_H

#include <stdint.h>

typedef struct arena_t
{
	uint8_t* allocation;
	int32_t cap;
	int32_t size;
} arena_t;


#endif /* End ARENA_TYPES_H */
