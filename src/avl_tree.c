#include "avl_tree.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "error_macros.h"
#include "pointer_memes.h"

#define BALANCE

typedef enum __attribute__((packed)) dir_t
{
	LEFT = 0x00,
	RIGHT = 0x01
} dir_t;


void avl_print(avl_node_t* node, const unsigned int depth)
{
	if (node != NULL)
	{
		uint8_t flags = node->left ? 0x01 : 0x00;
		flags |= node->right ? 0x02 : 0x00;
		avl_print(node->right, depth + 1);
		for (int i = 0; i < depth; ++i)
		{
			printf("    ");
		}
		switch (flags)
		{
		case 0x03: printf("%4zu───┤\n", node->value); break;
		case 0x02: printf("%4zu───┘\n", node->value); break;
		case 0x01: printf("%4zu───┐\n", node->value); break;
		case 0x00: printf("%4zu\n", node->value); break;
		}
		avl_print(node->left, depth + 1);
	}
}

static inline int balance(avl_node_t* node)
{
	return (node->right != NULL ? 1 + node->right->depth : 0) - (node->left != NULL ? 1 + node->left->depth : 0);
}

static void update_depth(avl_node_t* node)
{
	for (/* :^) */; node != NULL; node = node->parent)
	{
		const size_t left_depth = node->left != NULL ? node->left->depth + 1: 0;
		const size_t right_depth = node->right != NULL ? node->right->depth + 1: 0;
		node->depth = right_depth > left_depth ? right_depth : left_depth;
	}
}

static void rotate(avl_node_t** node, const dir_t dir)
{
	const dir_t opp_dir = 1 - dir;
	avl_node_t* n = *node;
	avl_node_t* child = n->child[opp_dir];
	avl_node_t* parent = n->parent;
	if (parent != NULL)
	{
		const dir_t n_dir = parent->right == n ? RIGHT : LEFT;
		parent->child[n_dir] = child;
	}
	n->child[opp_dir] = child->child[dir];
	n->parent = child;
	child->child[dir] = n;
	child->parent = parent;
	update_depth(n);
	*node = child;
}

static void insert(avl_node_t* node, avl_node_t* new_node)
{
	dir_t dir = new_node->value >= node->value ? RIGHT : LEFT;
	avl_node_t** child = &node->child[dir];
	while (*child != NULL)
	{
		node = *child;
		dir = new_node->value >= node->value ? RIGHT : LEFT;
		child = &node->child[dir];
	}
	new_node->parent = node;
	new_node->depth = 0;
	*child = new_node;
}

static void delete(avl_node_t** root, avl_node_t** node)
{
	enum __attribute__((packed))
	{
		NONE = 0x00,
		HAS_LEFT_CHILD = 0x01,
		HAS_RIGHT_CHILD = 0x02,
		HAS_TWO_CHILDREN = 0x03,
		HAS_A_PARENT = 0x04,
	};
	avl_node_t* n = *node;
	avl_node_t* temp = NULL;
	avl_node_t* parent = n->parent;
	avl_node_t* left = n->left;
	avl_node_t* right = n->right;
	const uint8_t flags = (left != NULL) |
		((right != NULL) << 1) |
		((parent != NULL) << 2); // idgaf anymore
	const dir_t dir = flags & HAS_A_PARENT && parent->right == n ? RIGHT : LEFT;
	switch (flags & HAS_TWO_CHILDREN)
	{
	case HAS_TWO_CHILDREN:
		for (n = right; n->left != NULL; n = n->left);
		if (n != right)
		{
			temp = n->parent;
			temp->left = n->right;
			if (n->right != NULL)
			{
				n->right->parent = temp;
				temp = n->right;
			}
			n->right = right;
			right->parent = n;
		}
		else
		{
			temp = n;
		}
		n->left = left;
		left->parent = n;
		n->parent = parent;
		break;
	case HAS_LEFT_CHILD: __attribute__((fallthrough));
	case HAS_RIGHT_CHILD:
		n = n->child[flags & HAS_RIGHT_CHILD ? RIGHT : LEFT];
		n->parent = parent;
		temp = n;
		break;
	default:
		temp = parent;
		n = NULL;
	}
	if (parent != NULL)
	{
		parent->child[dir] = n;
	}
	else
	{
		*root = n;
	}
	update_depth(temp);
	*node = n;
}

static void retrace(avl_node_t** root, avl_node_t* node)
{
	avl_node_t* n0 = *root;
	for (/* :^) */; node != NULL; node = node->parent)
	{
		const int n_balane = balance(node);
		const dir_t dir = n_balane > 0 ? RIGHT : LEFT;
		if (n_balane < -1 || 1 < n_balane )
		{
			avl_node_t* child = node->child[dir];
			const int c_balance = balance(child);
			if (n_balane * c_balance < 0)
			{
				rotate(&child, dir);
			}
			rotate(&node, 1 - dir);
		}
		n0 = node;
	}
	*root = n0;
}

void avl_insert(avl_node_t** root, avl_node_t* node)
{
	if (*root != NULL)
	{
		insert(*root, node);
		const avl_node_t* parent = node->parent;
		update_depth(parent);
		#ifdef BALANCE
		retrace(root, parent->parent);
		#endif
	}
	else
	{
		node->left = NULL;
		node->right = NULL;
		*root = node;
	}
}

