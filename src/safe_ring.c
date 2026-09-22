#include "safe_ring.h"
#include <stdint.h>
#include "atomics.h"
#include <string.h>
#include "threading.h"


static struct futex write_ftx = {0};
static struct futex read_ftx = {0};
static struct conditional write_tx = {0};
static struct conditional read_tx = {0};

static inline const int32_t readable_size(struct safe_ring* ring)
{
	const int32_t size = ring->size;
	const int32_t head = load_ex(&ring->head, acquire);
	const int32_t tail = load_ex(&ring->tail, acquire);
	return head >= tail ? head - tail : head + size - tail;
}

static inline const int32_t writeable_size(struct safe_ring* ring)
{
	const int32_t size = ring->size;
	const int32_t head = load_ex(&ring->head, acquire);
	const int32_t tail = load_ex(&ring->tail, acquire);
	return head >= tail ? tail + size - head : tail - head;
}

int32_t safe_ring_write(struct safe_ring* ring, void* src, const int32_t size)
{
	futex_lock(&write_ftx);
	while (writeable_size(ring) < size)
	{
		cond_wait(&read_tx, &write_ftx);
	}
	int32_t head0 = load_ex(&ring->head, acquire);
	const int32_t head = swap_ex(&ring->head, (head0 + size) % ring->size, acq_rel);
	thread_fence(acquire);
	for (int32_t i = 0; i < size; ++i)
	{
		store_ex(ring->addr + head + i, ((uint8_t*)src)[i], release);
	}
	thread_fence(release);
	futex_unlock(&write_ftx);
	cond_broadcast(&write_tx);
	return size;

}

int32_t safe_ring_read(struct safe_ring* ring, void* dest, const int32_t size)
{
	futex_lock(&read_ftx);
	while (readable_size(ring) < size)
	{
		cond_wait(&write_tx, &read_ftx);
	}
	int32_t tail0 = load_ex(&ring->tail, acquire);
	const int32_t tail = swap_ex(&ring->tail, (tail0 + size) % ring->size, acq_rel);
	thread_fence(acquire);
	for (int32_t i = 0; i < size; ++i)
	{
		((uint8_t*)dest)[i] = load_ex(ring->addr + tail + i, acquire);
	}
	thread_fence(release);
	futex_unlock(&read_ftx);
	cond_broadcast(&read_tx);
	return size;
}

int32_t safe_ring_n_write(struct safe_ring* ring, void* src, const int32_t n, const int32_t elem_size)
{
	const int32_t size = n * elem_size;
	futex_lock(&write_ftx);
	while (writeable_size(ring) < size)
	{
		cond_wait(&read_tx, &write_ftx);
	}
	int32_t head0 = load_ex(&ring->head, acquire);
	const int32_t head = swap_ex(&ring->head, (head0 + size) % ring->size, acq_rel);
	thread_fence(acquire);
	for (int32_t i = 0; i < n; ++i)
	{
		for (int32_t j = 0; j < elem_size; ++j)
		{
			store_ex(ring->addr + i * elem_size + j, ((uint8_t*)src)[j], release);
		}
		/* memcpy(ring->addr + head + i * elem_size, src, elem_size); */
	}
	thread_fence(release);
	futex_unlock(&write_ftx);
	cond_broadcast(&write_tx);
	return size;

}
