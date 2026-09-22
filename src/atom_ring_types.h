#ifndef ATOM_RING_TYPES_H
#define ATOM_RING_TYPES_H

#include <stdint.h>
#include <stdatomic.h>

// NOTE: atom ring error enums are just the sync_addr errror enums

enum atom_ring_errs
{
	ATOM_RING_SIZE_CHECK_TIMEOUT = -32,
	ATOM_RING_TIMEOUT
};

// NOTE: alignment MATTERS
struct atom_ring
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
	// NOTE: used_size only used for fence-fence synchronization so memcpy writes are visible in other threads
	// and direction is actually a NECESSARY for size calculations
	// which makes both of them COMPLETELY VITAL
	// double-word
	_Atomic(uint32_t) used_size;
	_Atomic(uint32_t) direction;
	// NOTE: available != readable/writeable
	_Atomic(uint8_t) w_mask[4096];
	_Atomic(uint8_t) r_mask[4096];
};

#endif /* End ATOM_RING_TYPES_H */
