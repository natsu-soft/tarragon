#include "atom_ring.h"
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include "atomics.h"
#include <string.h>
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
	_Atomic(int32_t) pos0;
	_Atomic(int32_t) pos1;
	_Atomic(enum dbg_tag) tag;
};

#define MSG_RING_SIZE 32

struct
{
	_Atomic(uint32_t) head;
	struct dbg_msg messages[MSG_RING_SIZE];
} msg_ring = {0};
/* static struct ticketlock msg_lock = {0}; */

static void cache_write(const int32_t start, const int32_t end)
{
	const int32_t idx = fetch_add_ex(&msg_ring.head, 1, seq_cst) % MSG_RING_SIZE;
	/* printf("%d\n", idx); */
	struct dbg_msg* temp = msg_ring.messages + idx;
	temp->pos0 = start;
	temp->pos1 = end;
	temp->tag = CACHE_WRITE;
}

static void cache_read(const int32_t start, const int32_t end)
{
	const int32_t idx = fetch_add_ex(&msg_ring.head, 1, seq_cst) % MSG_RING_SIZE;
	/* printf("%d\n", idx); */
	struct dbg_msg* temp = msg_ring.messages + idx;
	temp->pos0 = start;
	temp->pos1 = end;
	temp->tag = CACHE_READ;
}

static void cache_update_tail(const int32_t pos0, const int32_t pos1)
{
	const int32_t idx = fetch_add_ex(&msg_ring.head, 1, seq_cst) % MSG_RING_SIZE;
	/* printf("%d\n", idx); */
	struct dbg_msg* temp = msg_ring.messages + idx;
	store_ex(&temp->pos0, pos0, release);
	store_ex(&temp->pos1, pos1, release);
	store_ex(&temp->tag, UPDATE_TAIL, release);
}

static void cache_update_head(const int32_t pos0, const int32_t pos1)
{
	const int32_t idx = fetch_add_ex(&msg_ring.head, 1, seq_cst) % MSG_RING_SIZE;
	/* printf("%d\n", idx); */
	struct dbg_msg* temp = msg_ring.messages + idx;
	store_ex(&temp->pos0, pos0, release);
	store_ex(&temp->pos1, pos1, release);
	store_ex(&temp->tag, UPDATE_HEAD, release);
}


