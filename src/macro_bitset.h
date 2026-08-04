#ifndef MACRO_BITSET_H
#define MACRO_BITSET_H

#include <stdint.h>

#define BITSET_SPAN_TYPE(PREFIX, TYPE) BITSET(PREFIX, TYPE, (1 << sizeof(TYPE)) * (1 << sizeof(TYPE)))

// width in nibbles
#define BITSET(PREFIX, TYPE, WIDTH)																		\
	struct PREFIX##_bitset																				\
	{																									\
		uint64_t bits[WIDTH];																			\
	};																									\
	static inline void PREFIX##_bitset_add(struct PREFIX##_bitset* dest, const TYPE value)				\
	{																									\
		dest->bits[value >> 6] |= 1LL << (value & 0x3f);													\
	}																									\
	static inline void PREFIX##_bitset_rm(struct PREFIX##_bitset* dest, const TYPE value)				\
	{																									\
		dest->bits[value >> 6] &= ~(1LL << (value & 0x3f));												\
	}																									\
	static inline uint64_t PREFIX##_bitset_has(struct PREFIX##_bitset* src, const TYPE value)			\
	{																									\
		return src->bits[value >> 6] & (1LL << (value & 0x3f));											\
	}																									\
	static inline void PREFIX##_bitset_union(struct PREFIX##_bitset* dest, struct PREFIX##_bitset* src)	\
	{																									\
		for (int i = 0; i < WIDTH; ++i)																	\
		{																								\
			dest->bits[i] |= src->bits[i];																\
		}																								\
	}

#endif /* End MACRO_BITSET_H */
