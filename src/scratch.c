#include "scratch.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "arena.h"

typedef struct alloc_header_t
{
	size_t size;
} alloc_header_t;

#define SCRATCH_OVERHEAD 4096

static arena_t scratch = {0};
/* static arena_t work_arena = {0}; */

static void __attribute__((constructor)) scratch_init(void)
{
	arena_init(&scratch, 2 * SCRATCH_OVERHEAD);
	/* arena_init(&work_arena, 2 * SCRATCH_OVERHEAD); */
}

void* scratch_alloc(const size_t size)
{
	/* size_t total_size = size + padding + sizeof(alloc_header_t); */
	size_t padding = 0;
	alloc_header_t* header;
	const uint8_t modulo = scratch.size & (sizeof(size_t) - 1);
	padding = modulo > 0 ? sizeof(size_t) - modulo : 0;
	const size_t total_size = size + padding + sizeof *header;
	if (scratch.size + total_size > scratch.cap)
	{
		const size_t new_size = scratch.cap + total_size + SCRATCH_OVERHEAD;
		arena_resize(&scratch, new_size);
	}
	header = arena_alloc(&scratch, sizeof *header);
	header->size = size + padding;
	return arena_alloc(&scratch, size + padding);
}

void scratch_free(void* ptr)
{
	alloc_header_t* header = (alloc_header_t*)ptr - 1;
	const size_t total_size = header->size + sizeof(*header);
	arena_free(&scratch, total_size);
}

void scratch_free_all()
{
	scratch.size = 0;
}

void scratch_audit()
{
	printf("> SCRATCH AUDIT\n");
	printf(">> scratch: %zu/%zu\n", scratch.size , scratch.cap);
}

/* arena_t* scratch_work() */
/* { */
/* 	work_arena.size = 0; */
/* 	return &work_arena; */
/* } */
