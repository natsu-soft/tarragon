#ifndef ATOM_RING_TYPES_H
#define ATOM_RING_TYPES_H

#include <stdint.h>
#include <stdatomic.h>

// NOTE: atom ring error enums are just the sync_addr errror enums

// NOTE: alignment MATTERS
struct atom_ring
{
	void* addr;
	// double-word
	int32_t size;
	int32_t fd;
	// double-word
	_Atomic(int32_t) read_state;
	_Atomic(int32_t) write_state;
	// double-word
	// pre-transaction values
	_Atomic(int32_t) head;
	_Atomic(int32_t) tail;
	// double-word
	// NOTE: keep the track of the transacting sizes for calculating the read/writeable sizes
	_Atomic(int32_t) read_tx;
	_Atomic(int32_t) write_tx;
	// double-word
};

#endif /* End ATOM_RING_TYPES_H */
