#include "scratch.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "arena.h"
#include "pool.h"

#define SCRATCH_OVERHEAD 4096
#define NUM_SCRATCHPADS0 16

struct {
	void* allocation;
	size_t cap;
} scratch = {0};

static pool_t scratch_pool;

static void __attribute__((constructor)) scratch_init(void)
{
	scratch.allocation = malloc(SCRATCH_OVERHEAD);
	scratch.cap = SCRATCH_OVERHEAD;
	pool_init(&scratch_pool, sizeof(arena_t), NUM_SCRATCHPADS0);
	/* arena_init(scratch_pool.allocation, SCRATCH_OVERHEAD); */
	/* scratchpad = calloc(NUM_SCRATCHPADS0, sizeof *scratchpad); */
	/* arena_init(scratchpad, SCRATCH_OVERHEAD); */
}

void* scratch_alloc(const size_t bytes)
{
	if (scratch.cap < bytes)
	{
		void* temp = realloc(scratch.allocation, bytes + SCRATCH_OVERHEAD);
		assert(temp && "Failed to realloc scratch memory!");
		scratch.allocation = temp;
		scratch.cap = bytes + SCRATCH_OVERHEAD;
	}
	return scratch.allocation;
}

void* scratch_calloc(const size_t bytes)
{
	void* addr = scratch_alloc(bytes);
	memset(addr, 0x00, bytes);
	return addr;
}

arena_t* checkout_scratch_arena(void)
{
	arena_t* res = pool_alloc(&scratch_pool);
	if (res->cap == 0)
	{
		arena_init(res, SCRATCH_OVERHEAD);
	}
	return res;
}

void checkin_scratch_arena(struct arena_t* scratch_arena)
{
	pool_free(&scratch_pool, scratch_arena);
}
