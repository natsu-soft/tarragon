#include "api_map.h"
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "../allocators/arena.h"
#include "../api_structs.h"
#include "../strings.h"
#include "../gdscript/token_ids.h"
IMPL_UNARY_KEY
IMPL_BINARY_KEY

uint8_t api_map_cache_quiet = 1;

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

static FORCE_INLINE uint64_t rotl64 (uint64_t x, int8_t r )
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
	api_ptr_t* values = arena_calloc(backing_arena, cap * sizeof *values);
	char** keys = arena_alloc(backing_arena, cap * sizeof *keys);
	int32_t* lens = arena_calloc(backing_arena, cap * sizeof *lens);
	map->values = values;
	map->keys = keys;
	map->lengths = lens;
	map->size = 0;
	map->cap = cap;
}

api_ptr_t api_map_cache(api_map_t* map, const string* str, const api_ptr_t ptr)
{
	const int32_t cap = map->cap;
	assert(map->size <= cap && "index map out of space.");
	const uint32_t cap_mask = cap - 1;
	/* const int32_t index = murmur3(key, len, (size_t)map) & cap_mask; */
	const int32_t index = murmur3(str->chars, str->length, HASH_SEED) & cap_mask;
	/* const self_ptr_t* keys = abs_self(&map->keys_selfptrs); */
	/* const int32_t index = djb2(key, len); */
	int32_t* lens = map->lengths;
	for (int32_t i = 0; i < cap; ++i)
	{
		const int32_t j = (index + i) & cap_mask;
		if (lens[j] == 0)
		{
			map->keys[j] = str->chars;
			lens[j] = str->length;
			map->values[j] = ptr;
			/* printf("cached: %d@'%.*s' -> %#018lx\n", j, str->length, str->value, ptr); */
			map->size++;
			return ptr;
		}
	}
	return (api_ptr_t){ 0x00 };
}

api_ptr_t api_map_fetch(const api_map_t* map, const string* str)
{
	const int32_t cap = map->cap;
	const uint32_t cap_mask = cap - 1;
	const char* key = str->chars;
	const int32_t len = str->length;
	const int32_t index = murmur3(key, len, HASH_SEED) & cap_mask;
	int32_t* lens = map->lengths;
	char** keys = map->keys;
	for (int32_t i = 0; i < cap; ++i)
	{
		const int32_t j = (index + i) & cap_mask;
		const int16_t l = lens[j];
		/* if (l == len && strncmp(key, abs_self(keys + j), len) == 0) */
		/* { */
		if (l == len && strncmp(key, keys[j], len) == 0)
		{
			return map->values[j];
		}
		else if (l == 0)
		{
			break;
		}
	}
	return (api_ptr_t){ 0 };
}

const uint64_t api_map_hash(const void* str, const int32_t len)
{
	return murmur3(str, len, HASH_SEED);
}


// just print out the keys & addresses, bro
void api_map_audit(const api_map_t* map)
{
	for (int i = 0; i < map->cap; ++i)
	{
		const int len = map->lengths[i];
		if (len > 0)
		{
			char* key = map->keys[i];
			const api_ptr_t value = map->values[i];
			enum api_tag payload_tag = api_tag(map->values[i]);
			string str;
			if (key[0] != ' ')
			{
				switch (payload_tag)
				{
#define X(A,...) \
					case API_##A: \
						printf("%d: '%.*s' -> " #A "@%016lx\n", i, len, key, api_ptr(value)); \
					break;
					API_TAGS
#undef X
				}
			}
			else
			{
				api_ptr_t operands[2] = { 0x00 };
				int num_operands = 1;
				token_id_t operator;
				uint8_t foo = key[1];
				if (len == 18)
				{
					union binary_key binary_key;
					memcpy(&binary_key, key, sizeof binary_key);
					operator = binary_key.operator;
					++num_operands;
					assert(memcmp(&binary_key, key, len) == 0);
					operands[0] = binary_key.lhs;
					operands[1] = binary_key.rhs;
				}
				else if (len == 10)
				{
					union unary_key unary_key;
					memcpy(&unary_key, key, sizeof unary_key);
					operator = unary_key.operator;
					assert(memcmp(&unary_key, key, len) == 0);
					operands[0] = unary_key.operand;
				}
				else
				{
					assert(0 && "Brother this is NOT an operator key--the length is ALL WRONG!");
				}
				switch (operator) {
#define X(ENUM, STR, LEN) case GDSCRIPT_##ENUM: printf("%d: %.*s", i, LEN, STR); break;
					GDSCRIPT_OPERATORS
#undef X
#define X(ENUM, STR, LEN) case GDSCRIPT_##ENUM:					\
					printf("NON-OPERATOR: '%.*s'\n", LEN, STR);	\
					fflush(stdout);								\
					assert(0 && "UNKNOWN OPERATOR");			\
					break;
						GDSCRIPT_NOT_OPERATORS
#undef X
				default:
					printf(" bad operator: %03d(str: %.*s)\n", operator, len, key);
					fflush(stdout);
					assert(0 && "UNKNOWN OPERATOR");
				}
				for (int j = 0; j < num_operands; ++j)
				{
					/* printf("\toperand[%d]: %016lx\n", j, operands[j]); */
					enum api_tag tag = api_tag(operands[j]);
					class_t* class;
					enumerator_t* enumerator;
					native_struct_t* native_struct;
					switch(tag)
					{
					case API_CLASS:
						class = api_ptr(operands[j]);
						printf(" %.*s", class->name.length, class->name.chars);
						break;
					case API_ENUM:
						enumerator = api_ptr(operands[j]);
						printf(" enum@%016lx", enumerator);
						break;
					case API_NATIVE_STRUCT:
						native_struct = api_ptr(operands[j]);
						printf(" %.*s", native_struct->format.length, native_struct->format.chars);
						break;
					default:
						printf("BAD OPERAND API TYPE: %d\n", tag);
						fflush(stdout);
						assert(0 && "UNRECOGNIZED TAG TYPE");
					}
				}
				switch(payload_tag)
				{
				case API_CLASS:
					str = ((class_t*)api_ptr(map->values[i]))->name;
					printf(" -> %.*s\n", str.length, str.chars, api_ptr(value));
					break;
				case API_ENUM:
				{
					printf(" -> [");
					enumerator_t* e = api_ptr(value);
					for (int j = 0; j < e->num_values; ++j)
					{
						str = e->values[j].name;
						printf(" %.*s", str.length, str.chars);
					}
					printf(" ]\n");
				}
					break;
				case API_NATIVE_STRUCT:
					str = ((native_struct_t*)api_ptr(map->values[i]))->format;
					printf(" -> { %.*s }@%016lx\n", str.length, str.chars, api_ptr(value));
					break;
				default:
					printf(" -> (%d)@%016x\n", payload_tag, api_ptr(value));
				}
			}
		}
	}
}

#warning api_map_ppm() is fucky
void api_map_ppm(FILE* dest, const int cols, const api_map_t* map)
{
	fprintf(dest, "P3\n%d, %d\n1\n", cols, map->cap / cols);
	char** keys = map->keys;
	int32_t* lens = map->lengths;
	const uint32_t cap_mask = map->cap - 1;
	for (int i = 0; i < map->cap; ++i)
	{
		if (lens[i] > 0)
		{
			const int ideal = murmur3(keys[i], lens[i], HASH_SEED) & cap_mask;
			fprintf(dest, "%.*s ", 6, i == ideal ? "1 1 1 " : "1 0 0 ");
		}
		else
		{
			fprintf(dest, "0 0 0 ");
		}
	}
}