void avl_remove(avl_node_t** root, avl_node_t* node)
{
	if (*root != NULL)
	{
		avl_node_t* parent = node->parent;
		delete(root, &node);
		#ifdef BALANCE
		retrace(root, node != NULL ? node : parent);
		#endif
	}
}

avl_node_t* avl_find_min_gte(avl_node_t* node, const size_t value)
{
	enum __attribute__((packed))
	{
		EQUAL = 0x00,
		LESS_THAN = 0x01,
		GREATER_THAN = 0x02,
	};
	avl_node_t* res = NULL;
	while (node != NULL)
	{
		const size_t node_val = node->value;
		const uint8_t flags = (node_val < value) | ((node_val > value) << 1);
		switch (flags)
		{
		case LESS_THAN:
			node = node->right;
			break;
		case GREATER_THAN:
			res = node;
			node = node->left;
			break;
		default:
			return node;
		}
	}
	return res;
}

avl_node_t* avl_first_gte(avl_node_t* node, const size_t value)
{
	for(; node != NULL; node = node->right)
	{
		if (node->value >= value)
		{
			return node;
		}
	}
	return NULL;
}

void avl_relative_ptrs(avl_node_t* node)
{
	for (int i = 0; i < 2; ++i)
	{
		if (node->child[i] != NULL)
		{
			avl_relative_ptrs(node->child[i]);
			point_relative(&node->child[i]);
		}
	}
	if (node->parent)
	{
		point_relative(&node->parent);
	}
}

void avl_absolute_ptrs(avl_node_t* node)
{
	for (int i = 0; i < 2; ++i)
	{
		if (node->child[i] != NULL)
		{
			point_absolute(&node->child[i]);
			avl_absolute_ptrs(node->child[i]);
		}
	}
	if (node->parent)
	{
		point_absolute(&node->parent);
	}
}

size_t avl_value_sum(avl_node_t* node)
{
	size_t sum = node->value;
	for (int i = 0; i < 2; ++i)
	{
		if (node->child[i] != NULL)
		{
			sum += avl_value_sum(node->child[i]);
		}
	}
	return sum;
}

size_t avl_num_nodes(avl_node_t* node)
{
	size_t num = 1;
	for (int i = 0; i < 2; ++i)
	{
		if (node->child[i] != NULL)
		{
			num += avl_num_nodes(node->child[i]);
		}
	}
	return num;
}

void avl_populate_list(avl_node_t* node, avl_node_t** list, size_t* index)
{
	const size_t i = *index;
	list[i] = node;
	*index = i + 1;
	for (int i = 0; i < 2; ++i)
	{
		if (node->child[i] != NULL)
		{
			avl_populate_list(node->child[i], list, index);
		}
	}
}

int avl_tree_test(int argc, char** argv)
{
	#define NUM 9999
	avl_node_t* nodes = malloc(NUM * sizeof *nodes);
	avl_node_t* other_nodes = malloc(NUM * sizeof *nodes);
	/* printf("%d, %d\n", (size_t)nodes & (sizeof(size_t) - 1), (size_t)other_nodes & (sizeof(size_t) - 1)); */
	/* printf("alloca: [%#zx, %#zx)\n", (size_t)nodes, (size_t)nodes + NUM * sizeof *nodes); */
	memset(nodes, 0x00, NUM * sizeof *nodes);
	printf("  -- insertion:");
	avl_node_t* root = NULL;
	for (int i = 0; i < NUM; ++i)
	{
		nodes[i].value = i;
		avl_insert(&root, nodes + i);
		/* printf("\n===================== i: %d =====================\n\n", i); */
		/* struggle_print(root, 0); */
		const int r_balance = balance(root);
		assertf(-2 < r_balance && r_balance < 2, "ur fukt: %d", i);
	}
	printf(" GOOD!\n");
	printf("  -- translation:");
	avl_relative_ptrs(root);
	point_relative_offset(&root, nodes - 1);
	memcpy(other_nodes, nodes, NUM * sizeof *nodes);
	free(nodes); // HIT ME WITH THE SEGFAULT
	nodes = NULL;
	point_absolute_offset(&root, other_nodes - 1);
	avl_absolute_ptrs(root);
	printf(" GOOD!\n");
	/* printf("  -- stress relative to absolute: "); */
	/* for (int i = 0; i < 999; ++i) */
	/* { */
	/* 	avl_relative_ptrs(root); */
	/* 	point_relative_offset(&root, other_nodes - 1); */
	/* 	point_absolute_offset(&root, other_nodes - 1); */
	/* 	avl_absolute_ptrs(root); */
	/* } */
	/* printf(" GOOD!\n"); */
	printf("  -- removal:");
	/* printf("ptrs: OK!\n"); */
	for (int i = 0; i < NUM - 1; ++i)
	{
		avl_remove(&root, other_nodes + i);
		/* printf("\n===================== i: %d =====================\n\n", i); */
		/* struggle_print(root, 0); */
		const int r_balance = balance(root);
		assertf(-2 < r_balance && r_balance < 2, "ur fukt2: %d", i);
	}
	avl_remove(&root, other_nodes + NUM - 1);
	assert(root == NULL);
	free(other_nodes);
	printf(" GOOD!\n");
	return 0;
}
