#include "atom_ring.h"
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "atomics.h"
#include "sync_addr.h"
#include "threading.h"

// https://github.com/torvalds/linux/raw/refs/heads/master/Documentation/memory-barriers.txt
// https://lwn.net/Articles/576486/

enum dbg_tag
{
	CACHE_WRITE = 0x00,
	CACHE_READ,
	UPDATE_TAIL,
	UPDATE_HEAD
};

struct dbg_msg
{
	uint32_t pos0;
	uint32_t pos1;
	enum dbg_tag tag;
};

#define MSG_RING_SIZE 1024

struct
{
	_Atomic(uint32_t) head;
	struct dbg_msg messages[MSG_RING_SIZE];
} msg_ring = {0};
/* static struct ticketlock msg_lock = {0}; */

static void cache_write(const uint32_t start, const uint32_t end)
{
	const uint32_t idx = fetch_add_ex(&msg_ring.head, 1, seq_cst) & (MSG_RING_SIZE - 1);
	/* printf("%d\n", idx); */
	struct dbg_msg* temp = msg_ring.messages + idx;
	temp->pos0 = start;
	temp->pos1 = end;
	temp->tag = CACHE_WRITE;
}

static void cache_read(const uint32_t start, const uint32_t end)
{
	const uint32_t idx = fetch_add_ex(&msg_ring.head, 1, seq_cst) & (MSG_RING_SIZE - 1);
	/* printf("%d\n", idx); */
	struct dbg_msg* temp = msg_ring.messages + idx;
	temp->pos0 = start;
	temp->pos1 = end;
	temp->tag = CACHE_READ;
}

static void cache_update_tail(const uint32_t pos0, const uint32_t pos1)
{
	const uint32_t idx = fetch_add_ex(&msg_ring.head, 1, seq_cst) & (MSG_RING_SIZE - 1);
	/* printf("%d\n", idx); */
	struct dbg_msg* temp = msg_ring.messages + idx;
	temp->pos0 = pos0;
	temp->pos1 = pos1;
	temp->tag = UPDATE_TAIL;
}

static void cache_update_head(const uint32_t pos0, const uint32_t pos1)
{
	const uint32_t idx = fetch_add_ex(&msg_ring.head, 1, seq_cst) & (MSG_RING_SIZE - 1);
	/* printf("%d\n", idx); */
	struct dbg_msg* temp = msg_ring.messages + idx;
	temp->pos0 = pos0;
	temp->pos1 = pos1;
	temp->tag = UPDATE_HEAD;
}


static void msg_print_all(void)
{
	const int32_t i0 = msg_ring.head - MSG_RING_SIZE < msg_ring.head ? msg_ring.head - MSG_RING_SIZE : 0;
	thread_fence(acquire);
	printf("msg_ring.head %d\n", msg_ring.head);
	for (int32_t i = i0; i < msg_ring.head; ++i)
	{
		struct dbg_msg* msg = msg_ring.messages + (i % MSG_RING_SIZE);

		switch(msg->tag)
		{
		case CACHE_WRITE:
			printf("writing: [%d, %d)\n", msg->pos0, msg->pos1);
			break;
		case CACHE_READ:
			printf("reading: [%d, %d)\n", msg->pos0, msg->pos1);
			break;
		case UPDATE_TAIL:
			printf("tail %d -> %d\n", msg->pos0, msg->pos1);
			break;
		case UPDATE_HEAD:
			printf("head %d -> %d\n", msg->pos0, msg->pos1);
			break;
		}
	}
}


// TODO: read & UNDERSSTAND fence-fence synchronization
// https://en.cppreference.com/c/atomic/atomic_thread_fence

// consider this: https://david.alvarezrosa.com/posts/optimizing-a-lock-free-ring-buffer/
// he's not using a real ring tho just an array
// he's also NOT memcpying--he's working bit-by-bit lmao
// also just ONE writer & ONE reader dawg ur MULTI-READING

/* The OUTLINE of a read/write

   loop:
	 while (valid_size < tx_size): wait
	 if (trylock(state)): actually transact
	 else: wait

   Now, transactions get fucky depending onw hether or not it's serial or concurrent

   serial transaction (only one read/write, others queue):
     0.  if valid size < tx_size: unlock tx_state then CONTINUE
     1.  fetch_inc transaction_start by tx_size
     3.  RELAXED load number of OPPOSITE transactors
     4.  *** ACQUIRE FENCE ***
     5.  COPY THE MEMORY
         + error check for debugging
     6.  *** RELEASE FENCE ***
	 7.  RELAXED increment transaction volume
     8.  increment visible_start by tx size
     9.  unlock tx_state
     10. wake other transactor if all waiting

   concurrent transaction:
     0.  if valid size < tx_size: unlock tx_state then CONTINUE
     1.  fetch_inc transaction_start by tx_size
     2.  unlock tx_state
     3.  RELAXED load total OPPOSITE transaction vol
     4.  *** ACQUIRE FENCE ***
     5.  COPY THE MEMORY
         + error check for debugging
     6.  *** RELEASE FENCE ***
	 7.  ACQ_REL increment transaction volume
	 8.  while visible_start != tx_start: wait on visible_start (or just spin lmao)
	 9.  visible start = tx_start + tx_size
	 10. wake all waiting on visible_start
     11. wake other transactor if all waiting

	 NOTE: I *think* I don't even need to use atomics for the initial
	 load of tx_start WHILESTVE incrementing tx_start since *NO
	 MATTER WHAT* only one thread should be incrementing that
	 bad boy at a time
 */

