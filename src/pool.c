#include "pool.h"
#include <stdlib.h>
#include "error_macros.h"


typedef struct pool_node_t
{
	// ALLOCATION RELATIVE POINTERS
	uint32_t next;
} pool_node_t;


void pool_init(pool_t* pool, const uint16_t chunk_size, const uint16_t chunk_cap)
{
	const size_t alloc_size = (uint32_t)chunk_size * (uint32_t)chunk_cap;
	assertf(chunk_size >= sizeof(pool_node_t), "Chunk size is too small!"); // bill ur so smart
	assertf(alloc_size >= sizeof(pool_node_t), "requested memory allocation is too small!"); // bill ur so smart
	pool->allocation = calloc(chunk_cap, chunk_size);
	pool->cap = alloc_size;
	pool->size = 0;
	pool->chunk_size = chunk_size;
	pool->chunk_cap = chunk_cap;
	pool_free_all(pool);
}

void* pool_alloc(pool_t* pool)
{
	assertf(pool->head < pool->cap, "POOL OVERFLOW!");
	pool_node_t* ptr = pool->head + pool->allocation;
	pool->head = ptr->next;
	return ptr;
}

void pool_free(pool_t* pool, void* ptr)
{
	const uint8_t* alloc = pool->allocation;
	if (alloc <= ptr && ptr < alloc + pool->size)
	{
		pool_node_t* temp = ptr;
		temp->next = pool->head;
		pool->head = (uint8_t*)temp - alloc;
	}
}

void pool_free_all(pool_t* pool)
{
	const uint32_t size = pool->chunk_size; // hoist pls
	const uint32_t num = pool->chunk_cap;
	const uint8_t* alloc = pool->allocation;
	pool_node_t* node;
	for (uint32_t i = 0; i < num; ++i)
	{
		node = alloc + i * size;
		node->next = (i + 1) * size; // alloc_size treated as NULL
	}
	pool->head = 0;
}
