#ifndef API_MAP_H
#define API_MAP_H

#include <stdint.h>
#include "../strings.h"

#ifdef _WIN32
typedef struct _iobuf FILE;
typedef unsigned long long size_t;
#else
typedef struct _IO_FILE FILE;
typedef unsigned long size_t;
#endif

#ifndef IMPL_UNARY_KEY
#define IMPL_UNARY_KEY							\
	union unary_key								\
	{											\
		uint8_t bits[10];						\
		struct __attribute__((packed))			\
		{										\
			char header;						\
			token_id_t operator;				\
			api_ptr_t operand;					\
		};										\
	};
#endif/* End IMPL_UNARY_KEY */

#ifndef IMPL_BINARY_KEY
#define IMPL_BINARY_KEY							\
	union binary_key							\
	{											\
		uint8_t bits[18];						\
		struct __attribute__((packed))			\
		{										\
			char header;						\
			token_id_t operator;				\
			api_ptr_t lhs;						\
			api_ptr_t rhs;						\
		};										\
	};
#endif /* End IMPL_BINARY_KEY */

typedef struct api_map_t
{
	union api_ptr_t* values;
	STRING_ARRAY(keys, lengths);
	int32_t cap;
	int32_t size;
} api_map_t;

typedef struct arena_t arena_t;

void api_map_back(api_map_t* map, arena_t* backing_arena, int32_t cap);
union api_ptr_t api_map_cache(api_map_t* map, const string* str,  const union api_ptr_t ptr);
union api_ptr_t api_map_fetch(const api_map_t* map, const string* str);
void api_map_audit(const api_map_t* map);
/* void api_map_dump_keys(string_narray* dest, arena_t* arena, const api_map_t* map); */
void api_map_make_serializable(api_map_t* map);


const uint64_t api_map_hash(const void* str, const int32_t len);
void api_map_ppm(FILE* dest, const int cols, const api_map_t* map);

#endif /* End API_MAP_H */