enum
{
	FREE		= 0x00,
	LOCKED		= 0x01,
	WAITING		= 0x02,
	ERR_STOP	= 0x04,
	EVICTED		= 0x08,
};

static inline const int32_t writeable_size(struct atom_ring* ring)
{
	const int32_t head = load_ex(&ring->head, acquire);
	const int32_t tail = load_ex(&ring->writing_tail, acquire);
	/* const int32_t dir = load_ex(&ring->direction, acquire); */
	/* return head <= size ? size - (tail - head) : head - tail; */
	return  tail >= head ? ring->size - (tail - head) - 1 : head - tail - 1;
}

static inline const int32_t readable_size(struct atom_ring* ring)
{
	const int32_t head = load_ex(&ring->reading_head, acquire);
	const int32_t tail = load_ex(&ring->tail, acquire);
	return tail >= head ? tail - head : tail + ring->size - head;
}

static inline int32_t fetch_add_mod(_Atomic(int32_t)* val_ptr, const int32_t add, const int32_t mod)
{
	int32_t value0 = load_ex(val_ptr, acquire);
	int32_t res;
	do
	{
		res = (value0 + add) % mod;
	} while (!comp_swap_weak_ex(val_ptr, &value0, res, acq_rel, acquire));
	return value0;
}

static inline int32_t fetch_add_mask(_Atomic(int32_t)* val_ptr, const int32_t add, const uint32_t mask)
{
	int32_t value0 = load_ex(val_ptr, acquire);
	int32_t res;
	do
	{
		res = (value0 + add) & mask;
	} while (!comp_swap_weak_ex(val_ptr, &value0, res, acq_rel, acquire));
	return value0;
}

static void print_ring(struct atom_ring* ring, const int32_t i0, const int32_t i1)
{
	fflush(stdout);
	int32_t idx = 0;
	putc('\n', stdout);
	for (int32_t r = 0; r < ring->size; r += 32)
	{
		printf("%004d ", r);
		for (int32_t c = 0; c < 32; ++c)
		{
			idx = r + c;
			if (c == 16)
			{
				putc(' ', stdout);
			}
			if (idx == i0)
			{
				putc('[', stdout);
			}
			else if (idx == i1)
			{
				putc(']', stdout);
			}
			else
			{
				putc(' ', stdout);
			}
			printf("%02x", ring->addr[idx]);
		}
		putc('\n', stdout);
	}
	printf("\n----- SECOND ALLOCATION ------------------------------------------------------------------------------\n\n");
	for (int32_t r = ring->size; r < 2 * ring->size; r += 32)
	{
		printf("%004d ", r);
		for (int32_t c = 0; c < 32; ++c)
		{
			idx = r + c;
			if (c == 16)
			{
				putc(' ', stdout);
			}
			if (idx - ring->size == i0)
			{
				putc('[', stdout);
			}
			else if (idx - ring->size == i1)
			{
				putc(']', stdout);
			}
			else
			{
				putc(' ', stdout);
			}
			printf("%02x", ring->addr[idx]);
		}
		putc('\n', stdout);
	}
}

static inline int32_t unlock_and_wake(_Atomic(int32_t)* state_ptr)
{
	int32_t res = 0;
	if (fetch_and_ex(state_ptr, ~LOCKED, acq_rel) & WAITING)
	{
		res = sync_addr_wake_all(state_ptr);
	}
	return res;
}

static inline int32_t wake_if_only_waiting(_Atomic(int32_t)* state_ptr)
{
	int32_t res = 0;
	if (fetch_and_ex(state_ptr, ~WAITING, acq_rel) == WAITING)
	{
		res = sync_addr_wake_all(state_ptr);
	}
	return res;
}

// NOTE: atom_ring_read & concurrent_read both zero read regions.
// But -ONLY- atom_ring_concurrent_write checks if its writing into a zeroed region.

