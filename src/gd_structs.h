#ifndef GD_STRUCTS_H
#define GD_STRUCTS_H

#include <stdint.h>

enum { // uint8_t
	GD_NONE = 0x00,
	GD_CONSTANT,
	GD_ENUM,
	GD_BITFIELD, // also a gd_enum_t lmao
	GD_ENUM_VALUE,
	GD_CLASS_T,
	GD_FUNCTION_T,
	GD_STRUCT_T
};

typedef struct gd_info_t // GOD I love intrusive structs
{
	char* name;
	char* description;
	uint8_t type;
} gd_info_t;

typedef struct gd_constant_t
{
	gd_info_t info;
} gd_constant_t;

typedef struct gd_enum_t
{
	gd_info_t info;
	struct gd_enum_value_t* values;
	uint32_t num_values;
} gd_enum_t;

typedef struct gd_enum_value_t
{
	gd_info_t info;
	int value;
} gd_enum_value_t;

typedef struct gd_class_t
{
	gd_info_t info;
	struct gd_class_t* inherits;
	struct gd_class_t* fields;
	char** field_names;
	struct gd_func_t* operators;
	struct gd_func_t* methods;
	uint8_t num_fields; // MORE THAN 255 FIELDS? I SERIOSULY DOUBT IT;
	uint8_t num_operators; // MORE THAN 255 METHODS I SERIOSULY DOUBT IT;
	uint8_t num_methods; // MORE THAN 255 METHODS I SERIOSULY DOUBT IT;
} gd_type_t;

typedef struct gd_struct_t
{
	gd_info_t info;
	gd_type_t* field_types;
	char** field_names;
	uint8_t num_fields;
} gd_struct_t;

typedef struct gd_func_t
{
	gd_info_t info;
	struct gd_type_t* return_type;
	struct gd_type_t* param_types;
	char** param_names;
	uint8_t num_params; // seriously--more than 255 parameters??? yeah right
} gd_func_t;

#endif /* End GD_STRUCTS_H */
