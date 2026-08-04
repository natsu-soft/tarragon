#ifndef MACRO_RING_H
#define MACRO_RING_H

#include <assert.h>

#define RING(NAME, TYPE)												\
	struct NAME##_ring													\
	{																	\
		TYPE *buffer;													\
		int cap;														\
		int size;														\
		int head;														\
		int tail;														\
	};																	\
																		\
	static int NAME##_ring_push(struct NAME##_ring* ring, const TYPE value)	\
	{																	\
		assert(ring->size < ring->cap && #NAME " ring overflow!");		\
		ring->buffer[ring->tail] = value;								\
		ring->tail = (ring->tail + 1) % ring->cap;						\
		return ++(ring->size);											\
	}																	\
																		\
	static TYPE NAME##_ring_pop(struct NAME##_ring* ring)				\
	{																	\
		assert(ring->size > 0 && #NAME " ring underflow");				\
		TYPE res = ring->buffer[ring->head];							\
		ring->head = (ring->head + 1) % ring->cap;						\
		--(ring->size);													\
		return res;														\
	}																	\
																		\
	static TYPE NAME##_ring_peek(const struct NAME##_ring* ring)		\
	{																	\
		return ring->buffer[ring->head];								\
	}																	\
																		\
	static void reset(struct NAME##_ring* ring)							\
	{																	\
		ring->size = 0;													\
		ring->head = 0;													\
		ring->tail = 0;													\
	}




#endif /* End MACRO_RING_H */
