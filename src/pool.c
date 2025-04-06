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


void pool_init(pool_t* pool, size_t chunk_size, size_t memory_size)
{
	assertf(chunk_size >= sizeof(pool_node_t), "Chunk size is too small!"); // bill ur so smart
	assertf(memory_size >= sizeof(pool_node_t), "requested memory allocation is too small!"); // bill ur so smart
	const size_t actual_size = chunk_size * (memory_size / chunk_size);
	pool->memory = malloc(actual_size);
	pool->memory_size = actual_size;
	pool->chunk_size = chunk_size;
	pool->head = memory_size;
	pool_free_all(pool);
}

void* pool_alloc(pool_t* pool)
{
	if (pool->head < pool->memory_size)
	{
		pool_node_t* head = pool->memory + pool->head;
		if (head->next != 0)
		{
			pool->head = head->next + (int32_t)head - (int32_t)pool->memory;
		}
		else
		{
			pool->head = pool->memory_size;
		}
		return head;
	}
	else
	{
		const size_t chunk_size = pool->chunk_size;
		const int n0 = pool->memory_size / chunk_size;
		const int n1 = (RESIZE_SCALE * n0) / chunk_size;
		const size_t new_size = chunk_size * n1;
		uint8_t* memory = realloc(pool->memory, new_size);
		pool_node_t* node = memory + (n1 - 1) * chunk_size;
		node->next = 0;
		for (int i = n1 - 2; i > n0; --i)
		{
			node = memory + i * chunk_size;
			node->next = (int32_t)memory + (i + 1) * chunk_size - (int32_t)node;
		}
		pool->head = (int32_t)node - (int32_t)memory;
		pool->memory = memory;
		pool->memory_size = new_size;
		return memory + n0 * chunk_size;
	}
}

void pool_free(pool_t* pool, void* ptr)
{
	if (ptr)
	{
		pool_node_t* temp = ptr;
		temp->next = (int32_t)pool->head - (int32_t)temp;
		pool->head = (uint32_t)temp - (uint32_t)pool->memory;
	}
}

void pool_free_all(pool_t* pool)
{
    const int num = pool->memory_size / pool->chunk_size;
	const int chunk_size = pool->chunk_size;
	uint8_t* memory = pool->memory;
	pool_node_t* node = memory + (num - 1) * chunk_size;
	node->next = 0;
	for (int i = num - 2; i >= 0; --i)
	{
		node = memory + i * chunk_size;
		node->next = (int32_t)memory + (i + 1) * chunk_size - (int32_t)node;
	}
	pool->head = 0;
}
