#include "free_tree.h"
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "avl_tree.h"
#include "error_macros.h"

#define RESIZE_SCALE 1.618
#define RESIZE_OVERHEAD 4096


void free_tree_init (free_tree_t* tree, const size_t bytes)
{
	assertf(bytes > sizeof(avl_node_t), "allocation is too small, must be atleast %zu!", sizeof(avl_node_t));
	tree->size = bytes;
	tree->memory = malloc(bytes);
	free_tree_free_all(tree);
}

void* free_tree_alloc(free_tree_t* tree, const size_t bytes)
{
	avl_node_t* node = avl_find_min_gte(tree->root, bytes + sizeof(alloc_header_t));
	if (node != NULL)
	{
		const size_t total = node->value;
		avl_remove(&tree->root, node);
		alloc_header_t* header = node;
		header->size = bytes;
		const size_t align = sizeof(size_t);
		const size_t modulo = ((size_t)node + bytes) & (align - 1);
		const size_t padding = modulo > 0 ? align - modulo: 0;
		header->padding = padding;
		if (total > bytes + padding)
		{
			node = ((uint8_t*)header + bytes + padding);
			memset(node, 0x00, sizeof *node);
			node->value = total - bytes - padding;
			avl_insert(&tree->root, node);
		}
		return header + 1;
	}
	else
	{
		const size_t m0 = tree->size;
		const size_t new_size = RESIZE_SCALE * m0;
		const size_t m1 = (new_size - m0 > bytes ? new_size : m0 + bytes + sizeof(alloc_header_t*)) + RESIZE_OVERHEAD;
		// relativize & realloc
		uint8_t* memory = tree->memory;
		const uint8_t root_flag = tree->root != NULL ? 1 : 0;
		size_t root_offset = 0;
		if (root_flag)
		{
			root_offset = (size_t)tree->root - (size_t)memory;
			avl_relative_ptrs(tree->root);
		}
		memory = realloc(memory, m1);
		if (root_flag)
		{
			tree->root = memory + root_offset;
			avl_node_t* root = tree->root;
			avl_absolute_ptrs(root);
			if (root->left == NULL && root->right == NULL)
			{
				root->value = m1;
				goto retry;
			}
		}
		// YES, there are SOME leaks--MOTHER OF ALL OMLETTES, JACK!
		const size_t align = sizeof(size_t);
		const size_t modulo = ((size_t)memory + m0) & (align - 1);
		const size_t padding = modulo > 0 ? align - modulo : 0;
		node = memory + m0 + padding;
		memset(node, 0x00, sizeof *node);
		node->value = m1 - m0 - padding;
		avl_insert(&tree->root, node);
	retry:
		tree->size = m1;
		tree->memory = memory;
		return free_tree_alloc(tree, bytes);
	}
}

// for arrays
void* free_tree_realloc(free_tree_t* tree, void* ptr, size_t bytes)
{
	alloc_header_t* header = (alloc_header_t*)ptr - 1;
	const size_t size = header->size;
	const size_t padding = header->padding;
	if (bytes > size + padding)
	{
		// 1. copy to temp buffer
		// 2. free
		// 3. alloc
		// 4. copy from temp buffer
	}
	else if (bytes >= size)
	{
		header->padding -= bytes - size;
		header->size = bytes;
	}
	else
	{
		header->padding += size - bytes;
		if (header->padding >= sizeof(avl_node_t*))
		{
			// NEW NODE!
		}
		header->size = bytes;
	}
	return ptr;
}


void free_tree_free(free_tree_t* tree, const void* ptr)
{
	alloc_header_t* header = (alloc_header_t*)ptr - 1;
	const size_t free_size = header->size + header->padding;
	avl_node_t* node = header;
	memset(node, 0, sizeof *node);
	node->value = free_size;
	avl_insert(&tree->root, node);
}

void free_tree_free_all(free_tree_t* tree)
{
	avl_node_t* root = tree->memory;
	memset(root, 0, sizeof *root);
	root->value = tree->size;
	tree->root = root;
}


static void relative(free_tree_t* tree)
{
	avl_relative_ptrs(tree->root);
	tree->root = (void*)((size_t)tree->root - (size_t)tree->memory);
}

static void absolute(free_tree_t* tree)
{
	tree->root = (void*)((size_t)tree->root + (size_t)tree->memory);
	avl_absolute_ptrs(tree->root);
}


void free_tree_serialize(free_tree_t* tree, void* destination)
{
	uint8_t* dest = destination;
	relative(tree);
	memcpy(dest , &tree->root, sizeof tree->root);
	size_t offset = sizeof tree->root;
	memcpy(dest + offset, &tree->size, sizeof tree->size);
	offset += sizeof tree->size;
	memcpy(dest + offset, tree->memory, tree->size);
	absolute(tree);
}

void free_tree_deserialize(free_tree_t* tree, void* source)
{
	uint8_t* src = source;
	if (tree->memory != NULL)
	{
		free(tree->memory);
	}
	memcpy(&tree->root, src, sizeof tree->root);
	size_t offset = sizeof tree->root;
	memcpy(&tree->size, src + offset, sizeof tree->size);
	offset += sizeof tree->size;
	tree->memory = malloc(tree->size);
	memcpy(tree->memory, src + offset, tree->size);
	absolute(tree);
}
