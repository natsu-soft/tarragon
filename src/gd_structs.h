#ifndef GD_STRUCTS_H
#define GD_STRUCTS_H

#include <stdint.h>

typedef struct gd_type_t gd_type_t;
typedef struct gd_func_t gd_func_t;

typedef struct gd_info_t
{
	char* name;
	char* description;
} gd_info_t;

typedef struct gd_type_t
{
	gd_info_t info;
	gd_type_t* parent;
	gd_type_t* fields;
	gd_func_t* methods;
	uint8_t num_fields; // MORE THAN 255 FIELDS? I SERIOSULY DOUBT IT;
	uint8_t num_methods; // MORE THAN 255 METHODS I SERIOSULY DOUBT IT;
} gd_type_t;

typedef struct gd_func_t
{
	gd_info_t info;
	gd_type_t* return_type;
	gd_type_t* param_types;
	char* param_names;
	uint8_t num_params; // seriously--more than 255 parameters??? yeah right
} gd_func_t;

#endif /* End GD_STRUCTS_H */
