#include "map16.h"
/* #include <stdlib.h> */
#include <stdint.h>
#include <string.h>
#include <assert.h>


//-----------------------------------------------------------------------------
// MurmurHash3 was written by Austin Appleby, and is placed in the public
// domain. The author hereby disclaims copyright to this source code.

// Note - The x86 and x64 versions do _not_ produce the same results, as the
// algorithms are optimized for their respective platforms. You can still
// compile and run any of them on any platform, but your performance with the
// non-native version will be less than optimal.

#ifdef __GNUC__
#define FORCE_INLINE __attribute__((always_inline)) inline
#else
#define FORCE_INLINE inline
#endif

static FORCE_INLINE uint64_t rotl64 ( uint64_t x, int8_t r )
{
  return (x << r) | (x >> (64 - r));
}

#define BIG_CONSTANT(x) (x##LLU)
#define GETBLOCK(p, i) (p[i])
#define ROTL64(x,y)	rotl64(x,y)

static FORCE_INLINE uint64_t fmix64 (uint64_t k)
{
  k ^= k >> 33;
  k *= BIG_CONSTANT(0xff51afd7ed558ccd);
  k ^= k >> 33;
  k *= BIG_CONSTANT(0xc4ceb9fe1a85ec53);
  k ^= k >> 33;
  return k;
}

// this was originally the 128-bit x64 version (x64_128)
static uint32_t murmur3 (const void* key, const int len, const uint32_t seed)
{
	const uint8_t* data = (const uint8_t*)key;
	const int nblocks = len / 16;
	int i;
	uint64_t h1 = seed;
	uint64_t h2 = seed;
	uint64_t c1 = BIG_CONSTANT(0x87c37b91114253d5);
	uint64_t c2 = BIG_CONSTANT(0x4cf5ad432745937f);
	//----------
	// body
	const uint64_t * blocks = (const uint64_t *)(data);
	for(i = 0; i < nblocks; i++)
	{
		uint64_t k1 = GETBLOCK(blocks,i*2+0);
		uint64_t k2 = GETBLOCK(blocks,i*2+1);
		k1 *= c1; k1  = ROTL64(k1,31); k1 *= c2; h1 ^= k1;
		h1 = ROTL64(h1,27); h1 += h2; h1 = h1*5+0x52dce729;
		k2 *= c2; k2  = ROTL64(k2,33); k2 *= c1; h2 ^= k2;
		h2 = ROTL64(h2,31); h2 += h1; h2 = h2*5+0x38495ab5;
	}
	//----------
	// tail
	const uint8_t * tail = (const uint8_t*)(data + nblocks*16);
	uint64_t k1 = 0;
	uint64_t k2 = 0;
	switch(len & 15)
	{
	case 15:
		k2 ^= (uint64_t)(tail[14]) << 48;
	case 14:
		k2 ^= (uint64_t)(tail[13]) << 40;
	case 13:
		k2 ^= (uint64_t)(tail[12]) << 32;
	case 12:
		k2 ^= (uint64_t)(tail[11]) << 24;
	case 11:
		k2 ^= (uint64_t)(tail[10]) << 16;
	case 10:
		k2 ^= (uint64_t)(tail[ 9]) << 8;
	case  9:
		k2 ^= (uint64_t)(tail[ 8]) << 0;
		k2 *= c2; k2  = ROTL64(k2,33); k2 *= c1; h2 ^= k2;
	case  8:
		k1 ^= (uint64_t)(tail[ 7]) << 56;
	case  7:
		k1 ^= (uint64_t)(tail[ 6]) << 48;
	case  6:
		k1 ^= (uint64_t)(tail[ 5]) << 40;
	case  5:
		k1 ^= (uint64_t)(tail[ 4]) << 32;
	case  4:
		k1 ^= (uint64_t)(tail[ 3]) << 24;
	case  3:
		k1 ^= (uint64_t)(tail[ 2]) << 16;
	case  2:
		k1 ^= (uint64_t)(tail[ 1]) << 8;
	case  1:
		k1 ^= (uint64_t)(tail[ 0]) << 0;
		k1 *= c1; k1  = ROTL64(k1,31); k1 *= c2; h1 ^= k1;
	};
	//----------
	// finalization
	h1 ^= len;
	h2 ^= len;
	h1 += h2;
	h2 += h1;
	h1 = fmix64(h1);
	h2 = fmix64(h2);
	h1 += h2;
	h2 += h1;
	return h1 & UINT32_MAX;
}

void map16_init(map16_t* map, const int32_t pow2cap, uint8_t* allocation)
{
	map->keys = allocation;
	map->values = map->keys + pow2cap;
	map->lens = map->values + pow2cap;
	map->cap = pow2cap;
	map->size = 0;
	memset(map->lens, 0x00, pow2cap * sizeof *map->lens);
	// MAYBE values too
}

void map16_cache(map16_t* map, const char* key, const int16_t len, const int32_t value)
{
	const int32_t cap = map->cap;
	assert(map->size <= cap);
	const uint32_t cap_mask = cap - 1;
	const int32_t index = murmur3(key, len, (uint64_t)map & UINT32_MAX) & cap_mask;
	int16_t* lens = map->lens;
	for(int32_t i = 0; i < cap; ++i)
	{
		const int32_t j = (index + i) & cap_mask;
		if (lens[j] == 0)
		{
			map->keys[j] = key;
			map->lens[j] = len;
			map->values[j] = value;
			map->size++;
			return;
		}
	}
}

int16_t map16_fetch(map16_t* map, const char* key, const int16_t len)
{
	const int32_t cap = map->cap;
	const uint32_t cap_mask = cap - 1;
	const int32_t index = murmur3(key, len, (uint64_t)map & UINT32_MAX) & cap_mask;
	int16_t* lens = map->lens;
	char** keys = map->keys;
	for (int32_t i = 0; i < cap; ++i)
	{
		const int32_t j = (index + i) & cap_mask;
		const int32_t l = lens[j];
		if (len == l && strncmp(key, keys[j], len) == 0)
		{
			return map->values[j];
		}
		else if (l == 0)
		{
			return -1;
		}
	}
	return -1;
}

#include <stdio.h>

void map16_print(const map16_t* map)
{
	const int32_t cap = map->cap;
	int16_t* lens = map->lens;
	char** keys = map->keys;
	for (int32_t i = 0; i < cap; ++i)
	{
		const int32_t len = lens[i];
		if (len > 0)
		{
			printf("%d: ", i);
			char* key = keys[i];
			for (int32_t j = 0; j < len; ++j)
			{
				printf("%c", key[i]);
			}
			printf(" -> %d\n", map->values[i]);
		}
	}
}
