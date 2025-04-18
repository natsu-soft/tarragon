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

typedef struct free_tree_t
{
	struct avl_node_t* root;
	size_t size;
	uint8_t* memory;
	uint8_t valid_root; // JUST ADD IN A FLAG LMAO
	/* size_t cap; */
	/* uint8_t policies; */
} free_tree_t;

/* void free_tree_relative_ptrs(free_tree_t* tree); */

/* void free_tree_absolute_ptrs(free_tree_t* tree); */

void free_tree_init (free_tree_t* tree, const size_t bytes);

void free_tree_grow(free_tree_t* tree, const size_t bytes);

void* free_tree_alloc(free_tree_t* tree, const size_t bytes);

// this was TOO based
/* void* free_tree_realloc(free_tree_t* tree, void* ptr, const size_t bytes); */

// ONLY shrinks allocations
void* free_tree_dealloc(free_tree_t* tree, void* ptr, const size_t bytes);

void free_tree_free(free_tree_t* tree, const void* ptr);

void free_tree_clear(free_tree_t* tree);

void free_tree_serialize(free_tree_t* tree, void* destination);

void free_tree_deserialize(free_tree_t* tree, void* source);

float free_tree_coverage(free_tree_t* tree);

#endif /* End FREE_TREE_H */
