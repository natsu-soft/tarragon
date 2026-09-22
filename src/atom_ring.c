#include "atom_ring.h"
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include "atomics.h"
#include <string.h>
#include "sync_addr.h"
#include "threading.h"

struct dbg_msg
{
	_Atomic(int32_t) position;
	_Atomic(int32_t) size; // NOTE: negative sizes are READS, positive sizes are WRITES
};

#define MSG_RING_SIZE 65536

struct
{
	_Atomic(int64_t) head;
	struct dbg_msg messages[MSG_RING_SIZE];
} msg_ring = {0};
/* static struct ticketlock msg_lock = {0}; */

static void cache_write(const int32_t start, const int32_t size)
{
	struct dbg_msg* temp = msg_ring.messages + (fetch_add_ex(&msg_ring.head, 1, acq_rel) & MSG_RING_SIZE);
	store(&temp->position, start);
	store(&temp->size, size);
}

static void cache_read(const int32_t start, const int32_t size)
{
	struct dbg_msg* temp = msg_ring.messages + (fetch_add_ex(&msg_ring.head, 1, acq_rel) & MSG_RING_SIZE);
	store(&temp->position, start);
	store(&temp->size, -size);
}


static void msg_print_all(void)
{
	const int32_t i0 = msg_ring.head - MSG_RING_SIZE > 0 ? msg_ring.head - MSG_RING_SIZE : 0;
	printf("messages: [%d, %d)\n", i0, msg_ring.head);
	for (int32_t i = i0; i < msg_ring.head; ++i)
	{
		struct dbg_msg* msg = msg_ring.messages + (i & MSG_RING_SIZE);
		if (msg->size < 0)
		{
			printf("reading [%d, %d)\n", msg->position, msg->position - msg->size);
		}
		else
		{
			printf("writing [%d, %d)\n", msg->position, msg->position + msg->size);
		}
	}
	fflush(stdout);
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

enum
{
	FREE		= 0x00,
	LOCKED		= 0x01,
	WAITING		= 0x02,
	/* ERR_STOP	= 0x04, */
};

enum
{
	FORWARD	= 0x00,
	BACKWARD	= 0x01
};

static inline const int32_t writeable_size(struct atom_ring* ring)
{
	const int32_t size = ring->size;
	const int32_t head = load_ex(&ring->head, acquire);
	const int32_t tail = load_ex(&ring->tail, acquire);
	const int32_t dir = load_ex(&ring->direction, acquire);
	return dir == FORWARD ? size - (tail - head) : head - tail;
}

static inline const int32_t readable_size(struct atom_ring* ring)
{
	const int32_t size = ring->size;
	const int32_t head = load_ex(&ring->head, acquire);
	const int32_t tail = load_ex(&ring->tail, acquire);
	const int32_t dir = load_ex(&ring->direction, acquire);
	return dir == FORWARD ? tail - head : tail + size - head;
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
	global_ftx_lock();
	fflush(stdout);
	int32_t idx = 0;
	for (int32_t r = 0; r < ring->size; r += 32)
	{
		printf("%004d", r);
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
	printf("\nTX COUNTS:\n");
	for (int16_t r = 0; r < ring->size; r += 16)
	{
		printf("tx%004d", r);
		for (int16_t c = 0; c < 16; ++c)
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
			printf("%02d", load(ring->w_mask + idx));
		}
		putc(' ', stdout);
		for (int16_t c = 0; c < 16; ++c)
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
			printf("%02d", load(ring->r_mask + idx));
		}
		putc('\n', stdout);
	}
	putc('\n', stdout);
	global_ftx_unlock();
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
	/* int32_t state = load(write_state) & ~(LOCKED | ERR_STOP); */
	int32_t state = load(write_state) & ~LOCKED;
	for (int32_t i = 0; i < 16; ++i)
	{
		for(int32_t j = 0; writeable_size(ring) < size; ++j)
		{
			state |= WAITING | fetch_or_ex(write_state, WAITING, release);
			sync_addr_wait(write_state, &state);
			state ^= WAITING;
			if (j == 15)
			{
				return ATOM_RING_SIZE_CHECK_TIMEOUT;
			}
		}
		if (comp_swap_ex(write_state, &state, LOCKED | state, acq_rel, acquire))
		{
			if (writeable_size(ring) < size)
			{
				assert(0 && "writing: lol nvm");
			}
			int32_t tail = fetch_add_mod(&ring->writing_tail, size, ring->size);
			/* cache_write(tail, size); */
			const int32_t available = load_ex(&ring->used_size, relaxed);
			thread_fence(acquire);

			// TODO: in-flight error checking

			memcpy(ring->addr + tail, src, size);
			thread_fence(release);
			fetch_add_ex(&ring->used_size, size, acq_rel);
			for (int32_t i = 0; i < size; ++i)
			{
				fetch_add_ex(ring->w_mask + tail + i, 1, acq_rel);
			}
			const int32_t tail0 = load_ex(&ring->tail, acquire);
			const int32_t tail1 = (tail + size) % ring->size;
			if (tail1 < tail0) fetch_or_ex(&ring->direction, BACKWARD, release);
			store_ex(&ring->tail, tail1, release);
			unlock_and_wake(write_state);
			wake_if_only_waiting(read_state);
			return size;
		}
		int32_t wait_state = WAITING | state;
		if (comp_swap_ex(write_state, &state, wait_state, acq_rel, acquire))
		{
			sync_addr_wait(write_state, &wait_state);
			state ^= WAITING;
		}
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
	for (int32_t i = 0; i < 16; ++i)
	{
		/* while (readable_size(ring) < size) */
		for (int32_t j= 0; readable_size(ring) < size; ++j)
		{
			state = WAITING | fetch_or_ex(read_state, WAITING, release);
			sync_addr_wait(read_state, &state);
			state ^= WAITING;
			if (j == 15)
			{
				return ATOM_RING_SIZE_CHECK_TIMEOUT;
			}
		}
		if (comp_swap_ex(read_state, &state, LOCKED | state, acq_rel, acquire))
		{
			if (readable_size(ring) < size)
			{
				assert(0 && "reading: lol nvm");
			}
			int32_t head = fetch_add_mod(&ring->reading_head, size, ring->size);
			const int32_t available = load_ex(&ring->used_size, relaxed);
			thread_fence(acquire); // want to finalize all non-atomic memcpying into the ring before reading

			// TODO: in-flight error checking

			memcpy(dest, ring->addr + head, size);
			memset(ring->addr + head, 0x00, size);

			thread_fence(release); // finalize the non-atomic zeroing
			fetch_sub_ex(&ring->used_size, size, acq_rel);
			const int32_t head0 = load_ex(&ring->head, acquire);
			const int32_t head1 = (head + size) % ring->size;
			if (head1 < head0)
			{
				fetch_and_ex(&ring->direction, ~BACKWARD, release);
			}
			store_ex(&ring->head, (head + size) % ring->size, release);
			for (int32_t i = 0; i < size; ++i)
			{
				fetch_add_ex(ring->r_mask + head + i, 1, acq_rel);
			}
			unlock_and_wake(read_state);
			wake_if_only_waiting(write_state);
			return size;
		}
		int32_t wait_state = WAITING | state;
		if (comp_swap_ex(read_state, &state, wait_state, acq_rel, acquire))
		{
			sync_addr_wait(read_state, &wait_state);
			state ^= WAITING;
		}
	}
	return ATOM_RING_TIMEOUT;
}


int32_t atom_ring_n_write(struct atom_ring* ring, void* src, const int32_t n, const int32_t elem_size)
{
	assert(0 && "TODO");
	return -1;
}

int32_t atom_ring_write_concurrently(struct atom_ring* ring, void* src, const int32_t size)
{
	assert(0 && "TODO");
	return -1;
}

int32_t atom_ring_read_concurrently(struct atom_ring* ring, void* dest, const int32_t size)
{

	assert(0 && "TODO");
	return -1;
}

#include "memery.h"
#include "error_macros.h"

static int32_t write_now(struct atom_ring* ring, void* src, const int32_t size)
{
	assert(0 < size);
	assert(size <= ring->size);
	_Atomic(int32_t)* const read_state = &ring->read_state;
	_Atomic(int32_t)* const write_state = &ring->write_state;
	/* int32_t state = load(write_state) & ~(LOCKED | ERR_STOP); */
	int32_t state = load(write_state) & ~LOCKED;
	/* for (int32_t i = 0; i < 16; ++i) */
	/* { */
	/* 	for(int32_t j = 0; writeable_size(ring) < size; ++j) */
	/* 	{ */
	/* 		assert(0 && "ring saturated"); */
	/* 		state |= WAITING | fetch_or_ex(write_state, WAITING, release); */
	/* 		sync_addr_wait(write_state, &state); */
	/* 		state ^= WAITING; */
	/* 		if (j == 15) */
	/* 		{ */
	/* 			return ATOM_RING_SIZE_CHECK_TIMEOUT; */
	/* 		} */
	/* 	} */
		/* if (comp_swap_ex(write_state, &state, LOCKED | state, acq_rel, acquire)) */
		/* { */
			/* if (writeable_size(ring) < size) */
			/* { */
			/* 	assert(0 && "writing: lol nvm"); */
			/* } */
			int32_t tail = fetch_add_mod(&ring->writing_tail, size, ring->size);
			cache_write(tail, size);
			const int32_t available = load_ex(&ring->used_size, relaxed);
			thread_fence(acquire);

			// TODO: in-flight error checking

			memcpy(ring->addr + tail, src, size);
			thread_fence(release);
			fetch_add_ex(&ring->used_size, size, acq_rel);
			/* for (int32_t i = 0; i < size; ++i) */
			/* { */
			/* 	fetch_add_ex(ring->w_mask + tail + i, 1, acq_rel); */
			/* } */
			const int32_t tail0 = load_ex(&ring->tail, acquire);
			const int32_t tail1 = (tail + size) % ring->size;
			if (tail1 < tail0) fetch_or_ex(&ring->direction, BACKWARD, release);
			store_ex(&ring->tail, tail1, release);
			unlock_and_wake(write_state);
			wake_if_only_waiting(read_state);
			return size;
		/* } */
		/* else */
		/* { */
		/* 	assert(0 && "writer locked\n"); */
		/* } */
	/* 	int32_t wait_state = WAITING | state; */
	/* 	if (comp_swap_ex(write_state, &state, wait_state, acq_rel, acquire)) */
	/* 	{ */
	/* 		assert(0 && "writer busy"); */
	/* 		sync_addr_wait(write_state, &wait_state); */
	/* 		state ^= WAITING; */
	/* 	} */
	/* 	assert(0 && "writer eol"); */
	/* } */
	/* return ATOM_RING_TIMEOUT; */
}

int32_t read_now(struct atom_ring* ring, void* dest, const int32_t size)
{
	assert(0 < size);
	assert(size <= ring->size);
	_Atomic(int32_t)* const read_state = &ring->read_state;
	_Atomic(int32_t)* const write_state = &ring->write_state;
	/* int32_t state = load_ex(read_state, acquire) & ~(LOCKED | ERR_STOP); */
	int32_t state = load_ex(read_state, acquire) & ~LOCKED;
	/* for (;;) */
	/* { */
	/* 	while (readable_size(ring) < size) */
	/* 	/\* for (int32_t j= 0; readable_size(ring) < size; ++j) *\/ */
	/* 	{ */
	/* 		assert(0 && "reader starved\n"); */
	/* 		state = WAITING | fetch_or_ex(read_state, WAITING, release); */
	/* 		sync_addr_wait(read_state, &state); */
	/* 		state ^= WAITING; */
	/* 	} */
		if (comp_swap_ex(read_state, &state, LOCKED | state, acq_rel, acquire))
		{
			if (readable_size(ring) < size)
			{
				assert(0 && "reading: lol nvm");
			}
			int32_t head = fetch_add_mod(&ring->reading_head, size, ring->size);
			cache_read(head, size);
			const int32_t available = load_ex(&ring->used_size, relaxed);
			thread_fence(acquire); // want to finalize all non-atomic memcpying into the ring before reading

			// TODO: in-flight error checking

			memcpy(dest, ring->addr + head, size);
			memset(ring->addr + head, 0x00, size);

			thread_fence(release); // finalize the non-atomic zeroing
			fetch_sub_ex(&ring->used_size, size, acq_rel);
			const int32_t head0 = load_ex(&ring->head, acquire);
			const int32_t head1 = (head + size) % ring->size;
			if (head1 < head0)
			{
				fetch_and_ex(&ring->direction, ~BACKWARD, release);
			}
			store_ex(&ring->head, (head + size) % ring->size, release);
			/* for (int32_t i = 0; i < size; ++i) */
			/* { */
			/* 	fetch_add_ex(ring->r_mask + head + i, 1, acq_rel); */
			/* } */
			unlock_and_wake(read_state);
			wake_if_only_waiting(write_state);
			return size;
		}
		else
		{
			assert(0 && "reader locked");
		}
	/* 	} */
	/* 	int32_t wait_state = WAITING | state; */
	/* 	if (comp_swap_ex(read_state, &state, wait_state, acq_rel, acquire)) */
	/* 	{ */
	/* 		assert(0 && "reader busy\n"); */
	/* 		sync_addr_wait(read_state, &wait_state); */
	/* 		state ^= WAITING; */
	/* 	} */
	/* 	assert(0 && "reader eol"); */
	/* } */
	/* return ATOM_RING_TIMEOUT; */
}



#include <pthread.h>
#include <stdlib.h>


static int32_t assert_write(struct atom_ring* ring, void* src, const int32_t size, const int32_t expected_writeable_size)
{
	assert(writeable_size(ring) == expected_writeable_size);
	return atom_ring_write(ring, src, size);
}

static int32_t assert_read(struct atom_ring* ring, void* dest, const int32_t size, const int32_t expected_readable_size)
{
	assert(readable_size(ring) == expected_readable_size);
	return atom_ring_read(ring, dest, size);
}

#define TOTAL_BYTES 17179869184

struct san_struct
{
	struct atomic_ring* ring;
	_Atomic(int64_t)* cum;
};

static void write_job(struct san_struct* args)
{
	struct atomic_ring* ring = args->ring;
	_Atomic(int64_t)* cumulative = args->cum;
	int32_t remaining = load_ex(cumulative, acquire);
	/* while ((remaining = load_ex(cumulative, acquire)) < TOTAL_BYTES) */
	/* { */
	/* 	uint8_t byte = 0xff; */
	/* 	int32_t n = remaining > 64 ? rand() & 0x3f : rand() % remaining; */
	/* 	const int32_t res = atomic_ring_write_n_bytes(ring, &byte, remaining, 1); */
	/* 	fetch_add_ex(cumulative, res, release); */
	/* } */
}

static void read_job(struct san_struct* args)
{
	struct atomic_ring* ring = args->ring;
	_Atomic(int64_t)* cumulative = args->cum;
	/* while (load_ex(cumulative, acquire) < TOTAL_BYTES) */
	/* { */
	/* 	uint8_t dest[64]; */
	/* 	int32_t size = remaining > 64 ? rand() & 0x3f : rand() % remaining; */
	/* 	const int32_t res = atom_ring_read(ring, dest, size); */
	/* 	fetch_add_ex(cumulative, res, release); */
	/* } */
}

int atom_ring_sancheck(void)
{
	struct atom_ring test_ring = {0};
	ring_map((struct ring_buffer*)&test_ring, 4096, "sancheck");
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
	assert(test_ring.direction == BACKWARD);
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
	assert(test_ring.direction == FORWARD);
	printf("DONE\n- veritfying ring empty ");
	fflush(stdout);
	for (int32_t i = 0; i < size; ++i)
	{
		assert(test_ring.addr[i] == 0x00);
	}
	printf("DONE\nwrap test. ");
	// offset a scooch to check if the ring ACTUALL works
	uint32_t temp = -1u;
	atom_ring_write(&test_ring, &temp, sizeof temp);
	atom_ring_read(&test_ring, &temp, sizeof temp);
	fflush(stdout);
	for (int32_t lap = 0; lap < 128; ++lap)
	{
		/* printf(" + lap %2d\n" */
		/* 	   "\treadable: %d, writeable: %d\n" */
		/* 	   "\thead:%d, tail: %d, " */
		/* 	   "reading_head:%d, writing_tail: %d\n" */
		/* 	   "\tdirection: %s\n", */
		/* 	   lap, readable_size(&test_ring), writeable_size(&test_ring), */
		/* 	   test_ring.head, test_ring.tail, */
		/* 	   test_ring.reading_head, test_ring.writing_tail, */
		/* 	   test_ring.direction == FORWARD ? "FORWARD" : "BACKWARDS"); */
		/* printf(" + lap %2d\n", lap); */
		fflush(stdout);
		for (int32_t i = 0; i < size; ++i)
		{
			uint64_t double_word = -1u;
			/* assertf(readable_size(&test_ring) == 0, "readable_size: %d", readable_size(&test_ring)); */
			/* assertf(writeable_size(&test_ring) == size, "writeable size: %d", writeable_size(&test_ring)); */
			assert_write(&test_ring, &double_word, 1, size);
			/* if (lap >= 15) printf("writing\n"); */
			/* assertf(writeable_size(&test_ring) == size - 1, "writeable size: %d", writeable_size(&test_ring)); */
			/* assertf(readable_size(&test_ring) == 1, "readable_size: %d", readable_size(&test_ring)); */
			/* if (lap >= 15) printf("reading\n"); */
			assert_read(&test_ring, &double_word, 1, 1);
			assert(double_word == -1u);
			/* if (lap >= 15) */
			/* { */
			/* 	printf("\b\b\b\b"); */
			/* } */
		}
	}
	printf("DONE\n");
	fflush(stdout);
	// TODO: spin up some mfn THREADS
	ring_unmap((struct ring_buffer*)&test_ring);
	#define NUM_WRITERS 3
	#define NUM_READERS 5
	pthread_t writers[NUM_WRITERS] = {0};
	pthread_t readers[NUM_READERS] = {0};
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	int64_t bytes_written = 0;
	int64_t bytes_read = 0;
	for (int32_t i = 0; i < NUM_READERS; ++i)
	{
		pthread_create(readers + i, &attr, read_job, &bytes_read);
	}
	for (int32_t i = 0; i < NUM_WRITERS; ++i)
	{
		pthread_create(writers + i, &attr, write_job, &bytes_written);
	}
	for (int32_t i = 0; i < NUM_WRITERS; ++i)
	{
		pthread_join(writers[i], NULL);
	}
	assert(bytes_written == TOTAL_BYTES);
	for (int32_t i = 0; i < NUM_READERS; ++i)
	{
		pthread_join(readers[i], NULL);
	}
	assert(bytes_read == TOTAL_BYTES);
	return 0;
}
