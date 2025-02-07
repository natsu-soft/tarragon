#ifndef ARENA_H
#define ARENA_H

#include <stddef.h>
#include <stdint.h>

typedef struct arena
{
	uint32_t size;
	size_t cap;
	uint8_t* memory;
} arena_t;

arena_t* arena_init(size_t bytes);

void* arena_alloc(arena_t* arena, size_t bytes);

void arena_free(arena_t* arena, size_t bytes);

#endif /* End ARENA_H */
