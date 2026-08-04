#include "arena.h"
#include <assert.h>
#include <stdlib.h>
#include "../error_macros.h"
/* #include "../logger.h" */

#define REALLOC_OVERHEAD 4096

void arena_init(arena_t* arena, const int32_t bytes)
{
	arena->size = 0;
	arena->cap = bytes;
	arena->allocation = malloc(bytes);
}

void arena_grow(arena_t* arena, const int32_t new_size)
{
	if (arena->cap >= new_size) return;
	uint8_t* allocation = realloc(arena->allocation, new_size + REALLOC_OVERHEAD);
	assert(allocation != NULL && "failed to grow arena!");
	arena->allocation = allocation;
	arena->cap = new_size + REALLOC_OVERHEAD;
}

void* arena_alloc(arena_t* arena, const int32_t bytes)
{
	/* LOG("allocating %zu from %zu", bytes, arena-> cap - arena->size); */
	/* fassertf(log_fp, arena->size + bytes <= arena->cap, "ARENA OVERFLOW!"); */
	if (arena->size + bytes > arena->cap)
	{
		return NULL;
	}
	else
	{
		void* addr = arena->allocation + arena->size;
		arena->size += bytes;
		return addr;
	}
}

void* arena_dynamic_alloc(arena_t* arena, const int32_t bytes)
{
	if (arena->size + bytes > arena->cap)
	{
		arena_grow(arena, arena->size + bytes);
	}
	return arena_alloc(arena, bytes);
}


void* arena_calloc(arena_t* arena, const int32_t bytes)
{
	void* res = arena_alloc(arena, bytes);
	memset(res, 0, bytes);
	return res;
}

void* arena_alloc_aligned(arena_t* arena, const int32_t bytes, const size_t alignment)
{
	// assert alignment is a power of 2 BECAUSE OTHERWISE THIS JUST DONT WORK
	assert((alignment & (alignment - 1)) == 0);
	const size_t arena_addr = (size_t)arena->allocation + arena->size;
	const size_t mod = arena_addr & (alignment - 1);
	const int32_t padding = mod > 0 ? alignment - mod : 0;
	assert(arena->size + bytes + mod <= arena->cap);
	void* addr = arena->allocation + arena->size + padding;
	arena->size += bytes + padding;
	return addr;
}

void arena_align2(arena_t* arena, const int32_t alignment)
{
	assert((alignment & (alignment - 1)) == 0);
	const size_t addr = (size_t)arena->allocation + arena->size;
	const size_t mod = addr & (alignment - 1);
	arena->size += mod > 0 ? alignment - mod : 0;
}


void arena_free(arena_t* arena, const int32_t bytes)
{
	assert(arena->size >= bytes);
	if (arena->size < bytes)
	{
		return;
	}
	else
	{
		arena->size -= bytes;
		assert(arena->size >= 0);
	}
}

void arena_free_all(arena_t* arena)
{
	arena->size = 0;
}


void* arena_pop(arena_t* arena, const int32_t bytes)
{
	assert(arena->size >= bytes);
	arena->size -= bytes;
	return arena->allocation + arena->size;
}

void* arena_push(arena_t* arena, const void* src, const int32_t bytes)
{
	assert(arena->cap >= arena->size + bytes);
	void* dest = arena->allocation + arena->size;
	/* printf("arena_push: %x from %x\n", dest, arena); */
	memcpy(dest, src, bytes);
	arena->size += bytes;
	return dest;
}

void arena_dynamic_push(arena_t* arena, const void* src, const int32_t bytes)
{
	if (arena->size + bytes > arena->cap)
	{
		arena_grow(arena, arena->size + bytes);
	}
	arena_push(arena, src, bytes);
}


void* arena_peek(arena_t* arena, const int32_t bytes)
{
	return arena->allocation + arena->size - bytes;
}

void arena_cat(arena_t* arena0, const arena_t* arena1)
{
	if (arena0->size + arena1->size > arena0->cap)
	{
		uint8_t* addr = realloc(arena0->allocation, arena0->size + arena1->size);
		assert(addr != NULL && "failed to reallocate memory for arena in arena_cat()");
		arena0->allocation = addr;
		arena0->cap = arena0->size + arena1->size;
		/* arena_grow(arena0, arena0->size + other->size); */
	}
	memcpy(arena0->allocation + arena0->size, arena1->allocation, arena1->size);
	arena0->size += arena1->size;
}


void arena_insert(arena_t* arena, const int32_t pos, const void* src, const int32_t bytes)
{
	if (arena->size + bytes > arena->cap)
	{
		arena_grow(arena, arena->size + bytes);
	}
	// TODO: some kind of assert
	memmove(arena->allocation + pos + bytes, arena->allocation + pos, arena->size - pos);
	memcpy(arena->allocation + pos, src, bytes);
}
