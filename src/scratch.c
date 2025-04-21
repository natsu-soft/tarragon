#include "scratch.h"
#include <stdlib.h>
#include "arena.h"


static arena_t scratch = {0};

static void __attribute__((constructor)) scratch_init(void)
{
	arena_init(&scratch, 4096);
}

void* scratch_alloc(size_t size)
{
	if (scratch.size + size <= scratch.cap)
	{
		return arena_alloc(&scratch, size);
	}
	else
	{
		return NULL;
	}
}

void scratch_free()
{
	scratch.size = 0;
}

void scratch_grow(size_t size)
{
	scratch.memory = realloc(scratch.memory, size);
	scratch.size = size;
}
