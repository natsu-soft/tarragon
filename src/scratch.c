#include "scratch.h"
#include <stdlib.h>
#include <string.h>
#include "arena.h"

typedef struct alloc_header_t
{
	size_t size;
} alloc_header_t;

#define INIT_SCRATCHPADS 8
#define SCRATCH_OVERHEAD 4096

static uint8_t scratch_num = 0;
static uint8_t scratch_index = 0;
static arena_t* scratchpads = NULL;

/* static arena_t scratch = {0}; */

static void __attribute__((constructor)) scratch_init(void)
{
	scratchpads = calloc(INIT_SCRATCHPADS, sizeof *scratchpads);
	scratch_num = INIT_SCRATCHPADS;
	arena_init(scratchpads, 0x10 * SCRATCH_OVERHEAD);
}

void* scratch_alloc(const size_t size)
{
	/* size_t total_size = size + padding + sizeof(alloc_header_t); */
	size_t padding = 0;
	arena_t* scratch = scratchpads + scratch_index;
	alloc_header_t* header;
	if (scratch->memory != NULL)
	{
		const uint8_t modulo = scratch->size & (sizeof(size_t) - 1);
		padding = modulo > 0 ? sizeof(size_t) - modulo : 0;
		const size_t total_size = size + padding + sizeof *header;
		if (scratch->size + total_size <= scratch->cap)
		{
			goto alloc;
		}
		else if (scratch->size == 0)
		{
			const size_t new_size =  size + sizeof *header + SCRATCH_OVERHEAD;
			scratch->memory = realloc(scratch->memory, new_size);
			scratch->cap = new_size;
			goto alloc;
		}
	}
	scratch_index++;
	if (scratch_index < scratch_num)
	{
		scratch = scratchpads + scratch_index;
		if (scratch->memory == NULL)
		{
			goto scratch_init;
		}
		else // size == 0 is a given (think about it)
		{
			const size_t req_size = size + SCRATCH_OVERHEAD + sizeof *header;
			if (scratch->cap <  req_size)
			{
				scratch->memory = realloc(scratch->memory, req_size);
				scratch->cap = req_size;
			}
			goto alloc;
		}
	}
	scratchpads = realloc(scratchpads, 2 * scratch_num * sizeof *scratchpads);
	memset(scratchpads + scratch_num, 0x00, scratch_num * sizeof *scratchpads);
	/* scratch_index = scratch_num; */
	scratch = scratchpads + scratch_index;
	scratch_num *= 2;
scratch_init:
	arena_init(scratch, size + SCRATCH_OVERHEAD + sizeof *header);
alloc:
	header = arena_alloc(scratch, sizeof *header);
	header->size = size + padding;
	return arena_alloc(scratch, size + padding);
}

void scratch_free(void* ptr)
{
	alloc_header_t* header = (alloc_header_t*)ptr - 1;
	const size_t total_size = header->size + sizeof(*header);
	arena_t* scratch = scratchpads + scratch_index;
	arena_free(scratch, total_size);
	if (scratch->size == 0)
	{
		scratch_index--;
	}
}