int32_t atom_ring_write(struct atom_ring* ring, void* src, const int32_t size)
{
	assert(0 < size);
	assert(size <= ring->size - 1);
	_Atomic(int32_t)* const write_state = &ring->write_state;
	int32_t state = load(write_state);
	while (!(state & EVICTED))
	{
		/* while(writeable_size(ring) < size) */
		/* { */
		/* 	state |= WAITING | fetch_or_ex(write_state, WAITING, release); */
		/* 	sync_addr_wait(write_state, &state); */
		/* 	state ^= WAITING; */
		/* } */
		state &= ~(LOCKED | EVICTED);
		if (size <= writeable_size(ring) &&
			comp_swap_ex(write_state, &state, LOCKED | state, acq_rel, acquire))
		{
			if (writeable_size(ring) < size)
			{
				assert(0 && "writing: lol nvm");
			}
			/* int32_t tail = fetch_add_mod(&ring->writing_tail, size, ring->size); */
			/* const int32_t tail = fetch_add_ex(&ring->writing_tail, size, acq_rel); */
			/* const int32_t tail = fetch_add_mask(&ring->writing_tail, size, ring->size - 1); */
			// honestly its just clearer this way
			const int32_t tail = load_ex(&ring->writing_tail, acquire);
			const int32_t tail1 = tail + size & ring->size - 1;
			store_ex(&ring->writing_tail, tail1, release);

			// TODO: in-flight error checking

			/* memcpy(ring->addr + (tail & ring->size - 1), src, size); */
			memcpy(ring->addr + tail, src, size);
			cache_write(tail, tail1);
			/* if (tail1 < tail0) fetch_or_ex(&ring->direction, REVERSE, release); */
			store_ex(&ring->tail, tail1, release);
			unlock_and_wake(write_state);
			wake_if_only_waiting(&ring->read_state);
			return size;
		}
		int32_t wait_state = WAITING | state;
		if (!(state & EVICTED) && comp_swap_ex(write_state, &state, wait_state, acq_rel, acquire))
		{
			sync_addr_wait(write_state, &wait_state);
		}
		state = load_ex(write_state, acquire);
	}
	return ATOM_RING_EVICTED;
}

int32_t atom_ring_read(struct atom_ring* ring, void* dest, const int32_t size)
{
	assert(0 < size);
	assert(size <= ring->size - 1);
	_Atomic(int32_t)* const read_state = &ring->read_state;
	int32_t state = load_ex(read_state, acquire);
	while (!(state & EVICTED))
	{
		/* whie (readable_size(ring) < size) */
		/* { */
		/* 	state = WAITING | fetch_or_ex(read_state, WAITING, release); */
		/* 	sync_addr_wait(read_state, &state); */
		/* 	state ^= WAITING; */
		/* } */
		state &= ~(LOCKED | EVICTED);
		if (size <= readable_size(ring) &&
			comp_swap_ex(read_state, &state, LOCKED | state, acq_rel, acquire))
		{
			if (readable_size(ring) < size)
			{
				assert(0 && "reading: lol nvm");
			}
			const int32_t head = load_ex(&ring->reading_head, acquire);
			const int32_t head1 = head + size & ring->size - 1;
			store_ex(&ring->reading_head, head1, release);
			// TODO: in-flight error checking
			memcpy(dest, ring->addr + head, size);
			// TODO: comment this out
			memset(ring->addr + head, 0x00, size);
			cache_read(head, head1);
			store_ex(&ring->head, head1, release);
			unlock_and_wake(read_state);
			wake_if_only_waiting(&ring->write_state);
			return size;
		}
		int32_t wait_state = WAITING | state;
		if (!(state & EVICTED) && comp_swap_ex(read_state, &state, wait_state, acq_rel, acquire))
		{
			sync_addr_wait(read_state, &wait_state);
		}
		state = load_ex(read_state, acquire);
	}
	return ATOM_RING_EVICTED;
}

int32_t atom_ring_n_write(struct atom_ring* ring, void* src, const int32_t n, const int32_t elem_size)
{
	const int32_t size = n * elem_size;
	assert(0 < size);
	assert(size <= ring->size - 1);
	_Atomic(int32_t)* const write_state = &ring->write_state;
	int32_t state = load(write_state);
	while (!(state & EVICTED))
	{
		/* while(writeable_size(ring) < size) */
		/* { */
		/* 	state |= WAITING | fetch_or_ex(write_state, WAITING, release); */
		/* 	sync_addr_wait(write_state, &state); */
		/* 	state ^= WAITING; */
		/* } */
		state &= ~(LOCKED | EVICTED);
		if (size <= writeable_size(ring) &&
			comp_swap_ex(write_state, &state, LOCKED | state, acq_rel, acquire))
		{
			if (writeable_size(ring) < size)
			{
				#ifndef NOFRAUD
				unlock_and_wake(write_state);
				state = load_ex(write_state, acquire);
				continue;
				#else
				assert(0 && "writing: lol nvm");
				#endif
			}
			/* int32_t tail = fetch_add_mod(&ring->writing_tail, size, ring->size); */
			/* const int32_t tail = fetch_add_ex(&ring->writing_tail, size, acq_rel); */
			/* const int32_t tail = fetch_add_mask(&ring->writing_tail, size, ring->size - 1); */
			// honestly its just clearer this way
			const int32_t tail = load_ex(&ring->writing_tail, acquire);
			const int32_t tail1 = tail + size & ring->size - 1;
			store_ex(&ring->writing_tail, tail1, release);

			// TODO: in-flight error checking

			/* memcpy(ring->addr + (tail & ring->size - 1), src, size); */
			for (int32_t i = 0; i < n; ++i)
			{
				memcpy(ring->addr + tail + i * elem_size, src, elem_size);
			}
			cache_write(tail, tail1);
			/* if (tail1 < tail0) fetch_or_ex(&ring->direction, REVERSE, release); */
			store_ex(&ring->tail, tail1, release);
			/* msg_print_all(); */
			unlock_and_wake(write_state);
			wake_if_only_waiting(&ring->read_state);
			return size;
		}
		int32_t wait_state = WAITING | state;
		if (!(state & EVICTED) && comp_swap_ex(write_state, &state, wait_state, acq_rel, acquire))
		{
			sync_addr_wait(write_state, &wait_state);
		}
		state = load_ex(write_state, acquire);
	}
	return ATOM_RING_EVICTED;
}


