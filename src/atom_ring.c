#include "atom_ring.h"
#include <assert.h>
#include <stdint.h>
#include "atomics.h"
#include <string.h>
#include <time.h>
#include "sync_addr.h"
#include "threading.h"

/* #error JUST A SINGLE STATE.  YES IT'LL BLOCK */
// consider this: https://david.alvarezrosa.com/posts/optimizing-a-lock-free-ring-buffer/
// he's not using a real ring tho just an array
// he's also NOT memcpying--he's working bit-by-bit lmao

enum
{
	FREE	= 0x00,
	LOCKED	= 0x01,
	QUEUED	= 0x02,
	WAITING	= 0x04,
};



/* #include <stdio.h> */

static inline int32_t readable_size(struct atom_ring* ring)
{
	const int32_t size = ring->size;
	const int32_t head = load_ex(&ring->head, acquire);
	const int32_t tail = load_ex(&ring->tail, acquire);
	return head >= tail ? head - tail : head + size - tail;
}

static inline int32_t writeable_size(struct atom_ring* ring)
{
	const int32_t size = ring->size;
	const int32_t head = load_ex(&ring->head, acquire);
	const int32_t tail = load_ex(&ring->tail, acquire);
	return head >= tail ? tail + size - head : tail - head;
}

int32_t atom_ring_write(struct atom_ring* ring, void* restrict src, const int32_t size)
{
	_Atomic(int32_t)* const read_state = &ring->read_state;
	_Atomic(int32_t)* const write_state = &ring->write_state;
	int32_t state;
	for (;;)
	{
		state = load(write_state) & ~LOCKED;
		if (size <= writeable_size(ring) - load(&ring->read_tx) &&
			comp_swap_ex(write_state, &state, LOCKED | state, acq_rel, acquire))
		{
			// futex wait until read_tx size is 0
			fetch_add(&ring->write_tx, size);
			const int32_t head = swap(&ring->head, (ring->head + size) % ring->size);
			state = LOCKED | WAITING;
			int32_t num_woke = 0;
			if (fetch_and(write_state, ~LOCKED) & WAITING)
			{
				num_woke = sync_addr_wake_all(write_state);
			}
			memcpy(ring->addr + head, src, size);
			fetch_sub(&ring->write_tx, size);
			if (fetch_and(read_state, ~WAITING) == WAITING)
			{
				sync_addr_wake_all(read_state);
			}
			return size;
		}
		int32_t wait_state = state | WAITING;
		if (comp_swap(write_state, &state, wait_state))
		{
			sync_addr_wait(write_state, &wait_state);
		}
	}
}

int32_t atom_ring_multi_read(struct atom_ring* ring, void* restrict dest, const int32_t size, const int32_t id, const uint32_t gen)
{
	_Atomic(int32_t)* const read_state = &ring->read_state;
	_Atomic(int32_t)* const write_state = &ring->write_state;
	int32_t state;
	for (;;)
	{
		state = load(read_state) & ~LOCKED;
		if (size <= readable_size(ring) - load(&ring->write_tx) &&
			comp_swap_ex(read_state, &state, LOCKED | state, acquire, acquire))
		{
			fetch_add(&ring->read_tx, size);
			const int32_t tail = swap(&ring->tail, (ring->tail + size) % ring->size);
			state = LOCKED | WAITING;
			if (fetch_and(read_state, ~LOCKED) & WAITING)
			{
				sync_addr_wake_all(read_state);
			}
			memcpy(dest, ring->addr + tail, size);
			fetch_sub(&ring->read_tx, size);
			if (fetch_and(write_state, ~WAITING) == WAITING)
			{
				sync_addr_wake_all(write_state);
			}
			/* printf(">> read: [%d, %d); woke %d readers\n", tail, tail + size); */
			return size;
		}
		int32_t wait_state = state | WAITING;
		if (comp_swap(read_state, &state, wait_state))
		{
			sync_addr_wait(read_state, &wait_state);
		}
	}
}
