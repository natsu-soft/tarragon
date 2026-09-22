#ifndef SAFE_RING_TYPES_H
#define SAFE_RING_TYPES_H

#include <stdint.h>
#include <stdatomic.h>


// NOTE: alignment MATTERS
struct safe_ring
{
	// NOTE: NECESSARY
	_Atomic(uint8_t)* addr;
	// double-word
	int32_t size;
	int32_t fd;
	// double-word
	// pre-transaction values
	_Atomic(int32_t) head;
	_Atomic(int32_t) tail;
};

#endif /* End SAFE_RING_TYPES_H */
