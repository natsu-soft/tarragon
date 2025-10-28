#ifndef API_STRUCTS_H
#define API_STRUCTS_H

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "relative_ptrs.h"
#include "hashmaps/api_map.h"

/* NOTE: ALL STRUCTS MUST BE MOD 8 BYTES ALIGNED */


/* NOTE: ON OPERATOR CACHING
 * operators are cached as the following string:
 * operator + diff_ptr_t of the lhs + diff_ptr_t of the rhs (for binary operators)
 * the cache fetch returns the tagged ptr of the type
 *
 * NOTE: relative pointers to types must ALWAYS be relatively offset
 * this lets you discriminate between enums & classes
 *
 * NOTE: FUNCTIONS CANT BE OVERLOADED IN GDSCRIPT
 */

// NOTE: go ahead & null terminate descriptions.




// 0x7, king
typedef enum api_tag_t
{
	API_NONE = 0x00,
	API_MEMBER, // NOTE: atm members are just 4bytes so you HAVE to put them first for the tags to work
	API_ENUM,
	API_FUNC,
	/* API_TYPE, // types have operators */
	API_CLASS, // classes do not!
	API_SIGNAL,
	API_CONSTANT,
	API_NATIVE_STRUCT
} api_tag_t;

typedef struct fetch_res_t
{
	void* ptr;
	diff_ptr_t diff_ptr;
	api_tag_t tag;
} fetch_res_t;

typedef enum class_flags_t
{
	CLASS_NO_FLAGS = 0x00,
	CLASS_IS_KEYED = 0x01,
	CLASS_IS_INSTANTIABLE = 0x02,
	CLASS_HAS_DESTRUCTOR = 0x04,
	CLASS_IS_SINGLETON = 0x08
} class_flags_t;

typedef enum func_flags_t
{
	FUNC_NO_FLAGS = 0x00,
	FUNC_CONST = 0x01,
	FUNC_VARIADIC = 0x02,
	FUNC_STATIC = 0x04,
	FUNC_VIRTUAL = 0x08,
} func_flags_t;

typedef struct enum_t
{
	self_ptr_t values;
	int16_t num_values;
	uint8_t is_bitfield;
} enum_t;


typedef struct enum_value_t
{
	self_ptr_t name; // NOTE: NULL TERMINATED
	int32_t value;
} enum_value_t;

typedef struct func_t
{
	self_ptr_t name_selfptr;
	self_ptr_t params;
	diff_ptr_t return_type; // tagged to discriminate between enums & classes (lets you quickly determine what the base addr is)
	int32_t name_len;
	int32_t __padding;
	int16_t num_params;
	uint8_t bitflags;
} func_t;

typedef struct param_t
{
	self_ptr_t name;
	diff_ptr_t type;
	self_ptr_t default_value;
	int32_t __padding;
} param_t;

typedef struct class_t
{
	api_map_t class_map; // hello, pointer-based polymorphism
	self_ptr_t constructors;
	diff_ptr_t inherits;
	// tbqh these just make debugging easier
	self_ptr_t name_selfptr;
	int32_t name_len;
	int16_t num_constructors;
	uint8_t bitflags;
} class_t;

typedef struct member_t
{
	diff_ptr_t type; // tagged
	int32_t __padding;
} member_t;

typedef struct constant_t
{
	diff_ptr_t type;
	self_ptr_t value; // null termianted string
} constant_t;


typedef struct signal_t
{
	self_ptr_t params;
	int16_t num_params;
} signal_t;


// wtf I actually need this shit?
typedef struct native_struct_t
{
	self_ptr_t format; // null  terminated string
	int32_t __padding;
} native_struct_t;

#endif /* End API_STRUCTS_H */