int32_t atom_ring_flush(struct atom_ring* ring, atom_ring_flush_func flush)
{
	_Atomic(int32_t)* const read_state = &ring->read_state;
	int32_t state = load_ex(read_state, acquire);
	while (!(state & EVICTED))
	{
		state &= ~(LOCKED | EVICTED);
		if (comp_swap_ex(read_state, &state, LOCKED | state, acq_rel, acquire))
		{
			const int32_t head = load_ex(&ring->reading_head, acquire);
			const int32_t tail = load_ex(&ring->tail, acquire);
			const int32_t size = tail > head ? tail - head : tail + ring->size - head;
			store_ex(&ring->reading_head, tail, release);
			flush(ring->addr + head, size);
			cache_read(head, tail);
			store_ex(&ring->head, tail, release);
			unlock_and_wake(read_state);
			wake_if_only_waiting(&ring->write_state);
			return size;
		}
		int32_t wait_state = WAITING | state;
		if (!(state & EVICTED) && comp_swap_ex(read_state, &state, wait_state, acq_rel, acquire))
		{
			sync_addr_wait(read_state, &wait_state);
		}
		state = load_ex(read_state, acquire);
	}
	return ATOM_RING_EVICTED;
}

int32_t atom_ring_concurrent_write(struct atom_ring* ring, void* src, const int32_t size, struct tx_range* write_range)
{
	assert(0 < size);
	assert(size <= ring->size - 1);
	_Atomic(int32_t)* const write_state = &ring->write_state;
	int32_t state = load_ex(write_state, acquire);
	while (!(state & EVICTED))
	{
		state &= ~(LOCKED | EVICTED);
		/* while (writeable_size(ring) < size) */
		/* { */
		/* 	state |= WAITING; */
		/* 	sync_addr_wait(write_state, &state); */
		/* 	state ^= WAITING; */
		/* } */
		if (size <= writeable_size(ring) &&
			comp_swap_ex(write_state, &state, LOCKED | state, acq_rel, acquire))
		{
			if (writeable_size(ring) < size)
			{
				#ifndef NOFRAUD
				unlock_and_wake(write_state);
				state = load_ex(write_state, acquire);
				continue;
				#else
				fetch_or(write_state, ERR_STOP);
				fetch_or(&ring->read_state, ERR_STOP);
				assert(0 && "writing: lol nvm");
				#endif
			}
			const int32_t tail = load_ex(&ring->writing_tail, acquire);
			const int32_t tail1 = tail + size & ring->size - 1;
			store_ex(&ring->writing_tail, tail1, release);
			unlock_and_wake(write_state);
			cache_write(tail, tail1);
			if (write_range != NULL)
			{
				write_range->start = tail;
				write_range->end = tail1;
			}
			// TODO: SANCHECK IN FLIGHT ERROR CHECKING
			for (int32_t i = 0; i < size; ++i)
			{
				assert(ring->addr[tail + i] == 0x00);
			}
			memcpy(ring->addr + tail, src, size);
			// TODO: WAIT dont SPIN
			while (load_ex(&ring->tail, acquire) != tail)
			{ ; }
			store_ex(&ring->tail, tail1, release);
			wake_if_only_waiting(&ring->read_state);
			return size;
		}
		int32_t wait_state = WAITING | state;
		if (!(state & EVICTED) && comp_swap_ex(write_state, &state, wait_state, acq_rel, acquire))
		{
			sync_addr_wait(write_state, &wait_state);
		}
		state = load_ex(write_state, acquire);
	}
	return ATOM_RING_EVICTED;
}

