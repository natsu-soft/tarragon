#ifndef POOL_H
#define POOL_H

// UNUSED

#include <stddef.h>
#include <stdint.h>

typedef struct pool_t
{
	uint8_t* memory;
	size_t memory_size;
	size_t chunk_size;
	uint32_t head; // relative (to memory) OwO
} pool_t;


void pool_init(pool_t* pool, size_t chunk_size, size_t memory_size);

void* pool_alloc(pool_t* pool);

void pool_free(pool_t* pool, void* ptr);

void pool_free_all(pool_t* pool);


#endif /* End POOL_H */
