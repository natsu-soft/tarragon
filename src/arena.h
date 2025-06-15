#ifndef ARENA_H
#define ARENA_H

#include <stddef.h>
#include <stdint.h>

typedef struct arena_t
{
	uint8_t* allocation;
	uint32_t cap;
	uint32_t size;
} arena_t;

void arena_init(arena_t* arena, const size_t bytes);

void arena_grow(arena_t* arena, const size_t bytes);

void* arena_alloc(arena_t* arena, const size_t bytes);

void* arena_dynamic_alloc(arena_t* arena, const size_t bytes);

void* arena_calloc(arena_t* arena, const size_t bytes);

void* arena_alloc_aligned(arena_t* arena, const size_t bytes, const size_t alignment);

void arena_align2(arena_t* arena, const size_t alignment);

void arena_free(arena_t* arena, const size_t bytes);

// for stack-like behavior
void* arena_pop(arena_t* arena, const size_t bytes);

void arena_push(arena_t* arena, const void* src, const size_t bytes);

void arena_dynamic_push(arena_t* arena, const void* src, const size_t bytes);

void* arena_peek(arena_t* arena, const size_t bytes);

void arena_cat(arena_t* arena0, const arena_t* arena1);

#endif /* End ARENA_H */
