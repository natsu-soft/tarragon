#ifndef ARENA_H
#define ARENA_H

#include <stddef.h>
#include <stdint.h>

typedef struct Arena
{
	int32_t size;
	size_t cap;
	uint8_t* memory;
} Arena;

Arena* arena_init(size_t bytes);

void* arena_alloc(Arena* arena, size_t bytes);

void arena_free_tail(Arena* arena, size_t bytes);

#endif /* End ARENA_H */
