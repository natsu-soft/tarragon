#ifndef ATOM_RING_H
#define ATOM_RING_H

#include <stdint.h>
#include "atom_ring_types.h"

// NOTE: they return non-negative bytes read on success, negative error codes on failure
// TODO: return negative error code failures
// NOTE: atomic_ring_multi_*() allow for multiple readers/writers. Their single counterparts wait until there are NO opposite transactions & only updates head/tail after transaction is FULLY completed
int32_t atom_ring_write(struct atom_ring* ring, void* src, const int32_t size);
int32_t atom_ring_read(struct atom_ring* ring, void* restrict dest, const int32_t size, const int32_t id, const uint32_t gen);

#endif /* End ATOM_RING_H */
