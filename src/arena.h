#ifndef ARENA_H
#define ARENA_H

#include <stddef.h>
#include <stdint.h>

typedef struct Arena {
	size_t size;
	size_t cap;
	uint8_t* memory;
} Arena;

void* arena_alloc(Arena* arena, size_t bytes);

#endif /* End ARENA_H */
