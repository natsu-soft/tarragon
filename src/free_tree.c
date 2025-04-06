#include "free_tree.h"
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "rb_tree.h"
#include "error_macros.h"

#define RESIZE_SCALE 1.618
#define RESIZE_OVERHEAD 4096


void free_tree_init (free_tree_t* tree, size_t bytes)
{
	assertf(bytes > sizeof(rb_node_t), "allocation is too small!");
	tree->size = bytes;
	tree->memory = malloc(bytes);
	free_tree_free_all(tree);
}

void* free_tree_alloc(free_tree_t* tree, size_t bytes)
{
	/* TODO */
}

void free_tree_free(free_tree_t* tree, void* ptr)
{
	alloc_header_t* header = (uint8_t*)ptr - sizeof *header;
	const size_t free_size = header->size + header->padding;
	rb_node_t* node = header;
	memset(node, 0, sizeof *node);
	node->value = free_size;
	node->color = RB_RED;
	rb_insert(&tree->root, node);
}

void free_tree_free_all(free_tree_t* tree)
{
	rb_node_t* root = tree->memory;
	memset(root, 0, sizeof *root);
	root->value = tree->size;
}
