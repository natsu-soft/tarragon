#include "avl_tree.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "error_macros.h"

typedef enum __attribute__((packed)) dir_t
{
	LEFT = 0x00,
	RIGHT = 0x01
} dir_t;


static void struggle_print(avl_node_t* node, const int depth)
{
	uint8_t flags = node->left ? 0x01 : 0x00;
	flags |= node->right ? 0x02 : 0x00;
	if (node->right)
	{
		struggle_print(node->right, depth + 1);
	}
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
	if (node->left)
	{
		struggle_print(node->left, depth + 1);
	}
}

static inline int balance(avl_node_t* node)
{
	return (node->right != NULL ? node->right->depth : 0) - (node->left != NULL ? node->left->depth : 0);
}

static void update_depth(avl_node_t* node)
{
	int depth = 0;
	for (/* :^) */; node != NULL; node = node->parent)
	{
		if (node->left)
		{
			depth = node->left->depth + 1;
		}
		if (node->right)
		{
			const int right_depth = node->right->depth + 1;
			depth = right_depth > depth ? right_depth : depth;
		}
		node->depth = depth;
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
	child->child[dir] = n;
	child->parent = parent;
	n->parent = child;
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
	*child = new_node;
}

static void delete(avl_node_t** root, avl_node_t** node)
{
	enum __attribute__((packed))
	{
		NONE = 0x00,
		HAS_LEFT_CHILD = 0x01,
		HAS_RIGHT_CHILD = 0x02,
		HAS_A_CHILD = 0x03,
		HAS_SUCCESSOR = 0x04,
		HAS_PARENT = 0x08,
	};
	avl_node_t* n = *node;
	avl_node_t* parent = n->parent;
	avl_node_t* left = n->left;
	avl_node_t* right = n->right;
	uint8_t flags = left != NULL ? HAS_LEFT_CHILD : NONE;
	flags |= right != NULL ? HAS_RIGHT_CHILD : NONE;
	flags |= parent != NULL ? HAS_PARENT : NONE;
	flags |= flags & HAS_RIGHT_CHILD && right->left != NULL ? HAS_SUCCESSOR : NONE;
	const dir_t dir = flags & HAS_PARENT && parent->right == n ? RIGHT : LEFT;
	if (flags & HAS_SUCCESSOR)
	{
		avl_node_t* successor = right->left;
		for (; successor->left != NULL; successor = successor->left); // DEVILISH
		const int value = successor->value;
		avl_node_t* successor_parent = successor->parent;
		successor_parent->left = successor->right;
		if (successor->right != NULL)
		{
			successor->right->parent = successor_parent;
		}
		memcpy(successor, n, sizeof *n);
		successor->value = value;
		if (flags & HAS_RIGHT_CHILD)
		{
			right->parent = successor;
		}
		if (flags & HAS_LEFT_CHILD)
		{
			left->parent = successor;
		}
		*node = successor;
		n = successor_parent;
	}
	else if (flags & HAS_RIGHT_CHILD)
	{
		right->parent = parent;
		if (flags & HAS_LEFT_CHILD)
		{
			left->parent = right;
		}
		n = right;
		*node = right;
	}
	else if (flags & HAS_LEFT_CHILD)
	{
		// actually yeah this is it
		left->parent = parent;
		n = left;
		*node = left;
	}
	else
	{
		n = parent;
		*node = NULL;
	}
	if (flags & HAS_PARENT)
	{
		parent->child[dir] = *node;
	}
	else
	{
		*root = *node;
	}
	update_depth(n);
}

static void retrace(avl_node_t** root, avl_node_t* node)
{
	avl_node_t* n0 = *root;
	enum __attribute__((packed))
	{
		NONE = 0x00,
		LEFT_HEAVY = 0x01,
		RIGHT_HEAVY = 0x02,
		QUEUE_ROTATE = 0x03,
		ROTATED = 0x04,
	};
	uint8_t flags = NONE;
	for (/* :^) */; node != NULL; node = node->parent)
	{
		const int n_balance = balance(node);
		flags &= ROTATED;
		flags |= n_balance > 1 ? RIGHT_HEAVY : NONE;
		flags |= n_balance < -1 ? LEFT_HEAVY : NONE;
		const dir_t dir = n_balance > 0 ? RIGHT : LEFT;
		if (flags & QUEUE_ROTATE)
		{
			avl_node_t* child = node->child[dir];
			const int c_balance = balance(child);
			if (n_balance * c_balance < 0)
			{
				/* if (verbose) printf(">> R0T %s\n", dir == RIGHT ? "RIGHT" : "LEFT"); */
				rotate(&child, dir);
			}
			/* if (verbose) printf("> ROT %s\n", 1 - dir == RIGHT ? "RIGHT" : "LEFT"); */
			/* if (verbose && node->parent == NULL && node != *root) */
			/* { */
			/* 	printf("HOLY SHIT UR FUKT"); */
			/* } */
			rotate(&node, 1 - dir);
			flags |= ROTATED;
		}
		n0 = node;
	}
	*root = n0;
	/* if (verbose && flags & ROTATED) */
	/* { */
	/* 	printf("\n"); */
	/* 	struggle_print(*root, 0); */
	/* } */
}

void avl_insert(avl_node_t** root, avl_node_t* node)
{
	if (*root != NULL)
	{
		insert(*root, node);
		avl_node_t* parent = node->parent;
		update_depth(parent);
		/* struggle_print(*root, 0); */
		retrace(root, parent->parent);
	}
	else
	{
		*root = node;
		/* struggle_print(*root, 0); */
	}
}

void avl_remove(avl_node_t** root, avl_node_t* node)
{
	if (*root != NULL)
	{
		avl_node_t* parent = node->parent;
		delete(root, &node);
		retrace(root, node != NULL ? node : parent);
	}
}

avl_node_t* avl_find_min_gte(avl_node_t* node, const size_t value)
{
	avl_node_t* res = node->value >= value ? node : NULL;
	while (node != NULL)
	{
		const size_t v = node->value;
		if (v == value)
		{
			return node;
		}
		else if (v > value)
		{
			res = node;
			node = node->left;
		}
		else
		{
			node = node->right;
		}
	}
	return res;
}

void avl_relative_ptrs(avl_node_t* node)
{
	if (node->parent != NULL)
	{
		node->parent = (void*)((size_t)node->parent - (size_t)node);
	}
	for (int i = 0; i < 2; ++i)
	{
		if (node->child[i] != NULL)
		{
			avl_relative_ptrs(node->child[i]);
		}
		node->child[i] = (void*)((size_t)node->child[i] - (size_t)node);
	}
}

void avl_absolute_ptrs(avl_node_t* node)
{
	if (node->parent != NULL)
	{
		node->parent = (void*)((size_t)node->parent + (size_t)node);
	}
	for (int i = 0; i < 2; ++i)
	{
		node->child[i] = (void*)((size_t)node->child[i] + (size_t)node);
		if (node->child[i] != NULL)
		{
			avl_absolute_ptrs(node->child[i]);
		}
	}
}

/* #define STANDALONE_AVL_TREE_C */
#ifdef STANDALONE_AVL_TREE_C

int main(int argc, char** argv)
{
	#define NUM 10000
	avl_node_t* nodes = alloca(NUM * sizeof *nodes);
	memset(nodes, 0x00, NUM * sizeof *nodes);
	avl_node_t* root = NULL;
	for (int i = 0; i < NUM; ++i)
	{
		/* printf("\n===================== i: %d =====================\n\n", i); */
		nodes[i].value = i;
		avl_insert(&root, nodes + i);
		/* struggle_print(root, 0); */
		const int r_balance = balance(root);
		assertf(-2 < r_balance && r_balance < 2, "ur fukt: %d", i);
	}
	printf("insertion: OK!\n");
	avl_relative_ptrs(root);
	avl_absolute_ptrs(root);
	printf("ptrs: OK!\n");
	for (int i = 0; i < NUM - 1; ++i)
	{
		/* printf("\n===================== i: %d =====================\n\n", i); */
		avl_remove(&root, nodes + i);
		/* struggle_print(root, 0); */
		const int r_balance = balance(root);
		assertf(-2 < r_balance && r_balance < 2, "ur fukt2: %d", i);
	}
	avl_remove(&root, nodes + NUM - 1);
	assert(root == NULL);
	printf("removal: OK!\n");
	return 0;
}

#endif
