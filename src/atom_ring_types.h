#ifndef ATOM_RING_TYPES_H
#define ATOM_RING_TYPES_H

#include <stdint.h>
#include <stdatomic.h>

// NOTE: atom ring error enums are just the sync_addr errror enums

enum atom_ring_results
{
	ATOM_RING_EVICTED = 0, /* not an error, sometime I just gotta boot all fuccn queued transactions */
	ATOM_RING_SIZE_CHECK_TIMEOUT = -32,
	ATOM_RING_TIMEOUT,
	ATOM_RING_INVALID_READ,
	ATOM_RING_OVERWRITE,
};

struct tx_range
{
	uint32_t start;
	uint32_t end;
};

// NOTE: alignment MATTERS
struct atom_ring
{
	_Atomic(uint8_t)* addr;
	// double-word
	uint32_t size; // speed NOW
	int32_t fd;
	// double-word
	// pre-transaction values
	_Atomic(int32_t) head;
	_Atomic(int32_t) tail;
	// double-word
	_Atomic(int32_t) write_state;
	_Atomic(int32_t) read_state;
	/* NOTE: ASPIRATIONAL
	 * post-transaction values to accomodate EITHER concurrent readers OR concurrent writers
	 * - this still doesnt accomodate multiple readers & writers
	 * - I DONT THINK YOU CAN HAVE BOTH
	 * - skill issue, I can have both >:^)
	 * BUT you can QUEUE the other */
	// double-word
	_Atomic(int32_t) reading_head;
	_Atomic(int32_t) writing_tail;
	// NOTE: NVM.  this is right: https://lo.calho.st/posts/black-magic-buffer/
	// direction is actually a NECESSARY for size calculations--WRONG!
	// double-word
	/* _Atomic(int32_t) direction; */
};

#endif /* End ATOM_RING_TYPES_H */