int32_t atom_ring_concurrent_read(struct atom_ring* ring, void* dest, const int32_t size, struct tx_range* read_range)
{
	assert(size > 0);
	assert(size <= ring->size - 1);
	_Atomic(int32_t)* const read_state = &ring->read_state;
	int32_t state = load_ex(read_state, acquire);
	while (!(state & EVICTED))
	{
		state &= ~(LOCKED | EVICTED);
		/* while(readable_size(ring) < size) */
		/* { */
		/* 	state |= WAITING; */
		/* 	sync_addr_wait(read_state, &state); */
		/* 	state ^= WAITING; */
		/* } */
		if (size <= readable_size(ring) &&
			comp_swap_ex(read_state, &state, LOCKED | state, acq_rel, acquire))
		{
			if (readable_size(ring) < size)
			{
				#ifndef NOFRAUD
				unlock_and_wake(read_state);
				state = load_ex(read_state, acquire);
				continue;
				#else
				fetch_or(read_state, ERR_STOP);
				fetch_or(&ring->write_state, ERR_STOP);
				assert(0 && "reading: lol");
				#endif
			}
			/* int32_t head = fetch_add_mod(&ring->reading_head, size, ring->size); */
			/* const uint32_t head = fetch_add(&ring->reading_head, size); */
			const int32_t head = load_ex(&ring->reading_head, acquire);
			const int32_t head1 = head + size & ring->size - 1;
			store_ex(&ring->reading_head, head1, release);
			unlock_and_wake(read_state);
			cache_read(head, head1);
			if (read_range != NULL)
			{
				read_range->start = head;
				read_range->end = head1;
			}
			memcpy(dest, ring->addr + (head & ring->size - 1), size);
			// TODO: delete this
			memset(ring->addr + (head & ring->size - 1), 0x00, size);
			// TODO: WAIT dont SPIN
			while (load_ex(&ring->head, acquire) != head) { ; }
			/* if (!(load_ex(read_state, acquire) & ERR_STOP)) cache_update_head(head, head1); */
			store_ex(&ring->head, head1, release);
			wake_if_only_waiting(&ring->write_state);
			return size;
		}
		int32_t waiting_state = WAITING | state;
		if (!(state & EVICTED) && comp_swap_ex(read_state, &state, waiting_state, acq_rel, acquire))
		{
			sync_addr_wait(read_state, &waiting_state);
		}
		state = load_ex(read_state, acquire);
	}
	return ATOM_RING_EVICTED;
}

// NOTE: flush is NOT concurrent
int32_t atom_ring_flushing_concurrent_write(struct atom_ring* ring, void* src, const int32_t size, atom_ring_flush_func flush)
{
	assert(0 < size);
	assert(size <= ring->size - 1);
	_Atomic(int32_t)* const write_state = &ring->write_state;
	int32_t state = load_ex(write_state, acquire);
	while (!(state & EVICTED))
	{
		while (!(state & EVICTED) && writeable_size(ring) < size)
		{
			state &= ~(LOCKED | EVICTED);
			if (comp_swap_ex(write_state, &state, LOCKED | state, acq_rel, acquire))
			{
				atom_ring_flush(ring, flush);
				unlock_and_wake(write_state);
			}
			int32_t wait_state = state | WAITING;
			if (!(state & EVICTED) && comp_swap_ex(write_state, &state, wait_state, acq_rel, acquire))
			{
				sync_addr_wait(write_state, &wait_state);
			}
			state = load_ex(write_state, acquire);
		}
		state &= ~(LOCKED | EVICTED);
		if (comp_swap_ex(write_state, &state, LOCKED | state, acq_rel, acquire))
		{
			if (writeable_size(ring) < size)
			{
				#ifndef NOFRAUD
				unlock_and_wake(write_state);
				state = load_ex(write_state, acquire);
				continue;
				#else
				fetch_or(write_state, ERR_STOP);
				fetch_or(&ring->read_state, ERR_STOP);
				assert(0 && "writing: lol nvm");
				#endif
			}
			const int32_t tail = load_ex(&ring->writing_tail, acquire);
			const int32_t tail1 = tail + size & ring->size - 1;
			store_ex(&ring->writing_tail, tail1, release);
			unlock_and_wake(write_state);
			cache_write(tail, tail1);
			// TODO: SANCHECK IN FLIGHT ERROR CHECKING
			for (int32_t i = 0; i < size; ++i)
			{
				assert(ring->addr[tail + i] == 0x00);
			}
			memcpy(ring->addr + tail, src, size);
			// TODO: WAIT dont SPIN
			while (load_ex(&ring->tail, acquire) != tail)
			{ ; }
			store_ex(&ring->tail, tail1, release);
			wake_if_only_waiting(&ring->read_state);
			return size;
		}
		int32_t wait_state = WAITING | state;
		if (!(state & EVICTED) && comp_swap_ex(write_state, &state, wait_state, acq_rel, acquire))
		{
			sync_addr_wait(write_state, &wait_state);
		}
		state = load_ex(write_state, acquire);
	}
	return ATOM_RING_EVICTED;
}


void atom_ring_evict_all(struct atom_ring* ring)
{
	if (swap_ex(&ring->write_state, EVICTED, acq_rel) & WAITING)
	{
		sync_addr_wake_all(&ring->write_state);
	}
	if (swap_ex(&ring->read_state, EVICTED, acq_rel) & WAITING)
	{
		sync_addr_wake_all(&ring->read_state);
	}
}

#include "memery.h"
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include "error_macros.h"

