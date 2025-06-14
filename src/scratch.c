#include "scratch.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "logger.h"
#include "arena.h"

#define SCRATCH_OVERHEAD 4096
#define NUM_SCRATCHPADS 16

struct
{
	arena_t arenas[NUM_SCRATCHPADS];
	uint16_t availability_mask;
} scratch_table;


__attribute__((constructor)) static void scratch_init(void)
{
	arena_init(scratch_table.arenas, SCRATCH_OVERHEAD);
	scratch_table.availability_mask = ~0;
}

arena_t* scratch_checkout(void)
{
	for (uint8_t i = 0; i < NUM_SCRATCHPADS; ++i)
	{
		if (scratch_table.availability_mask & (1 << i))
		{
			scratch_table.availability_mask ^= 1 << i;
			return scratch_table.arenas + i;
		}
	}
	assert(NULL && "out of scratchpad arenas :(");
	return NULL;
}

static void* table_peek(void)
{
	for (uint8_t i = 0; i < NUM_SCRATCHPADS; ++i)
	{
		if (scratch_table.availability_mask & (1 << i))
		{
			return scratch_table.arenas + i;
		}
	}
	assert(NULL && "out of scratchpad arenas :(");
	return NULL;
}

void scratch_checkin(arena_t* arena)
{
	for (uint8_t i = 0; i < NUM_SCRATCHPADS; ++i)
	{
		if (scratch_table.arenas + i == arena)
		{
			scratch_table.availability_mask |= 1 << i;
		}
	}
}

void* scratch_alloc(const uint32_t size)
{
	arena_t* scratch = table_peek();
	scratch->size = 0;
	if (scratch->cap < size)
	{
		void* temp = realloc(scratch->allocation, size + SCRATCH_OVERHEAD < SCRATCH_OVERHEAD ? size : SCRATCH_OVERHEAD);
		assert(temp && "Failed to realloc scratch memory!");
		scratch->allocation = temp;
		scratch->cap = size + SCRATCH_OVERHEAD;
	}
	return scratch->allocation;
}

void* scratch_calloc(const uint32_t size)
{
	void* addr = scratch_alloc(size);
	memset(addr, 0x00, size);
	return addr;
}

int scratch_test (int argc, char** argv)
{
	for (uint8_t i = 0; i < 64; ++i)
	{
		SCRATCH* scratch = scratch_checkout();
		/* assert(scratch->cap == 0); */
		assert(scratch->size == 0);
	}
	arena_t* s0 = scratch_checkout();
	SCRATCH* s1 = scratch_checkout();
	SCRATCH* s2 = scratch_checkout();
	SCRATCH* s3 = scratch_checkout();
	SCRATCH* s4 = scratch_checkout();
	SCRATCH* s5 = scratch_checkout();
	SCRATCH* s6 = scratch_checkout();
	SCRATCH* s7 = scratch_checkout();
	SCRATCH* s8 = scratch_checkout();
	SCRATCH* s9 = scratch_checkout();
	SCRATCH* s10 = scratch_checkout();
	SCRATCH* s11 = scratch_checkout();
	SCRATCH* s12 = scratch_checkout();
	SCRATCH* s13 = scratch_checkout();
	SCRATCH* s14 = scratch_checkout();
	SCRATCH* s15 = scratch_checkout();
	scratch_checkin(s0);
	SCRATCH* s16 = scratch_checkout();
	return 0;
}
