#include "pool.h"
#include <stdlib.h>
#include "error_macros.h"

typedef struct pool_elem_t
{
	pool_elem_t* next;
} pool_elem_t;


void pool_init(pool_t* pool, uint32_t num_chunks, size_t chunk_size)
{
	assertf(num_chunks, "Zero chunks for pool_init! U having giggl?");
	pool->num_chunks = num_chunks;
	pool->chunk_size = chunk_size;
	pool->memory = malloc(num_chunks + chunk_size);
	pool->head = pool->memory;
	pool_free_all(pool);
}

void* pool_alloc(pool_t* pool)
{
	assert(pool->head);
	uint8_t* res = pool->head;
	pool->head = pool->head->next;
	return res;
}

void pool_free(pool_t* pool, void* ptr)
{
	pool_elem_t* temp = ptr;
	temp->next = pool->head;
	pool->head = temp;
}

void pool_free_all(pool_t* pool)
{
	pool_elem_t* e;
	for (int i = pool->num_chunks - 2; i >= 0; --i)
	{
		e = pool->memory + i * pool->chunk_size;
		e->next = pool->memory + (i + 1) * pool->chunk_size;
	}
	pool->head = e;
	e = pool->memory + (pool->num_chunks - 1) * pool->chunk_size;
	e->next = NULL;
}
