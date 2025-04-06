#ifndef FREE_TREE_H
#define FREE_TREE_H

// NOTE: you actually need this

#include <stddef.h>
#include <stdint.h>

/* typedef enum _policy_t */
/* { */
/* 	NONE = 0x00, */
/* 	BEST_FIT = 0x01, */
/* 	RESIZEABLE = 0x02, // the onus lies on OTHER CODE to make sure it uses relative pointers if this is chosen */
/* } rb_policy_t; */

typedef struct alloc_header_t
{
	size_t size;
	size_t padding;
} alloc_header_t;

typedef struct free_tree_t
{
	struct rb_node_t* root;
	uint8_t* memory;
	size_t size;
	/* size_t cap; */
	/* uint8_t policies; */
} free_tree_t;

void free_tree_init (free_tree_t* tree, size_t bytes);

void* free_tree_alloc(free_tree_t* tree, size_t bytes);

void free_tree_free(free_tree_t* tree, void* ptr);

void free_tree_free_all(free_tree_t* tree);

#endif /* End FREE_TREE_H */
