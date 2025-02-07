#include "arena.h"
#include <assert.h>
#include <stdlib.h>
#include "error_macros.h"
#include "logger.h"

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
	/* LOG("allocating %zu from %zu", bytes, arena-> cap - arena->size); */
	fassertf(log_fp, arena->size + bytes <= arena->cap, "ARENA OVERFLOW!");
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

void arena_free(Arena* arena, size_t bytes)
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