// fuck it terabyte
/* #define TOTAL_BYTES 1099511627776 */
/* #define TOTAL_BYTES 268435456 */
/* #define TOTAL_BYTES 17179869184 /\* 16GB *\/ */
// #define TOTAL_BYTES 8589934592 /* 8GB */
#define TOTAL_BYTES (1024 * 1024 * 1024)


static int32_t assert_write(struct atom_ring* ring, void* src, const int32_t size, const int32_t expected_writeable_size)
{
	assert(writeable_size(ring) == expected_writeable_size);
	/* return atom_ring_concurrent_write(ring, src, size, NULL); */
	return atom_ring_write(ring, src, size);
}

static int32_t assert_read(struct atom_ring* ring, void* dest, const int32_t size, const int32_t expected_readable_size)
{
	assert(readable_size(ring) == expected_readable_size);
	/* return atom_ring_concurrent_read(ring, dest, size, NULL); */
	return atom_ring_read(ring, dest, size);
}

static void blocked_read(struct atom_ring* ring)
{
	uint8_t temp;
	assert(atom_ring_concurrent_read(ring, &temp, 1, NULL) == ATOM_RING_EVICTED);
	return;
}

static void blocked_write(struct atom_ring* ring)
{
	uint8_t temp = 0xff;
	assert(atom_ring_concurrent_write(ring, &temp, 1, NULL) == ATOM_RING_EVICTED);
	return;
}

enum san_status
{
	DEFAULT = 0x00,
	TRANSACTING,
	FINISHED
};

struct san_struct
{
	struct atom_ring* ring;
	_Atomic(int64_t)* cum;
	_Atomic(enum san_status) status;
};
static _Atomic(uint8_t) concurrent_test_status = 0;

static int64_t threaded_write(struct san_struct* args)
{
	struct atom_ring* ring = args->ring;
	_Atomic(int64_t)* cumulative = args->cum;
	int32_t bytes_written = 0;
	uint8_t src[1024] = {0};
	memset(src, 0xff, 1024);
	while(load_ex(&concurrent_test_status, acquire) == 0)
	{ ; }
	while(load_ex(&concurrent_test_status, acquire))
	{
		/* const int32_t size = (rand() & 0x200) + 1; */
		const int32_t size = (rand() & 400) + 1;
		struct tx_range range = {0};
		store_ex(&args->status, TRANSACTING, release);
		const int32_t res = atom_ring_concurrent_write(ring, src, size, &range);
		store_ex(&args->status, DEFAULT, release);
		if (res != ATOM_RING_EVICTED)
		{
			bytes_written += res;
			fetch_add_ex(cumulative, size, release);
		}
		else
		{
			break;
		}
	}
	store_ex(&args->status, FINISHED, release);
	return bytes_written;
}

static int64_t threaded_read(struct san_struct* args)
{
	struct atom_ring* ring = args->ring;
	_Atomic(int64_t)* cumulative = args->cum;
	int32_t bytes_read = 0;
	int64_t remaining;
	uint8_t dest[1024] = {0};
	while(load_ex(&concurrent_test_status, acquire) == 0)
	{ ; }
	while (load_ex(&concurrent_test_status, acquire))
	{
		const int32_t size = (rand() & 0x400) + 1;
		struct tx_range range = {0};
		args->status = TRANSACTING;
		int32_t res = atom_ring_concurrent_read(ring, dest, size, &range);
		args->status = DEFAULT;
		if (res == ATOM_RING_EVICTED)
		{
			break;
		}
		for (int32_t i = 0; i < size; ++i)
		{
			if (dest[i] != 0xff)
			{
				res = ATOM_RING_INVALID_READ;
				break;
			}
		}
		if (res == ATOM_RING_INVALID_READ)
		{
			global_ftx_lock();
			store_ex(&ring->read_state, ERR_STOP, release);
			store_ex(&ring->write_state, ERR_STOP, release);
			print_ring(ring, range.start, range.end % ring->size);
			printf("\nBAD: read: [%d, %d)\n", range.start, range.end);
			for (int32_t j = (range.start / 32 * 32); j < (range.end / 32 * 32) + 1; ++j)
			{
				if (j % 32 == 0)
				{
					printf("\n%04d ", j);
				}
			}
			const int32_t row0 = (range.start / 32) * 32;
			printf("%04d", row0);
			for (int32_t i = row0; i < range.start; ++i)
			{
				if (i % 16)
				{
					printf(" --");
				}
				else
				{
					printf("  --");
				}
			}
			for (int32_t i = 0; i < size; ++i)
			{
				if ((range.start + i) % 32 == 0)
				{
					printf("\n%04d ", (range.start + i) & (ring->size - 1));
				}
				else if ((range.start + i) % 16 == 0)
				{
					printf(" ");
				}
				printf(" %02x", dest[i]);
			}
			printf("\nREAD INVALID MEMORY!\n");
			msg_print_all();
			fflush(stdout);
			assert(0x00 && "error in src/atom_ring.c::read_job()");
		}
		assert(res == size);
		bytes_read += size;
		fetch_add_ex(cumulative, size, release);
	}
	args->status = FINISHED;
	return bytes_read;
}


