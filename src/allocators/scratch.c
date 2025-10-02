#include "scratch.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "arena.h"

#define SCRATCH_OVERHEAD 4096
#define NUM_SCRATCH 8

static arena_t scratch = {0x00};

__attribute__((constructor))
void scratch_init(void)
{
	arena_init(&scratch, SCRATCH_OVERHEAD);
}

void* scratch_alloc(const int32_t size)
{
	if (scratch.cap < size)
	{
		arena_grow(&scratch, size);
	}
	return scratch.allocation;
}

arena_t* scratch_arena(const int32_t size)
{
	scratch.size = 0;
	if (scratch.cap < size)
	{
		arena_grow(&scratch, size);
	}
	return &scratch;
}
