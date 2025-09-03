#ifndef BUILTIN_H
#define BUILTIN_H

#include <stdint.h>
#include "gdscript/gdscript_ids.h"
typedef struct supported_version_t
{
	int16_t major;
	int16_t minor;
	int16_t patch;
} supported_version_t;

typedef struct gdscript_enum_t
{
	char* name;
	struct enum_value_t* values;
	int16_t name_len;
	int16_t num_values;
	uint8_t is_bitfield;
} gdscript_enum_t;

typedef struct enum_value_t
{
	char* name;
	char* desc;
	int32_t value;
	int16_t desc_len;
	int16_t name_len;
} enum_value_t;

typedef enum gdscript_func_flags_t
{
	FUNC_NO_FLAGS = 0x00,
	FUNC_VARARG = 0x01,
	FUNC_CONST = 0x02,
	FUNC_STATIC = 0x04,
	FUNC_VIRTUAL = 0x08,
} gdscript_func_flags_t;

typedef struct gdscript_func_t
{
	char* name;
	struct func_param_t* params;
	char* desc;
	int32_t ret_index; // odd ret index is an enum ( ^ 0b1 it to get the real index)
	int32_t class_index; // -1 means global
	int16_t name_len;
	int16_t desc_len;
	int16_t num_params;
	uint8_t bitflags;
} gdscript_func_t;

typedef struct func_param_t
{
	char* name;
	int32_t name_len;
	int32_t type_index; // indices that are odd are enums ( ^ 0b1 it to get the real index)
} func_param_t;

typedef enum gdscript_class_flags_t
{
	CLASS_NO_FLAGS = 0x00,
	CLASS_KEYED = 0x01,
	CLASS_DESTRUCTOR = 0x02,
} gdscript_class_flags_t;

typedef struct gdscript_class_t
{
	char* name;
	struct operator_t* operators;
	struct gdscript_func_t* methods;
	struct constructor_t* constructors;
	int16_t name_len;
	int16_t num_operators;
	int16_t num_methods;
	int8_t num_constructors;
	uint8_t bitflags;
} gdscript_class_t;

typedef struct operator_t
{
	char* desc;
	int32_t ret_index;
	int32_t right_index; // if -1 it's unary
	int16_t desc_len;
	gdscript_id_t id;
} operator_t;

void gdscript_builtin(void);

void print_supported_version(void);

#endif /* End BUILTIN_H */
