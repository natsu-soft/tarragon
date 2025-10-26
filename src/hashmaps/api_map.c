#include "api_map.h"
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "../allocators/arena.h"

#define HASH_SEED 0xa85b0010


#define PASSERT(A, MSG, ...) if (!(A)) { printf(MSG, ##__VA_ARGS__); assert(A); }

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
static uint64_t murmur3 (const void* key, const int len, const uint32_t seed)
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
	return h1;
}

static inline int32_t fib_hash(const size_t hash, const int32_t cap)
{
	int32_t n = 0;
	for (int32_t i = 0; i <= 64 && 1u << n < cap; ++i);
	return ((hash * 11400714819323198485llu) >> (64 - n)) & INT32_MAX;
}

// it's lite, it's fast; it'll do.
// and if it dont, just use murmur3
// OK THIS IS INSUFFICIENT
static inline int32_t djb2(const char* str, const int16_t len)
/* static inline int32_t djb2(const char* str, const int16_t len, const int32_t cap) */
{
	/* int64_t hash = 5381; */
	size_t hash = 5381;
	for (int16_t i = 0; i < len; ++i)
	{
		hash += ((hash << 5) + hash) ^ str[i];
	}
	return hash & INT32_MAX;
	/* int32_t n = 0; */
	/* for (int32_t i = 0; i < 64 && (1u << n) < cap; ++i); */
	/* return (hash * 11400714819323198485llu) >> (64 - n) & INT32_MAX; */
}


void api_map_back(api_map_t* map, arena_t* backing_arena, const int32_t cap)
{
	assert(cap && (cap & (cap - 1)) == 0);
	map->cap = cap;
	const self_ptr_t* keys = arena_alloc(backing_arena, cap * sizeof *keys);
	const diff_ptr_t* values = arena_alloc(backing_arena, cap * sizeof *values);
	const int16_t* lens = arena_calloc(backing_arena, cap * sizeof *lens);
	self_relate(&map->keys_selfptrs, keys);
	self_relate(&map->values_diffptrs, values);
	self_relate(&map->lens_i16, lens);
	map->size = 0;
}

diff_ptr_t* api_map_cache(api_map_t* map, const char* key, const int32_t len,  const diff_ptr_t diff_ptr)
{
	const int32_t cap = map->cap;
	assert(map->size <= cap && "index map out of space.");
	const uint32_t cap_mask = cap - 1;
	/* const int32_t index = murmur3(key, len, (size_t)map) & cap_mask; */
	const int32_t index = murmur3(key, len, HASH_SEED) & cap_mask;
	const self_ptr_t* keys = abs_self(&map->keys_selfptrs);
	/* const int32_t index = djb2(key, len); */
	int16_t* lens = abs_self(&map->lens_i16);
	for (int32_t i = 0; i < cap; ++i)
	{
		const int32_t j = (index + i) & cap_mask;
		if (lens[j] == 0)
		{
			self_relate((self_ptr_t*)abs_self(&map->keys_selfptrs) + j, key);
			/* self_relate((self_t*)abs_self(&map->values_diffptrs) + j, value); */
			diff_ptr_t* diff_ptrs = abs_self(&map->values_diffptrs);
			memcpy(diff_ptrs + j, &diff_ptr, sizeof diff_ptr);
			lens[j] = len;
			map->size++;
			/* char* temp = alloca(len + 1); */
			/* memcpy(temp, key, len); */
			/* temp[len] = '\0'; */
			/* PASSERT(offset == api_map_fetch(map, key, len).delta, "failed in-place audit of '%s'\n", temp); */
			/* printf("cached:%d: '%s': %d\n", j, temp, offset); */
			return diff_ptrs + j;
		}
/* 		else if (lens[j] == len && strncmp(key, abs_self(keys + j), len) == 0) */
/* 		{ */
/* 			diff_ptr_t* offsets = abs_self(&map->values_diffptrs); */
/* 			char* temp = alloca(len + 1); */
/* 			memcpy(temp, key, len); */
/* 			temp[len] = '\0'; */
/* 			fprintf(stderr, "\nKEY: '%s'", temp); */
/* 			fprintf(stderr, "\nORIGINAL -> %#x(%d)", offsets[j].delta, offsets[j].delta & 0x7); */
/* 			fprintf(stderr, "\nNEW      -> %#x(%d)\n", offset, offset & 0x7); */
/* 			if (key[0] == ' ') */
/* 			{ */
/* 				fprintf(stderr, "key hex:\n\t"); */
/* 				for (int32_t l = 0; l < len; ++l) */
/* 				{ */
/* 					fprintf(stderr, " %#x", *(uint8_t*)(key + l)); */
/* 				} */
/* 				fprintf(stderr, "\noperator code:\n"); */
/* 				switch ((gdscript_id_t)key[1]) */
/* 				{ */
/* #define X(ID, ...) case GDSCRIPT_##ID: fprintf(stderr, "\t" #ID); break; */
/* 					GDSCRIPT_BUILTIN_TOKENS */
/* #undef X */
/* 				} */
/* 				const int32_t lhs = *(int32_t*)(key + 2); */
/* 				fprintf(stderr, " %#x(%d)", lhs, lhs & 0x7); */
/* 				if (len == 10) */
/* 				{ */
/* 					const int32_t rhs = *(int32_t*)(key + 6); */
/* 					fprintf(stderr, " %#x(%d)", rhs, rhs & 0x7); */
/* 				} */
/* 			} */
/* 			fprintf(stderr, "\n"); */
/* 			fflush(stderr); */
/* 			assert(0); */
/* 		} */
	}
	return NULL;
}

