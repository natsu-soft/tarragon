#include "scratch.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "../logger.h"
#include "arena.h"

#define SCRATCH_OVERHEAD 4096
#define NUM_SCRATCHPADS 16

static struct
{
	arena_t* arenas;
	uint16_t availability;
} scratch_table = {0};


__attribute__((constructor))
void scratch_init(void)
{
	scratch_table.arenas = calloc(NUM_SCRATCHPADS, sizeof(*scratch_table.arenas));
	assert(scratch_table.arenas && "scratch_init(): failed to allocate scratch arenas! How are you ALREADY out of memory?");
	arena_init(scratch_table.arenas, SCRATCH_OVERHEAD);
	scratch_table.availability = ~0;
}

arena_t* scratch_checkout(void)
{
	for (int8_t i = 0; i < NUM_SCRATCHPADS; ++i)
	{
		if (scratch_table.availability & (1 << i))
		{
			scratch_table.availability ^= 1 << i;
			arena_t* res = scratch_table.arenas + i;
			res->size = 0;
			return res;
		}
	}
	ERROR("OUT OF SCRATCHPADS!");
	assert(0 && "out of scratchpad arenas :(");
	return NULL;
}

struct arena_t* scratch_checkout_size(const int32_t size)
{
	int8_t available = NUM_SCRATCHPADS;
	for (int8_t i = 0; i < NUM_SCRATCHPADS; ++i)
	{
		if (scratch_table.availability & (1 << i))
		{
			scratch_table.availability ^= 1 << i;
			arena_t* res = scratch_table.arenas + i;
			if (res->size < size)
			{
				arena_grow(res, size);
			}
			res->size = 0;
			return res;
		}
	}
	ERROR("OUT OF SCRATCHPADS!");
	assert(0 && "out of scratchpad arenas :(");
	return NULL;
}


static arena_t* table_peek(void)
{
	for (int8_t i = 0; i < NUM_SCRATCHPADS; ++i)
	{
		if (scratch_table.availability & (1 << i))
		{
			arena_t* res = scratch_table.arenas + i;
			res->size = 0;
			return res;
		}
	}
	ERROR("OUT OF SCRATCHPADS!");
	assert(0 && "out of scratchpad arenas :(");
	return NULL;
}

void scratch_checkin(const arena_t* arena)
{
	for (int8_t i = 0; i < NUM_SCRATCHPADS; ++i)
	{
		if (scratch_table.arenas + i == arena)
		{
			scratch_table.availability |= 1 << i;
		}
	}
}

void scratch_checkin_all(void)
{
	scratch_table.availability = ~0;
}

void* scratch_alloc(const int32_t size)
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

void* scratch_calloc(const int32_t size)
{
	void* addr = scratch_alloc(size);
	memset(addr, 0x00, size);
	return addr;
}

int scratch_test (int argc, char** argv)
{
	for (int8_t i = 0; i < 64; ++i)
	{
		SCRATCH* scratch = scratch_checkout();
		/* assert(scratch->cap == 0); */
		assert(scratch->size == 0);
	}
	for (int8_t i = 0; i < NUM_SCRATCHPADS; ++i)
	{
		arena_t* scratch = scratch_checkout();
		arena_dynamic_push(scratch, "foo bar baz bru\0", 16);
		/* printf("%d: %s\n", i, scratch->allocation); */
	}
	scratch_checkin_all();
	for (int8_t i = 0; i < NUM_SCRATCHPADS; ++i)
	{
		arena_t* scratch = scratch_checkout();
		/* printf("%d: %s\n", i, scratch->allocation); */
		/* const uint32_t cap = scratch->cap; */
		const void* addr0 = scratch->allocation;
	}
	scratch_checkin_all();
	scratch_audit();
	return 0;
}

void scratch_audit(void)
{
	uint8_t cum = 0;
	for (uint8_t i = 0; i < NUM_SCRATCHPADS; ++i)
	{
		if (scratch_table.availability & (1 << i)) ++cum;
	}
	printf("%d/%d scratchpads available\n", cum, NUM_SCRATCHPADS);
}
