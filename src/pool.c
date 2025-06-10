#include "pool.h"
#include <stdlib.h>
#include "error_macros.h"

#define RESIZE_SCALE 1.618

// UNUSED

typedef struct pool_node_t
{
	// SELF RELATIVE POINTERS
	int32_t next;
} pool_node_t;


void pool_init(pool_t* pool, const uint16_t chunk_size, const uint16_t chunk_cap)
{
	const size_t alloc_size = (uint32_t)chunk_size * (uint32_t)chunk_cap;
	assertf(chunk_size >= sizeof(pool_node_t), "Chunk size is too small!"); // bill ur so smart
	assertf(alloc_size >= sizeof(pool_node_t), "requested memory allocation is too small!"); // bill ur so smart
	pool->allocation = malloc(alloc_size);
	pool->cap = alloc_size;
	pool->size = 0;
	pool->chunk_size = chunk_size;
	pool->chunk_cap = chunk_cap;
	pool_free_all(pool);
}

void* pool_alloc(pool_t* pool)
{
	if (pool->head < pool->cap)
	{
		pool_node_t* head = pool->allocation + pool->head;
		if (head->next != 0)
		{
			pool->head = head->next + (int32_t)head - (int32_t)pool->allocation;
		}
		else
		{
			pool->head = pool->cap;
		}
		return head;
	}
	else
	{
		const size_t chunk_size = pool->chunk_size;
		const int n0 = pool->cap / chunk_size;
		const int n1 = (RESIZE_SCALE * n0) / chunk_size;
		const size_t new_size = chunk_size * n1;
		uint8_t* memory = realloc(pool->allocation, new_size);
		pool_node_t* node = memory + (n1 - 1) * chunk_size;
		node->next = 0;
		for (int i = n1 - 2; i > n0; --i)
		{
			node = memory + i * chunk_size;
			node->next = (int32_t)memory + (i + 1) * chunk_size - (int32_t)node;
		}
		pool->head = (int32_t)node - (int32_t)memory;
		pool->allocation = memory;
		pool->cap = new_size;
		return memory + n0 * chunk_size;
	}
	pool->size += pool->chunk_size;
}

void pool_free(pool_t* pool, void* ptr)
{
	if (ptr)
	{
		pool_node_t* temp = ptr;
		temp->next = (int32_t)pool->head - (int32_t)temp;
		pool->head = (uint32_t)temp - (uint32_t)pool->allocation;
	}
}

void pool_free_all(pool_t* pool)
{
    // dost thou even hoist?
	const uint16_t size = pool->chunk_size;
	const uint32_t cap = pool->cap;
	const uint8_t* alloc = pool->allocation;
	const int chunk_size = pool->chunk_size;
	uint8_t* memory = pool->allocation;
	pool_node_t* node = memory + (num - 1) * chunk_size;
	node->next = 0;
	for (int i = num - 2; i >= 0; --i)
	{
		node = memory + i * chunk_size;
		node->next = (int32_t)memory + (i + 1) * chunk_size - (int32_t)node;
	}
	pool->head = 0;
}
