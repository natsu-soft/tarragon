#ifndef SAFE_RING_H
#define SAFE_RING_H

#include "safe_ring_types.h"
#include <stdint.h>

int32_t safe_ring_write(struct safe_ring* ring, void* src, const int32_t size);
int32_t safe_ring_read(struct safe_ring* ring, void* src, const int32_t size);

int32_t safe_ring_n_write(struct safe_ring* ring, void* src, const int32_t n, const int32_t elem_size);



#endif /* End SAFE_RING_H */