static void msg_print_all(void)
{
	const int32_t i0 = msg_ring.head - MSG_RING_SIZE > 0 ? msg_ring.head - MSG_RING_SIZE : 0;
	thread_fence(acquire);
	for (int32_t i = i0; i < msg_ring.head; ++i)
	{
		struct dbg_msg* msg = msg_ring.messages + (i % MSG_RING_SIZE);

		switch(load_ex(&msg->tag, acquire))
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

static inline void stream_fence_asm(void)
{
	__asm__ __volatile__("sfence" ::: "memory");
}

static inline void memory_fence_asm(void)
{
	__asm__ __volatile__("" ::: "memory");
}

enum
{
	FREE		= 0x00,
	LOCKED		= 0x01,
	WAITING		= 0x02,
	ERR_STOP	= 0x04,
};

static inline const int32_t writeable_size(struct atom_ring* ring)
{
	const int32_t size = ring->size;
	const int32_t head = load_ex(&ring->head, acquire);
	const int32_t tail = load_ex(&ring->writing_tail, acquire);
	/* const int32_t dir = load_ex(&ring->direction, acquire); */
	/* return head <= size ? size - (tail - head) : head - tail; */
	return  size - (tail - head);
}

static inline const int32_t readable_size(struct atom_ring* ring)
{
	const int32_t head = load_ex(&ring->reading_head, acquire);
	const int32_t tail = load_ex(&ring->tail, acquire);
	return tail - head;
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
	return 0;
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

int32_t atom_ring_write(struct atom_ring* ring, void* src, const int32_t size)
{
	assert(0 < size);
	assert(size <= ring->size);
	_Atomic(int32_t)* const read_state = &ring->read_state;
	_Atomic(int32_t)* const write_state = &ring->write_state;
	int32_t state = load(write_state) & ~LOCKED;
	for (;;)
	{
		while(writeable_size(ring) < size)
		{
			state |= WAITING | fetch_or_ex(write_state, WAITING, release);
			sync_addr_wait(write_state, &state);
			state ^= WAITING;
		}
		if (comp_swap_ex(write_state, &state, LOCKED | state, acq_rel, acquire))
		{
			if (writeable_size(ring) < size)
			{
				assert(0 && "writing: lol nvm");
			}
			/* int32_t tail = fetch_add_mod(&ring->writing_tail, size, ring->size); */
			int32_t tail = fetch_add_ex(&ring->writing_tail, size, acq_rel);

			// TODO: in-flight error checking

			memcpy(ring->addr + tail, src, size);
			const int32_t tail1 = tail + size;
			cache_write(tail, tail1);
			/* if (tail1 < tail0) fetch_or_ex(&ring->direction, REVERSE, release); */
			store_ex(&ring->tail, tail1, release);
			unlock_and_wake(write_state);
			wake_if_only_waiting(read_state);
			return size;
		}
		int32_t wait_state = WAITING | state;
		if (comp_swap_ex(write_state, &state, wait_state, acq_rel, acquire))
		{
			sync_addr_wait(write_state, &wait_state);
		}
		state = FREE;
	}
	return ATOM_RING_TIMEOUT;
}

int32_t atom_ring_read(struct atom_ring* ring, void* dest, const int32_t size)
{
	assert(0 < size);
	assert(size <= ring->size);
	_Atomic(int32_t)* const read_state = &ring->read_state;
	_Atomic(int32_t)* const write_state = &ring->write_state;
	/* int32_t state = load_ex(read_state, acquire) & ~(LOCKED | ERR_STOP); */
	int32_t state = load_ex(read_state, acquire) & ~LOCKED;
	/* for (int32_t i = 0; i < 16; ++i) */
	for (;;)
	{
		while (readable_size(ring) < size)
		{
			state = WAITING | fetch_or_ex(read_state, WAITING, release);
			sync_addr_wait(read_state, &state);
			state ^= WAITING;
		}
		if (comp_swap_ex(read_state, &state, LOCKED | state, acq_rel, acquire))
		{
			if (readable_size(ring) < size)
			{
				assert(0 && "reading: lol nvm");
			}
			/* int32_t head = fetch_add_mod(&ring->reading_head, size, ring->size); */
			int32_t head = fetch_add_ex(&ring->reading_head, size, acq_rel);

			// TODO: in-flight error checking

			memcpy(dest, ring->addr + head, size);
			memset(ring->addr + head, 0x00, size);

			cache_read(head, head + size);
			if (head + size > ring->size)
			{
				fetch_sub_ex(&ring->reading_head, ring->size, acq_rel);
				fetch_sub_ex(&ring->writing_tail, ring->size, acq_rel);
				fetch_sub_ex(&ring->tail, ring->size, acq_rel);
				head -= ring->size;
			}
			const int32_t head1 = head + size;
			store_ex(&ring->head, head1, release);
			unlock_and_wake(read_state);
			wake_if_only_waiting(write_state);
			return size;
		}
		int32_t wait_state = WAITING | state;
		if (comp_swap_ex(read_state, &state, wait_state, acq_rel, acquire))
		{
			sync_addr_wait(read_state, &wait_state);
		}
		state = FREE;
	}
	return ATOM_RING_TIMEOUT;
}


int32_t atom_ring_n_write(struct atom_ring* ring, void* src, const int32_t n, const int32_t elem_size)
{
	assert(0 && "TODO");
	return -1;
}

// TODO: stop swapping for concurrent write/read
int32_t atom_ring_concurrent_write(struct atom_ring* ring, void* src, const int32_t size, struct tx_range* write_range)
{
	assert(0 < size);
	assert(size <= ring->size);
	_Atomic(int32_t)* const write_state = &ring->write_state;
	int32_t state = load(write_state) & ~(LOCKED | ERR_STOP);
	for (;;)
	{
		while (writeable_size(ring) < size)
		{
			state |= WAITING;
			sync_addr_wait(write_state, &state);
			state ^= WAITING;
		}
		if (comp_swap_ex(write_state, &state, LOCKED | state, acq_rel, acquire))
		{
			if (writeable_size(ring) < size)
			{
				fetch_or(write_state, ERR_STOP);
				fetch_or(&ring->read_state, ERR_STOP);
				assert(0 && "writing: lol nvm");
				/* state = FREE; */
				/* unlock_and_wake(write_state); */
				/* continue; */
			}
			/* int32_t tail = fetch_add_mod(&ring->writing_tail, size, ring->size); */
			int32_t tail = fetch_add_ex(&ring->writing_tail, size, acq_rel);
			cache_write(tail, tail + size);
			if (write_range != NULL)
			{
				write_range->start = tail;
				write_range->end = tail + size;
			}
			unlock_and_wake(write_state);
			thread_fence(seq_cst);

			// SANCHECK IN FLIGHT ERROR CHECKING
			/* for (int32_t i = 0; i < size; ++i) */
			/* { */
			/* 	if(ring->addr[tail + i] != 0x00) */
			/* 	{ */
			/* 		return ATOM_RING_OVERWRITE; */
			/* 	} */
			/* } */
			memcpy(ring->addr + tail, src, size);
			thread_fence(seq_cst);
			/* memory_fence_asm(); */
			/* Update the tail used for readable size calculation */
			int32_t tail0 = load_ex(&ring->tail, acquire);
			int32_t tail1 = (tail + size) % ring->size;
			// TODO: consider WAITING instead of SPINNING
			while ((tail0 = load_ex(&ring->tail, acquire)) != tail)
			{ ; }
			/* if (!(load_ex(write_state, acquire) & ERR_STOP)) cache_update_tail(tail, tail1); */
			store_ex(&ring->tail, tail1, release);
			wake_if_only_waiting(&ring->read_state);
			return size;
		}
		int32_t wait_state = WAITING | state;
		if (comp_swap_ex(write_state, &state, wait_state, acq_rel, acquire))
		{
			sync_addr_wait(write_state, &wait_state);
		}
		state = FREE;
	}
	return ATOM_RING_TIMEOUT;
}

int32_t atom_ring_concurrent_read(struct atom_ring* ring, void* dest, const int32_t size, struct tx_range* read_range)
{
	assert(0 < size);
	assert(size <= ring->size);
	_Atomic(int32_t)* const read_state = &ring->read_state;
	int32_t state = load_ex(read_state, acquire) & ~(LOCKED | ERR_STOP);
	for (;;)
	{
		while(readable_size(ring) < size)
		{
			state |= WAITING;
			sync_addr_wait(read_state, &state);
			state ^= WAITING;
		}
		if (comp_swap_ex(read_state, &state, LOCKED | state, acq_rel, acquire))
		{
			if (readable_size(ring) < size)
			{
				fetch_or(read_state, ERR_STOP);
				fetch_or(&ring->write_state, ERR_STOP);
				assert(0 && "reading: lol");
			}
			/* int32_t head = fetch_add_mod(&ring->reading_head, size, ring->size); */
			int32_t head = fetch_add(&ring->reading_head, size);
			cache_read(head, head + size);
			if (head + size >= ring->size)
			{
				fetch_sub_ex(&ring->reading_head, ring->size, release);
				fetch_sub_ex(&ring->writing_tail, ring->size, release);
				fetch_sub_ex(&ring->tail, ring->size, release);
			}
			if (read_range != NULL)
			{
				read_range->start = head;
				read_range->end = head + size;
			}
			unlock_and_wake(read_state);

			thread_fence(seq_cst);
			/* for (int32_t i = 0; i < size; ++i) */
			/* { */
			/* 	if (ring->addr[head + i] != 0xff) */
			/* 	{ */
			/* 		return ATOM_RING_INVALID_READ; */
			/* 	} */
			/* } */
			memcpy(dest, ring->addr + head, size);
			memset(ring->addr + head, 0x00, size);
			thread_fence(seq_cst);

			// TODO: WAIT dont SPIN
			const int32_t head1 = head + size >= ring->size ? head + size - ring->size : head + size;
			while (load_ex(&ring->head, acquire) != head) { ; }
			/* if (!(load_ex(read_state, acquire) & ERR_STOP)) cache_update_head(head, head1); */
			store_ex(&ring->head, head1, release);
			wake_if_only_waiting(&ring->write_state);
			return size;
		}
		int32_t waiting_state = WAITING | state;
		if (comp_swap_ex(read_state, &state, waiting_state, acq_rel, acquire))
		{
			sync_addr_wait(read_state, &waiting_state);
		}
		state = FREE;
	}
	return ATOM_RING_TIMEOUT;
}

#include "memery.h"
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include "error_macros.h"

#define TOTAL_BYTES 1073741824


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

struct san_struct
{
	struct atom_ring* ring;
	_Atomic(int64_t)* cum;
};

static void threaded_write(struct san_struct* args)
{
	struct atom_ring* ring = args->ring;
	_Atomic(int64_t)* cumulative = args->cum;
	int64_t remaining;
	while ((remaining = TOTAL_BYTES - load_ex(cumulative, acquire)) > 0)
	{
		uint8_t src[64] = {0};
		int32_t size = (remaining > 64 ? rand() & 0x3f : rand() % remaining) + 1;
		for (int32_t i = 0; i < size; ++i)
		{
			src[i] = 0xff;
		}
		struct tx_range range = {0};
		const int32_t res = atom_ring_concurrent_write(ring, src, size, &range);
		if (res == ATOM_RING_OVERWRITE)
		{
			global_ftx_lock();
			store_ex(&ring->read_state, ERR_STOP, release);
			store_ex(&ring->write_state, ERR_STOP, release);
			print_ring(ring, range.start, range.end % ring->size);
			msg_print_all();
			printf("\nBAD: wrote: [%d, %d)\n", range.start, range.end % ring->size);
			for (int32_t i = 0; i < res; ++i)
			{
				if (i % 32 == 0) putc('\n', stdout);
				else if (i % 16 == 0) putc(' ', stdout);
				printf("%02x", src[i]);
			}
			printf("\nOVERWROTE VALID MEMORY!\n");
			fflush(stdout);
			assert(0 && "error in " __FILE__"::write_job()");
		}
		assert(size == res);
		fetch_add_ex(cumulative, res, acq_rel);
		/* assert(load_ex(cumulative, acquire) > 0 && "WTF CHIEF"); */
	}
}

static void threaded_read(struct san_struct* args)
{
	struct atom_ring* ring = args->ring;
	_Atomic(int64_t)* cumulative = args->cum;
	int64_t cum;
	while ((cum = load_ex(cumulative, acquire)) < TOTAL_BYTES)
	{
		uint8_t dest[64] = {0};
		const int64_t remaining = TOTAL_BYTES - cum;
		int32_t size = (remaining > 64 ? rand() & 0x3f : rand() % remaining) + 1;
		struct tx_range range = {0};
		int32_t res = atom_ring_concurrent_read(ring, dest, size, &range);
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
			msg_print_all();
			printf("\nBAD: read: [%d, %d)\n", range.start, range.end % ring->size);
			for (int32_t i = 0; i < size; ++i)
			{
				if (i % 32 == 0) putc('\n', stdout);
				else if (i % 16 == 0) putc(' ', stdout);
				printf(" %02x", dest[i]);
			}
			printf("\nREAD INVALID MEMORY!\n");
			fflush(stdout);
			assert(0x00 && "error in " __FILE__"::read_job()");
		}
		assert(res == size);
		fetch_add_ex(cumulative, res, acq_rel);
	}
}

int atom_ring_sancheck(void)
{
	const char spinner[4] = { '-', '/', '|', '\\' };
	struct atom_ring test_ring = {0};
	ring_map((struct ring_buffer*)&test_ring, 4096, "sancheck");
	memset(test_ring.addr, 0x00, 2 * test_ring.size);
	test_ring.addr[test_ring.size + 1] = 0xff;
	test_ring.addr[0] = 0xaa;
	print_ring(&test_ring, 0, 2);
	assert(0);
	const int32_t size = test_ring.size;
	printf("- writeable: %d (head: %d, tail: %d)\n", writeable_size(&test_ring), test_ring.head, test_ring.tail);
	assert(writeable_size(&test_ring) == size);
	printf("- filling ring. ");
	for (int32_t i = 0; i < size; ++i)
	{
		assert(writeable_size(&test_ring) == size - i);
		uint8_t byte = 0xff;
		/* atom_ring_write(&test_ring, &byte, 1); */
		assert_write(&test_ring, &byte, 1, size - i);
	}
	/* assert(test_ring.direction == REVERSE); */
	printf("DONE\n- verifying ring full ");
	for (int32_t i = 0; i < size; ++i)
	{
		assert(test_ring.addr[i] == 0xff);
	}
	printf("DONE\n- readable: %d (head: %d, tail: %d)\n", readable_size(&test_ring), test_ring.head, test_ring.tail);
	assert(readable_size(&test_ring) == size);
	printf("- emptying ring. ");
	fflush(stdout);
	for (int32_t i = 0; i < size; ++i)
	{
		/* assert(readable_size(&test_ring) == size - i); */
		/* fflush(stdout); */
		uint8_t byte;
		assert_read(&test_ring, &byte, 1, size - i);
		assert(byte == 0xff);
	}
	assert(readable_size(&test_ring) == 0);
	/* assert(test_ring.direction == FORWARD); */
	printf("DONE\n- veritfying ring empty ");
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
	atom_ring_read(&test_ring, &temp, sizeof temp);
	printf("- wrap test. ");
	const int32_t pos0 = test_ring.tail;
	for (int32_t lap = 0; lap < 128; ++lap)
	{
		for (int32_t i = 0; i < size; ++i)
		{
			putc(spinner[i & 0x03], stdout);
			/* putc('.', stdout); */
			/* printf("%d\n", i); */
			fflush(stdout);
			uint64_t double_word = -1u;
			assert(writeable_size(&test_ring) == 4096);
			assert(readable_size(&test_ring) == 0);
			assert_write(&test_ring, &double_word, 1, size);
			assert(writeable_size(&test_ring) == 4095);
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
	fflush(stdout);
	// TODO: spin up some mfn THREADS
	ring_unmap((struct ring_buffer*)&test_ring);
	#define NUM_WRITERS 2
	#define NUM_READERS 4
	pthread_t writers[NUM_WRITERS] = {0};
	pthread_t readers[NUM_READERS] = {0};
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	_Atomic(int64_t) bytes_written = 0;
	_Atomic(int64_t) bytes_read = 0;
	struct san_struct read_args = { .ring = &test_ring, .cum = &bytes_read };
	struct san_struct write_args = { .ring = &test_ring, .cum = &bytes_written };
	for (int32_t i = 0; i < NUM_READERS; ++i)
	{
		pthread_create(readers + i, &attr, threaded_read, &read_args);
	}
	for (int32_t i = 0; i < NUM_WRITERS; ++i)
	{
		pthread_create(writers + i, &attr, threaded_write, &write_args);
	}
	store_ex(&msg_ring.head, 0u, release);
	printf("- irregular concurrent read/write test. ");
	for (uint8_t i = 0;
		 load_ex(&bytes_written, acquire) < TOTAL_BYTES || load_ex(&bytes_read, acquire) < TOTAL_BYTES;
		 ++i)
	{
		printf("%010ld/%010ld", load_ex(&bytes_written, acquire), load_ex(&bytes_read, acquire));
		sleep(1);
		if(load_ex(&test_ring.read_state, acquire) & ERR_STOP)
		{
			printf("write/read: %ld/%ld\n", bytes_written, bytes_read);
			printf("FAILED: IRREGULAR CONCURRENT IO TEST\n");
			return 1;
		}
		printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
		fflush(stdout);
	}
	printf("DONE\n");
	// They're dead but im just making sure
	for (int32_t i = 0; i < NUM_WRITERS; ++i)
	{
		pthread_join(writers[i], NULL);
	}
	for (int32_t i = 0; i < NUM_READERS; ++i)
	{
		pthread_join(readers[i], NULL);
	}
	assert(bytes_written >= TOTAL_BYTES);
	assert(bytes_read >= TOTAL_BYTES);
	ring_unmap(&test_ring);
	return 0;
}