diff_ptr_t api_map_fetch(const api_map_t* map, const char* key, const int32_t len)
{
	const int32_t cap = map->cap;
	const uint32_t cap_mask = cap - 1;
	const int32_t index = murmur3(key, len, HASH_SEED) & cap_mask;
	/* const int32_t index = djb2(key, len); */
	int16_t* lens = abs_self(&map->lens_i16);
	self_ptr_t* keys = abs_self(&map->keys_selfptrs);
	for (int32_t i = 0; i < cap; ++i)
	{
		const int32_t j = (index + i) & cap_mask;
		const int16_t l = lens[j];
		/* if (l == len && strncmp(key, abs_self(keys + j), len) == 0) */
		/* { */
		if (l == len && strncmp(key, abs_self(keys + j), len) == 0)
		{
			diff_ptr_t* values = abs_self(&map->values_diffptrs);
			return values[j];
		}
		else if (l == 0)
		{
			break;
		}
	}
	/* char* temp = alloca(len + 1); */
	/* memcpy(temp, key, len); */
	/* temp[len] = '\0'; */
	/* printf("  failed to find a value for '%s'!\n", temp); */
	return (diff_ptr_t){ 0 };
}

const uint64_t api_map_hash(const uint8_t* str, const int32_t len)
{
	return murmur3(str, len, HASH_SEED);
}


void api_map_audit(const api_map_t* map)
{
	const int32_t cap = map->cap;
	self_ptr_t* keys = abs_self(&map->keys_selfptrs);
	diff_ptr_t* values = abs_self(&map->values_diffptrs);
	int16_t* lens = abs_self(&map->lens_i16);
	for (int32_t i = 0; i < cap; ++i)
	{
		const int16_t len = lens[i];
		if (len > 0)
		{

			char* temp = alloca(len + 1);
			memcpy(temp, abs_self(keys + i), len);
			temp[len] = '\0';
			const diff_ptr_t expected = values[i];
			PASSERT(expected.delta != 0, "%d: '%s' should have a non-zero entry!\n", i, temp);
			const diff_ptr_t actual = api_map_fetch(map, temp, len);
			PASSERT(expected.delta == actual.delta, "%d: self-audit of '%s' (len: %d) failed! Expected: %#x(%d), fetched: %#x!\n", i, temp, len, expected.delta, expected.delta & 0x7, actual.delta);
			/* printf("%d: '%s'(%d chars) %d -> %d\n", i, temp, len, expected.delta, actual.delta); */
		}
		/* else */
		/* { */
		/* 	printf("EMPTY: %d\n", i); */
		/* } */
	}
}
