#include "arena.h"
#include <stdint.h>
#include <assert.h>
#include <stdlib.h>


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
