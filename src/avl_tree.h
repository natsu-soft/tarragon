#ifndef AVL_TREE_H
#define AVL_TREE_H

#include <stddef.h>
#include <stdint.h>

typedef struct avl_node_t
{
	struct avl_node_t* parent;
	union
	{
		struct
		{
			struct avl_node_t* left;
			struct avl_node_t* right;
		};
		struct avl_node_t* child[2];
	};
	size_t value;
	size_t depth;
} avl_node_t;

void avl_insert(avl_node_t** root, avl_node_t* node);

void avl_remove(avl_node_t** root, avl_node_t* node);

avl_node_t* avl_find_min_gte(avl_node_t* node, const size_t value);

void avl_relative_ptrs(avl_node_t* node);

void avl_absolute_ptrs(avl_node_t* node);

size_t avl_value_sum(avl_node_t* node);

#endif /* End AVL_TREE_H */
