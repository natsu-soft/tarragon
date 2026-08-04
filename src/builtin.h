// NOTE: UNUSED
#ifndef BUILTIN_H
#define BUILTIN_H

#include <stdint.h>
#include "gdscript/token_ids.h"
#include "hashmaps/ptr_map.h"
#include "hashmaps/op_map.h"

// NOTE: no descriptions atm

typedef enum api_tags_t
{
	API_NONE = 0x00,
	API_ENUM,
	API_FUNC,
	API_TYPE, // types have operators
	API_CLASS, // classes do not!
	API_SIGNAL,
	API_MEMBER,
	API_CONSTANT
} api_tags_t;

typedef enum class_flags_t
{
	CLASS_NO_FLAGS = 0x00,
	CLASS_IS_KEYED = 0x01,
	CLASS_HAS_DESTRUCTOR = 0x02,
	CLASS_IS_SINGLETON = 0x04
} class_flags_t;

// NOTE: NOTHING NEEDS A NAME--THE KEY & LEN IN THE INDEX MAP IS ITS NAME!
// ok, some things need a name but not everything needs a name
typedef struct type_t
{
	ptr_map_t tagged_map;
	op_map_t op_map;
	struct func_t* constructors;
	uint8_t bitflags;
} type_t;

typedef struct class_t
{
	ptr_map_t tagged_map;
	struct class_t* parent;
	struct func_t* constructors;
	uint8_t bitflags;
} class_t;

typedef struct enum_t
{
	struct enum_value_t* values;
	int16_t num_values;
	uint8_t is_bitfield;
} enum_t;

// ok these need a name
typedef struct enum_value_t
{
	char* name;
	int32_t value;
	/* int16_t name_len; */
} enum_value_t;

typedef enum func_flags_t
{
	FUNC_NO_FLAGS = 0x00,
	FUNC_CONST = 0x01,
	FUNC_VARARG = 0x02,
	FUNC_STATIC = 0x04,
	FUNC_VIRTUAL = 0x08,
} func_flags_t;

// NOTE: functions have to be linked lists for overloading
typedef struct func_t
{
	/* union */
	/* { */
	/* 	char* name; */
	/* 	struct func_t* next; */
	/* }; */
	struct func_t* next;
	struct param_t* params;
	void* return_type; // also tagged
	int16_t num_params;
	uint8_t bitflags;
} func_t;

typedef struct signal_t
{
	struct param_t* params;
	int32_t num_params;
} signal_t;

typedef struct param_t
{
	char* name;
	char* default_value;
	void* type;
	/* int16_t name_len; */
	/* int16_t default_value_len; */
} param_t;


// NOTE: operators have to be linked lists for overloading
typedef struct operator_t
{
	struct operator_t* next;
	void* return_type;
	void* right_type; // -1 if unary
} operator_t;

typedef struct member_t
{
	char* desc; // unused atm
	void* type;
} member_t;

typedef struct constant_t
{
	char* value;
	void* type;
	int16_t value_len;
} constant_t;


void build_gdscript_api(void);

void print_supported_version(void);

#endif /* End BUILTIN_H */
