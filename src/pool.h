#ifndef POOL_H
#define POOL_H

#include <stddef.h>
#include <stdint.h>

typedef struct pool_elem_t pool_elem_t;

typedef struct pool_t
{
	uint32_t num_chunks;
	uint32_t chunk_size;
	pool_elem_t* head;
	uint8_t* memory;
} pool_t;


void pool_init(pool_t* pool, uint32_t num_chunks, size_t chunk_size);

void* pool_alloc(pool_t* pool);

void pool_free(pool_t* pool, void* addr);

void pool_free_all(pool_t* pool);


#endif /* End POOL_H */
