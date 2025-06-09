#include "scratch.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "arena.h"


#define SCRATCH_OVERHEAD 4096

static arena_t scratch = {0};

static void __attribute__((constructor)) scratch_init(void)
{
	arena_init(&scratch, 2 * SCRATCH_OVERHEAD);
}

void* scratch_alloc(const size_t bytes)
{
	scratch.size = 0;
	if (scratch.cap < bytes)
	{
		void* temp = realloc(scratch.memory, bytes + SCRATCH_OVERHEAD);
		assert(temp && "Failed to realloc scratch memory!");
		scratch.memory = temp;
	}
	return scratch.memory;
}

void* scratch_calloc(const size_t bytes)
{
	void* addr = scratch_alloc(bytes);
	memset(addr, 0x00, bytes);
	return addr;
}

arena_t* scratch_arena()
{
	return &scratch;
}