#ifdef ATOM_RING_BANDWIDTH_TEST
int main(int argc, char** argv)
#else
int atom_ring_sancheck(void)
#endif
{
	const char spinner[4] = { '-', '/', '|', '\\' };
	struct atom_ring test_ring = {0};
	ring_map((struct ring_buffer*)&test_ring, 4096, "sancheck");
	memset(test_ring.addr, 0x00, 2 * test_ring.size);
	test_ring.addr[test_ring.size + 1] = 0xff;
	test_ring.addr[0] = 0xaa;
	uint32_t foo = 0x44332211;
	memcpy(test_ring.addr + test_ring.size - 2, &foo, 4);
	/* print_ring(&test_ring, test_ring.size - 2, 2); */
	/* assert(0); */
	memset(test_ring.addr, 0x00, test_ring.size);
	/* print_ring(&test_ring, test_ring.size - 2, 2); */
	/* assert(0); */
	const int32_t size = test_ring.size;
	printf("- writeable: %d (head: %d, tail: %d)\n", writeable_size(&test_ring), test_ring.head, test_ring.tail);
	assert(writeable_size(&test_ring) == size - 1);
	printf("- filling ring. ");
	for (int32_t i = 0; i < size - 1; ++i)
	{
		assert(writeable_size(&test_ring) == size - 1 - i);
		uint8_t byte = 0xff;
		/* atom_ring_write(&test_ring, &byte, 1); */
		assert_write(&test_ring, &byte, 1, size - 1 - i);
	}
	/* assert(test_ring.direction == REVERSE); */
	printf("DONE\n- verifying ring full ");
	for (int32_t i = 0; i < size - 1; ++i)
	{
		assert(test_ring.addr[i] == 0xff);
	}
	printf("DONE\n- readable: %d (head: %d, tail: %d)\n", readable_size(&test_ring), test_ring.head, test_ring.tail);
	assert(readable_size(&test_ring) == size - 1);
	printf("- emptying ring. ");
	fflush(stdout);
	for (int32_t i = 0; i < size - 1; ++i)
	{
		/* assert(readable_size(&test_ring) == size - i); */
		/* fflush(stdout); */
		uint8_t byte;
		assert_read(&test_ring, &byte, 1, size - i - 1);
		assert(byte == 0xff);
	}
	assert(readable_size(&test_ring) == 0);
	/* assert(test_ring.direction == FORWARD); */
	printf("DONE\n- verifying ring empty ");
	fflush(stdout);
	for (int32_t i = 0; i < size; ++i)
	{
		assert(test_ring.addr[i] == 0x00);
	}
	printf("DONE\n");
	fflush(stdout);
	// offset a scooch to check if the ring ACTUALL works
	uint32_t temp = -1u;
	atom_ring_write(&test_ring, &temp, sizeof temp);
	assert(readable_size(&test_ring) >= sizeof temp);
	atom_ring_read(&test_ring, &temp, sizeof temp);
	printf("- wrap test. ");
	fflush(stdout);
	const int32_t pos0 = test_ring.tail;
	for (int32_t lap = 0; lap < 128; ++lap)
	{
		for (int32_t i = 0; i < size - 1; ++i)
		{
			putc(spinner[i & 0x03], stdout);
			/* putc('.', stdout); */
			/* printf("%d\n", i); */
			fflush(stdout);
			uint64_t double_word = -1u;
			assert(writeable_size(&test_ring) == 4096 - 1);
			assert(readable_size(&test_ring) == 0);
			assert_write(&test_ring, &double_word, 1, size - 1);
			assert(writeable_size(&test_ring) == 4095 - 1);
			assert(readable_size(&test_ring) == 1);
			assert_read(&test_ring, &double_word, 1, 1);
			assert(double_word == -1u);
			putc('\b', stdout);
		}
	}
	printf("DONE\n- verifying empy. ");
	for (uint32_t i = 0; i < test_ring.size; ++i)
	{
		putc(spinner[i & 0x03], stdout);
		/* assert(test_ring.addr[i] == 0x00); */
		putc('\b', stdout);
	}
	printf("DONE\n");
	printf("- fuzzy strncmp check ");
	fflush(stdout);
	uint8_t src[64];
	for (int32_t i = 0; i < 64; ++i)
	{
		src[i] = 0xff;
	}
	for (int32_t n = 0; n < 256; ++n)
	{
		const int32_t i0 = test_ring.tail & test_ring.size - 1;
		int32_t remaining;
		while ((remaining = writeable_size(&test_ring)) > 0)
		{
			putc(spinner[n & 0x03], stdout);
			fflush(stdout);
			const int32_t size = (remaining > 64 ? rand() & 0x3f : rand() % remaining) + 1;
			atom_ring_write(&test_ring, src, size);
			putc('\b', stdout);
		}
		for (int32_t i = i0; i < i0 + test_ring.size - 1; ++i)
		{
			assert(test_ring.addr[i] == 0xff && "FRUITFUL ASSERT!!!");
		}
		assert(strncmp(test_ring.addr, test_ring.addr + test_ring.size, test_ring.size) == 0 && "WHATS GOING ON HERE???");
		test_ring.head = test_ring.tail;
		test_ring.reading_head = test_ring.tail;
	}
	printf("DONE!\n");
	fflush(stdout);
	//  spin up some mfn THREADS
	#define NUM_WRITERS 4
	#define NUM_READERS 4
	store_ex(&msg_ring.head, 0u, seq_cst);
	store_ex(&test_ring.head, 0, seq_cst);
	store_ex(&test_ring.tail, 0, seq_cst);
	store_ex(&test_ring.reading_head, 0, seq_cst);
	store_ex(&test_ring.writing_tail, 0, seq_cst);
	memset(test_ring.addr, 0x00, test_ring.size);
	assert(readable_size(&test_ring) == 0);
	assert(writeable_size(&test_ring) == test_ring.size - 1);
	fflush(stdout);
	pthread_t readers[NUM_READERS] = {0};
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	for (int32_t i = 0; i < NUM_READERS; ++i)
	{
		pthread_create(readers + i, &attr, blocked_read, &test_ring);
	}
	printf("- testing read eviction ");
	fflush(stdout);
	/* sleep(1); */
	atom_ring_evict_all(&test_ring);
	for (int32_t i = 0; i < NUM_READERS; ++i)
	{
		pthread_join(readers[i], NULL);
	}
	printf("DONE!\n");
	fflush(stdout);
	store_ex(&test_ring.write_state, FREE, release);
	store_ex(&test_ring.read_state, FREE, release);
	/* ring_unmap((struct ring_buffer*)&test_ring); */
	pthread_t writers[NUM_WRITERS] = {0};
	store_ex(&test_ring.tail, 4095, release);
	store_ex(&test_ring.writing_tail, 4095, release);
	for (int32_t i = 0; i < NUM_WRITERS; ++i)
	{
		pthread_create(writers + i, &attr, blocked_write, &test_ring);
	}
	printf("- testing writer eviction ");
	fflush(stdout);
	/* sleep(1); */
	atom_ring_evict_all(&test_ring);
	for (int32_t i = 0; i < NUM_WRITERS; ++i)
	{
		pthread_join(writers[i], NULL);
	}
	printf("DONE!\n");
	fflush(stdout);
	store_ex(&test_ring.tail, 0, release);
	store_ex(&test_ring.writing_tail, 0, release);
	store_ex(&test_ring.write_state, FREE, release);
	store_ex(&test_ring.read_state, FREE, release);
	_Atomic(int64_t) bytes_written = 0;
	_Atomic(int64_t) bytes_read = 0;
	struct san_struct write_args[NUM_WRITERS];
	struct san_struct read_args[NUM_READERS];
	for (int32_t i = 0; i < NUM_WRITERS; ++i)
	{
		write_args[i] = (struct san_struct){.ring = &test_ring, .cum = &bytes_written };
		pthread_create(writers + i, &attr, threaded_write, write_args + i);
	}
	for (int32_t i = 0; i < NUM_READERS; ++i)
	{
		read_args[i] = (struct san_struct){.ring = &test_ring, .cum = &bytes_read };
		pthread_create(readers + i, &attr, threaded_read, read_args + i);
	}
	printf("- irregular concurrent read/write test ");
	fflush(stdout);
	uint32_t i = 0;
	store_ex(&concurrent_test_status, 1, release);
	const size_t t0 = time(NULL);
	while(load_ex(&bytes_written, acquire) < TOTAL_BYTES || load_ex(&bytes_read, acquire) < TOTAL_BYTES)
	{ ; }
	const size_t t1 = time(NULL);
	printf("DONE in %zu seconds, total bytes read & write (& zeroed): %lld\n", t1 - t0, TOTAL_BYTES);
	printf("- joining threads\n");
	store_ex(&concurrent_test_status, 0, release);
	fflush(stdout);
	// NOTE: they're NOT DEAD
	for (int32_t i = 0; i < NUM_WRITERS; ++i)
	{
		int64_t res;
		atom_ring_evict_all(&test_ring);
		pthread_join(writers[i], &res);
		printf("  + writer%d wrote %ld/%ld bytes\n", i, res, TOTAL_BYTES);
		fflush(stdout);

	}
	for (int32_t i = 0; i < NUM_READERS; ++i)
	{
		int64_t res;
		pthread_join(readers[i], &res);
		printf("  + reader%d read %ld/%ld bytes\n", i, res, TOTAL_BYTES);
		fflush(stdout);
	}
	assert(bytes_written >= TOTAL_BYTES);
	assert(bytes_read >= TOTAL_BYTES);
	ring_unmap(&test_ring);
	return 0;
}
