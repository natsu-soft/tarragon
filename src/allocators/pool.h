#ifndef POOL_H
#define POOL_H

// NOTE: UNUSED

#include <stddef.h>
#include <stdint.h>

typedef struct pool_t
{
	uint8_t* allocation;
	uint32_t cap; // only 2TB ToT
	uint32_t head; // relative (to memory) OwO
	uint16_t chunk_size; // only 65KB ToT
	uint16_t chunk_cap; // only 65k chunks ToT
} pool_t;


void pool_init(pool_t* pool, const uint16_t chunk_size, const uint16_t chunk_cap);

void* pool_alloc(pool_t* pool);

void pool_free(pool_t* pool, void* ptr);

void pool_free_all(pool_t* pool);


#endif /* End POOL_H */
