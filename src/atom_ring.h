#ifndef ATOM_RING_H
#define ATOM_RING_H

#include <stdint.h>
#include "atom_ring_types.h"

// NOTE: they return non-negative bytes read on success, negative error codes on failure
// TODO: return negative error code failures
// NOTE: both() allow for multiple readers & writers simultaneously.
// if there's a bug it's probably something to do with how write/readable is calculated w/ transactions
int32_t atom_ring_write(struct atom_ring* ring, void* restrict src, const int32_t size);
int32_t atom_ring_read(struct atom_ring* ring, void* restrict dest, const int32_t size);

#endif /* End ATOM_RING_H */
