#ifndef ATOM_RING_H
#define ATOM_RING_H

#include <stdint.h>
#include "atom_ring_types.h"

// NOTE: they return non-negative bytes read on success, negative error codes on failure
// TODO: return negative error code failures
// NOTE: both() allow for multiple readers & writers simultaneously.
// if there's a bug it's probably something to do with how write/readable is calculated mid-transaction_S_

#error you can have single & concurrent readers, BUT ONLY ONE WRITER AT A TIME.  THE OTHERS CAN WAIT TIL THAT MEMCPY FINISHES

int32_t atom_ring_write(struct atom_ring* ring, void* src, const int32_t size);
int32_t atom_ring_read(struct atom_ring* ring, void* dest, const int32_t size);

int32_t atom_ring_n_write(struct atom_ring* ring, void* src, const int32_t n, const int32_t elem_size);

int32_t atom_ring_concurrent_write(struct atom_ring* ring, void* src, const int32_t size, struct tx_range* write_range);
int32_t atom_ring_concurrent_read(struct atom_ring* ring, void* dest, const int32_t size, struct tx_range* read_range);

#endif /* End ATOM_RING_H */
