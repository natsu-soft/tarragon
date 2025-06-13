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
	uint8_t* allocation;
	uint32_t cap;
	uint32_t head;
	uint16_t chunk_size;
	uint16_t chunk_cap;
} scratch_pool = {0};

typedef struct scratch_node_t
{
	arena_t arena;
	struct
	{
		uint8_t* _pad0;
		uint32_t _pad1;
		uint32_t next;
	};
} scratch_node_t;


__attribute__((constructor)) static void scratch_init(void)
{
	const uint16_t node_size = sizeof(scratch_node_t);
	const uint32_t cap = NUM_SCRATCHPADS * node_size;
	void* alloc = calloc(NUM_SCRATCHPADS, node_size);
	assert(alloc && "failed to allocate scratch memory!\n");
	scratch_pool.allocation = alloc;
	arena_init(alloc, SCRATCH_OVERHEAD);
	scratch_pool.cap = NUM_SCRATCHPADS * node_size;
	scratch_node_t* node;
	for (uint8_t i = 0; i < NUM_SCRATCHPADS; ++i)
	{
		node = alloc + i * node_size;
		node->next = (i + 1) * node_size;
	}
	scratch_pool.head = 0;
}

static void* pool_alloc(void)
{
	LOG("pool_alloc()");
	assert(scratch_pool.head < scratch_pool.cap && "SCRATCH POOL OVERFLOW!\n");
	scratch_node_t* ptr = scratch_pool.head + scratch_pool.allocation;
	scratch_pool.head = ptr->next;
	return ptr;
}

static void* pool_peek(void)
{
	assert(scratch_pool.head < scratch_pool.cap && "SCRATCH POOL OVERFLOW!\n");
	arena_t* res = scratch_pool.allocation + scratch_pool.head;
	return res;
}

static void pool_free(uint8_t* ptr)
{
	const uint8_t* alloc = scratch_pool.allocation;
	if (alloc <= ptr && ptr < alloc + scratch_pool.cap)
	{
		scratch_node_t* temp = ptr;
		temp->next = scratch_pool.head;
		scratch_pool.head = ptr - alloc;
	}
}

void* scratch_alloc(const uint32_t size)
{
	arena_t* scratch = pool_peek();
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

arena_t* scratch_checkout(void)
{
	LOG("scratch_checkout()");
	arena_t* scratch = pool_alloc();
	scratch->size = 0;
	return scratch;
}

void scratch_checkin(arena_t* scratch)
{
	pool_free(scratch);
}

int scratch_test (int argc, char** argv)
{
	const size_t h0 = scratch_pool.head;
	for (uint8_t i = 0; i < 64; ++i)
	{
		SCRATCH* scratch = scratch_checkout();
		/* assert(scratch->cap == 0); */
		assert(scratch->size == 0);
	}
	return 0;
}
