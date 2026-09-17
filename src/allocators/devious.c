#include "devious.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "arena.h"


#define DEVIOUS_OVERHEAD 4096
#define DEVIOUS0 8

static arena_t* available;


/* __attribute__((constructor)) */
void absolutely_devious(void)
{
	arena_t* arenas = calloc(DEVIOUS0, sizeof *arenas);
	arena_init(arenas, DEVIOUS_OVERHEAD);
	for (int32_t i = 0; i < DEVIOUS0 - 1; ++i)
	{
		arenas[i].size = sizeof *arenas;
	}
	available = arenas;
}

static void deviousness_continued(const int32_t size)
{
	arena_t* arenas = calloc(DEVIOUS0, sizeof *arenas);
	arena_init(arenas, size + DEVIOUS_OVERHEAD);
	for (int32_t i = 0; i < DEVIOUS0 - 1; ++i)
	{
		arenas[i].size = sizeof *arenas;
	}
	available = arenas;
}

arena_t* devious_checkout(void)
{
	if (!available)
	{
		absolutely_devious();
	}
	arena_t* res = available;
	available = res->size > 0 ? (uint8_t*)res + res->size : NULL;
	res->size = 0;
	return res;
}

void devious_checkin(arena_t* arena)
{
	// >:^)
	const int64_t delta = (uint8_t*)available - (uint8_t*)arena;
	assert(INT32_MAX >= delta && delta >= INT32_MIN && "NO MORE DEVILRY! REPENT IN CHRIST!");
	arena->size = (int32_t)(((delta & 0x8000000000000000) >> 32) | (delta & 0x7fffffff));
	available = arena;
}

arena_t* devious_sized_checkout(const int32_t size)
{
	if (!available)
	{
		deviousness_continued(size);
	}
	arena_t* res = available;
	available = res->size > 0 ? res + res->size : NULL;
	res->size = 0;
	if (res->cap < size)
	{
		arena_grow(res, size);
	}
	return res;
}

void* devious_alloc(const int32_t size)
{
	if (!available)
	{
		deviousness_continued(size);
	}
	else if (available->cap < size)
	{
		arena_grow(available, size);
	}
	return available->allocation;
}

void* devious_calloc(const int32_t size)
{
	void* ptr = devious_alloc(size);
	memset(ptr, 0x00, size);
	return ptr;
}

int devious_test(int arc, char** argv)
{
	for (int i = 0; i < 64; ++i)
	{
		DEVIOUS* scratch = devious_checkout();
	}
	arena_t** arenas = alloca(128 * sizeof *arenas);
	for (int i = 0; i < 128; ++i)
	{
		arenas[i] = devious_sized_checkout(1024);
	}
	for (int i = 0; i < 128; ++i)
	{
		devious_checkin(arenas[i]);
	}
	for (int i = 0; i < 128; ++i)
	{
		arenas[i] = devious_sized_checkout(4096);
	}
	for (int i = 0; i < 128; ++i)
	{
		devious_checkin(arenas[i]);
	}
	return 0;
}
