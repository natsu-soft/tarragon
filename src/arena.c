#include "arena.h"
#include <assert.h>
#include <stdlib.h>
#include "error_macros.h"
#include "logger.h"

void arena_init(arena_t* arena, size_t bytes)
{
	arena->size = 0;
	arena->cap = bytes;
	arena->memory = malloc(bytes);
}

void* arena_alloc(arena_t* arena, size_t bytes)
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

void* arena_calloc(arena_t* arena, size_t bytes)
{
	void* res = arena_alloc(arena, bytes);
	memset(res, 0, bytes);
	return res;
}

void* arena_alloc_aligned(arena_t* arena, size_t bytes, size_t alignment)
{
	// assert alignment is a power of 2 BECAUSE OTHERWISE THIS JUST DONT WORK
	assert((alignment & (alignment - 1)) == 0);
	size_t arena_addr = (size_t)arena->memory + arena->size;
	size_t mod = arena_addr % (alignment - 1);
	assert(arena->size + bytes + mod <= arena->cap);
	void* addr = arena->memory + arena->size + mod;
	arena->size += mod + bytes;
	return addr;
}


void arena_free(arena_t* arena, size_t bytes)
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

void* arena_pop(arena_t* arena, size_t bytes)
{
	assert((int32_t)arena->size - (int32_t)bytes >= 0);
	arena->size -= bytes;
	return arena->memory + arena->size;
}
