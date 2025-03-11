#ifndef ARENA_H
#define ARENA_H

#include <stddef.h>
#include <stdint.h>

typedef struct arena_t
{
	size_t cap;
	uint8_t* memory;
	uint32_t size;
} arena_t;

void arena_init(arena_t* arena, size_t bytes);

void* arena_alloc(arena_t* arena, size_t bytes);

void arena_free(arena_t* arena, size_t bytes);

#endif /* End ARENA_H */
