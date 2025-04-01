#ifndef FREE_RB_H
#define FREE_RB_H

#include <stddef.h>
#include <stdint.h>

typedef enum rb_policy_t
{
	NONE = 0x00,
	BEST_FIT = 0x01,
	RESIZEABLE = 0x02, // the onus lies on OTHER CODE to make sure it uses relative pointers if this is chosen
} rb_policy_t;

typedef enum rb_color_t
{
	RED = 0x00,
	BLACK = 0x01
} rb_color_t;

typedef struct alloc_header_t
{
	size_t size;
	size_t padding;
} alloc_header_t;

typedef struct free_node_t
{
	struct free_node_t* parent;
	struct free_node_t* lhs;
	struct free_node_t* rhs;
	size_t size;
} free_node_t;

typedef struct free_rb_t
{
	free_node_t* root;
	uint8_t* memory;
	size_t size;
	size_t cap;
	uint8_t policies;
} free_rb_t;

void free_rb_init (free_rb_t* rb, size_t bytes);

void* free_rb_alloc(free_rb_t* rb, size_t bytes);

void free_rb_free(free_rb_t* rb, void* addr);

void free_rb_free_all(free_rb_t* rb);

static void free_rb_coalesce(free_rb_t* rb);

#endif /* End FREE_RB_H */
