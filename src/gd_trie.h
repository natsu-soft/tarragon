#ifndef GD_TRIE_H
#define GD_TRIE_H
// NOTE-- not using a radix tree because that hurts my BRAIN

// NOTE: arena for api data then switch to free list for user data 
// see https://www.gingerbill.org/article/2021/11/30/memory-allocation-strategies-005/

#include "gd_structs.h"

typedef unsigned long size_t;

typedef void* (*trie_malloc_t)(size_t);
typedef void (*trie_free_t)(void*);

typedef struct trie_elem_t trie_elem_t;

/* typedef unsigned char uint8_t; */
/* typedef unsigned long uint32_t; */

/* typedef struct gd_constant_t gd_constant_t; */
/* typedef struct gd_enum_t gd_enum_t; */
/* typedef struct gd_method_t gd_method_t; */
/* typedef struct gd_class_t gd_class_t; // ALL types are classes (check the API) */
/* typedef struct gd_struct_t gd_struct_t; */
/* typedef struct gd_singleton_t gd_singleton_t; */
/* typedef struct gd_signal_t gd_signal_t; */

/* typedef struct gd_enum_def_t gd_enum_def_t; */
/* typedef struct gd_enum_t gd_enum_t; */

/* typedef union gd_type_t gd_type_t; */

/* typedef struct gd_signal_t */
/* { */
/* 	char* name; */
/* 	gd_type_t* parent; */
/* 	char* description; */
/* 	gd_type_t* param_types; */
/* 	char* param_names; */
/* 	uint8_t num_params; */
/* } gd_signal_t; */


/* typedef struct gd_header_t */
/* { */
/* 	char* name; */
/* 	char* description; */
/* 	gd_class_t* parent; */
/* } gd_header_t; */

/* typedef struct gd_constant_t */
/* { */
/* 	gd_header_t header; */
/* 	union { */
/* 		int integer; */
/* 		float real; */
/* 	} constant_value; */
/* 	enum { */
/* 		CONSTANT_INT, */
/* 		CONSTANT_REAL */
/* 	} constant_type; */
/* } gd_constant_t; */

/* typedef struct gd_enum_t */
/* { */
/* 	gd_header_t header; */
/* } gd_enum_t; */

/* typedef struct gd_class_t */
/* { */
/* 	gd_header_t header; */
/* 	gd_method_t* methods; */
/* 	uint8_t num_methods; */
/* } gd_class_t; */

/* typedef enum method_mask_t { */
/* 	METHOD_NONE = 0x00, */
/* 	METHOD_CONST = 0x01, */
/* 	METHOD_STATIC = 0x02, */
/* 	METHOD_VIRTUAL = 0x04, */
/* 	METHOD_VARIADIC = 0x08 */
/* } method_mask_t; */

/* typedef struct gd_method_t */
/* { */
/* 	gd_header_t header; */
/* 	gd_class_t* ret_value; */
/* 	gd_class_t* arg_types; */
/* 	char* arg_names; */
/* 	uint8_t num_args; */
/* 	uint8_t method_mask; */
/* } gd_method_t; */


/* typedef struct trie_value_t */
/* { */
/* 	enum { */
/* 		CHAR = 0, */
/* 		CONSTANT, */
/* 		ENUM, */
/* 		METHOD, */
/* 		CLASS, */
/* 		STRUCT, */
/* 		SINGLETON, */
/* 		SIGNAL */
/* 	} type; */
/* 	union */
/* 	{ */
/* 		char c; */
/* 		gd_constant_t gd_const; */
/* 		gd_enum_t gd_enum; */
/* 		gd_method_t gd_method; */
/* 		gd_class_t gd_class; */
/* 		gd_struct_t gd_struct; */
/* 		gd_singleton_t gd_singleton; */
/* 		gd_signal_t gd_signal; */
/* 	} value; */
/* } trie_value_t; */

typedef struct trie_ll_t trie_ll_t;

typedef struct trie_ll_t
{
	trie_elem_t* elem;
	trie_ll_t* next;
} trie_ll_t;

typedef struct trie_elem_t
{
	union {
		char c;
		gd_type_t* type;
		gd_func_t* func;
	} value;
	/* trie_elem_t* parent; */
	trie_ll_t* children;
	/* trie_value_t* value; */
	enum {
		TRIE_CHAR = 0x00,
		TRIE_TYPE,
		TRIE_FUNC,
	} value_type;
} trie_elem_t;


void set_gd_trie_alloc_funcs(trie_malloc_t malloc_fn, trie_free_t free_fn);

void add_child(trie_elem_t* elem, trie_elem_t* child);

void remove_child(trie_elem_t* elem, trie_elem_t* child);

#endif /* End GD_TRIE_H */
