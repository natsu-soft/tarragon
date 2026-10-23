#include "stdout_ring.h"
#include <string.h>
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include "memery.h"
#include "sync_addr.h"
#include "atomics.h"

// NOTE: alignment MATTERS
static struct
{
	uint8_t* addr;
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
	// double-word
	_Atomic(int32_t) writing_tail;
} o_ring = {0};

#define STDOUT_RING_SIZE 32768

enum
{
	FREE		= 0x00,
	LOCKED		= 0x01,
	WAITING		= 0x02,
};

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

static inline const int32_t writeable_size(void)
{
	const int32_t head = load_ex(&o_ring.head, acquire);
	const int32_t tail = load_ex(&o_ring.writing_tail, acquire);
	/* const int32_t dir = load_ex(&o_ring->direction, acquire); */
	/* return head <= size ? size - (tail - head) : head - tail; */
	return  tail >= head ? STDOUT_RING_SIZE - (tail - head) - 1 : head - tail - 1;
}

static inline const int32_t readable_size(void)
{
	const int32_t head = load_ex(&o_ring.head, acquire);
	const int32_t tail = load_ex(&o_ring.tail, acquire);
	return tail >= head ? tail - head : tail + STDOUT_RING_SIZE - head;
}

void stdout_ring_init(void)
{
	ring_map((struct ring_buffer*)&o_ring, STDOUT_RING_SIZE, "stdout_ring");
	assert(o_ring.size == STDOUT_RING_SIZE);
}

void stdout_ring_free(void)
{
	ring_unmap((struct ring_buffer*)&o_ring);
}

int32_t stdout_ring_printf(char* format, ...)
{
	va_list va_args;
	va_start(va_args, format);
	const int32_t size = snprintf(NULL, 0, format, va_args);
	assert(0 < size);
	if (size < STDOUT_RING_SIZE)
	{
		_Atomic(int32_t)* const write_state = &o_ring.write_state;
		int32_t state = load_ex(write_state, acquire);
		for (;;)
		{
			state &= ~LOCKED;
			if (comp_swap_ex(write_state, &state, LOCKED | state, acq_rel, acquire))
			{
				while (writeable_size() < size)
				{
					state = load_ex(&o_ring.read_state, acquire) & ~LOCKED;
					if (comp_swap_ex(&o_ring.read_state, &state, LOCKED | state, acq_rel, acquire))
					{
						const int32_t head = load_ex(&o_ring.head, acquire);
						const int32_t head1 = head + size & STDOUT_RING_SIZE - 1;
						assert(fwrite(o_ring.addr + head, 1, size, stdout) == size);
						store_ex(&o_ring.head, head1, release);
						unlock_and_wake(&o_ring.read_state);
						break;
					}
					int32_t wait_state = WAITING | state;
					if (comp_swap_ex(&o_ring.read_state, &state, wait_state, acq_rel, acquire))
					{
						sync_addr_wait(&o_ring.read_state, &wait_state);
					}
				}
				const int32_t tail = load_ex(&o_ring.writing_tail, acquire);
				const int32_t tail1 = tail + size & STDOUT_RING_SIZE - 1;
				store_ex(&o_ring.writing_tail, tail1, release);
				unlock_and_wake(write_state);
				vsprintf((char*)(o_ring.addr + tail), format, va_args);
				while (load_ex(&o_ring.tail, acquire) != tail)
				{ ; }
				store_ex(&o_ring.tail, tail1, release);
				break;
			}
			int32_t wait_state = WAITING | state;
			if (comp_swap_ex(write_state, &state, wait_state, acq_rel, acquire))
			{
				sync_addr_wait(write_state, &wait_state);
			}
			state = load_ex(write_state, acquire);
		}
	}
	else
	{
		// NOTE: printf is thread-safe, so no need to do any lock tomfoolery here
		vprintf(format, va_args);
	}
	va_end(va_args);
	return size;
}

int32_t stdout_ring_puts(const string* str)
{
	const int32_t size = str->length;
	assert(0 < size);
	if (size < STDOUT_RING_SIZE)
	{
		_Atomic(int32_t)* const write_state = &o_ring.write_state;
		int32_t state = load_ex(write_state, acquire);
		for (;;)
		{
			state &= ~LOCKED;
			if (comp_swap_ex(write_state, &state, LOCKED | state, acq_rel, acquire))
			{
				while (writeable_size() < size)
				{
					state = load_ex(&o_ring.read_state, acquire) & ~LOCKED;
					if (comp_swap_ex(&o_ring.read_state, &state, LOCKED | state, acq_rel, acquire))
					{
						const int32_t head = load_ex(&o_ring.head, acquire);
						const int32_t head1 = head + size & STDOUT_RING_SIZE - 1;
						assert(fwrite(o_ring.addr + head, 1, size, stdout) == size);
						store_ex(&o_ring.head, head1, release);
						unlock_and_wake(&o_ring.read_state);
						break;
					}
					int32_t wait_state = WAITING | state;
					if (comp_swap_ex(&o_ring.read_state, &state, wait_state, acq_rel, acquire))
					{
						sync_addr_wait(&o_ring.read_state, &wait_state);
					}
				}
				const int32_t tail = load_ex(&o_ring.writing_tail, acquire);
				const int32_t tail1 = tail + size & STDOUT_RING_SIZE - 1;
				store_ex(&o_ring.writing_tail, tail1, release);
				unlock_and_wake(write_state);
				memcpy(o_ring.addr + tail, str->chars, size);
				while (load_ex(&o_ring.tail, acquire) != tail)
				{ ; }
				store_ex(&o_ring.tail, tail1, release);
				break;
			}
			int32_t wait_state = WAITING | state;
			if (comp_swap_ex(write_state, &state, wait_state, acq_rel, acquire))
			{
				sync_addr_wait(write_state, &wait_state);
			}
			state = load_ex(write_state, acquire);
		}
	}
	else
	{
		printf("%.*s", size, str->chars);
	}
	return size;
}

int32_t stdout_ring_flush(void)
{
	_Atomic(int32_t)* const read_state = &o_ring.read_state;
	int32_t state = load_ex(read_state, acquire);
	for(;;)
	{
		state &= ~LOCKED;
		if (comp_swap_ex(read_state, &state, LOCKED | state, acq_rel, acquire))
		{
			const int32_t size = readable_size();
			if (size > 0)
			{
				const int32_t head = load_ex(&o_ring.head, acquire);
				const int32_t head1 = head + size & STDOUT_RING_SIZE - 1;
				assert(fwrite(o_ring.addr + head, 1, size, stdout) == size);
				store_ex(&o_ring.head, head1, release);
			}
			unlock_and_wake(read_state);
			return size;
		}
		int32_t wait_state = WAITING | state;
		if (comp_swap_ex(read_state, &state, wait_state, acq_rel, acquire))
		{
			sync_addr_wait(read_state, &wait_state);
		}
		state = load_ex(read_state, acquire);
	}
}

