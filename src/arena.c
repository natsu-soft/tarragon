#include "arena.h"
#include <assert.h>
#include <stdlib.h>

Arena* arena_init(size_t bytes)
{
	Arena* arena = malloc(sizeof * arena);
	arena->size = 0;
	arena->cap = bytes;
	arena->memory = malloc(bytes);
	return arena;
}

void* arena_alloc(Arena* arena, size_t bytes)
{
	assert(arena->size + bytes <= arena->cap);
	if (arena->size + bytes > arena->cap)
	{
		return NULL;
	}
	else
	{
		void* addr = arena->memory + arena->size;
		arena->size += bytes;
		return addr;
	}
}

void arena_free_tail(Arena* arena, size_t bytes)
{
	assert((int32_t)arena->size - (int32_t)bytes >= 0);
	if ((int32_t)arena->size - (int32_t)bytes < 0)
	{
		return;
	}
	else
	{
		arena->size -= bytes;
	}
}
