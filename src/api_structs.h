// NOTE: USED!!!
#ifndef API_STRUCTS_H
#define API_STRUCTS_H

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "relative_ptrs.h"
#include "hashmaps/api_map.h"

/* NOTE: ON OPERATOR CACHING
 * operators are cached as the following string:
 * operator + diff_ptr_t of the lhs + diff_ptr_t of the rhs (for binary operators)
 * the cache fetch returns the tagged ptr of the type
 *
 * NOTE: relative pointers to types must ALWAYS be relatively offset
 * this lets you discriminate between enums & classes
 *
 * NOTE: REJOICE! FUNCTIONS CAN NOT BE OVERLOADED IN GDSCRIPT!
 */

// NOTE: go ahead & null terminate descriptions.


#define API_TAGS									\
	X(CLASS, class, classes)						\
	X(FUNC, func, funcs)							\
	X(PARAM, param, params)							\
	X(ENUM_VALUE, enum_value, enum_values)			\
	X(CONSTANT, constant, constants)				\
	X(ENUM, enumerator, enumerators)				\
	X(SIGNAL, signal, signals)						\
	X(NATIVE_STRUCT, native_struct, native_structs) \
	X(MEMBER, member, members)

enum __attribute__((packed)) api_tag
{
	API_NONE = 0x00,
#define X(TAG, ...) API_##TAG,
API_TAGS
#undef X
	API_VOID_RETURN
};

enum __attribute__((packed)) api_metadata
{
	GODOT_API	= 0x10,
	METAFLAG_CAP = 0xf0,
};

#define X(_, NAME, ...) typedef struct NAME##_t NAME##_t;
API_TAGS
#undef X

/* // NOTE: if tag is none ptr should be null */
/* typedef struct api_ptr_t */
/* { */
/* 	union */
/* 	{ */
/* 		void* ptr; // for lazy access */
/* #define X(_, NAME, __) struct NAME##_t* NAME; */
/* API_TAGS */
/* #undef X */
/* 	}; */
/* 	enum api_tag tag; */
/* } api_ptr_t; */

// NOTE: this is cool and I _WILL_ use it >:^)
// usually 48-bits are _actually_ used for pointers, machines with > 256TB of memory use 57 bits
typedef union api_ptr_t
{
	struct
	{
		// see: https://lwn.net/Articles/717293/
		unsigned long dont_touch : 32;
		unsigned int probably_dont_touch: 25;
		enum api_tag tag : 4;
		enum api_metadata metadata: 3;
	};
	uint64_t bits;
	int64_t number;
	void* ptr;
	#define X(_, NAME, __) struct NAME##_t* NAME;
	API_TAGS
	#undef X
} api_ptr_t;

api_ptr_t tag_api_ptr(void* ptr, enum api_tag tag);
api_ptr_t flag_api_ptr(void* ptr,  uint8_t bitflags);
enum api_tag api_tag(const api_ptr_t api_ptr);
enum api_metadata api_metadata(const api_ptr_t api_ptr);
void* api_ptr(const api_ptr_t api_ptr);
#ifndef TAG_PTR
#define TAG_PTR(PTR, TAG) (void*)((size_t)PTR | ((size_t)TAG << 48))
#endif
#ifndef UNTAG
#define UNTAG(PTR) (void*)((~0x7fff000000000000) & PTR)
#endif
#ifndef API_ERR
#define API_ERR(PTR) (0x4000000000000000 & PTR)
#endif

enum gdscript_class_flags
{
	CLASS_NO_FLAGS = 0x00,
	CLASS_IS_KEYED = 0x01,
	CLASS_IS_INSTANTIABLE = 0x02,
	CLASS_HAS_DESTRUCTOR = 0x04,
	CLASS_IS_SINGLETON = 0x08
};

enum gdscript_func_flags
{
	FUNC_NO_FLAGS = 0x00,
	FUNC_CONST = 0x01,
	FUNC_VARIADIC = 0x02,
	FUNC_STATIC = 0x04,
	FUNC_VIRTUAL = 0x08,
};

typedef struct enumerator_t
{
	enum_value_t* values;
	int32_t num_values;
	uint8_t is_bitfield;
} enum_t;

typedef struct enum_value_t
{
	string name;
	int32_t value;
} enum_value_t;

typedef struct func_t
{
	string name; // NOTE: I shouldn't need this
	param_t* params;
	api_ptr_t return_type;
	int32_t num_params;
	uint8_t bitflags;
} func_t;

typedef struct param_t
{
	string name;
	string default_value;
	api_ptr_t type;
} param_t;

typedef struct class_t
{
	api_map_t class_map; // hello, pointer-based polymorphism
	func_t* constructors;
	class_t* inherits;
	// NOTE: you don't NEED this but it just makes debugging easier
	string name;
	int16_t num_constructors;
	uint8_t bitflags;
} class_t;

// MORE ptr based polymorphism
typedef struct member_t
{
	api_ptr_t type;
} member_t;

// oh fyggggg
typedef struct constant_t
{
	api_ptr_t type;
	string value;
} constant_t;

typedef struct signal_t
{
	param_t* params;
	int32_t num_params;
} signal_t;

// wtf I actually need this shit?
typedef struct native_struct_t
{
	string format;
} native_struct_t;

#endif /* End API_STRUCTS_H */
