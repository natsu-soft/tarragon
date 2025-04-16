#include "free_tree.h"
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "avl_tree.h"
#include "error_macros.h"
#include "pointer_memes.h"


/* typedef struct alloc_header_t */
/* { */
/* 	size_t size; */
/* 	size_t padding; */
/* } alloc_header_t; */

// NOTE: do this so that way you ALWAYS get a node when you free
typedef union alloc_header_t
{
	struct
	{
		size_t size;
		size_t padding;
	};
	uint8_t requisite_size[sizeof(avl_node_t)];
} alloc_header_t;

/* #define RESIZE_SCALE 1.618 */
#define KILOBYTE 1024
#define RESIZE_SCALE 1
/* #define RESIZE_OVERHEAD 4096 */
#define RESIZE_OVERHEAD 8 * KILOBYTE


static void relative(free_tree_t* tree)
{
	avl_relative_ptrs(tree->root);
	point_relative_to(&tree->root, tree->memory);
}

static void absolute(free_tree_t* tree)
{
	point_absolute_to(&tree->root, tree->memory);
	avl_absolute_ptrs(tree->root);
}


void free_tree_init (free_tree_t* tree, const size_t bytes)
{
	assertf(bytes > sizeof(avl_node_t), "allocation is too small, must be atleast %zu!", sizeof(avl_node_t));
	tree->size = bytes;
	tree->memory = malloc(bytes);
	free_tree_clear(tree);
}

void* free_tree_alloc(free_tree_t* tree, const size_t bytes)
{
	const size_t header_size = sizeof(alloc_header_t);
	avl_node_t* node = avl_find_min_gte(tree->root, bytes + header_size);
	if (node != NULL)
	{
		const size_t total = node->value;
		avl_remove(&tree->root, node);
		alloc_header_t* header = node;
		void* res = header + 1;
		header->size = bytes;
		const size_t align = sizeof(avl_node_t);
		const size_t modulo = ((size_t)res + bytes) & (align - 1);
		const size_t padding = modulo > 0 ? align - modulo: 0;
		/* printf("allocated: %zu, padding: %zu\n", bytes, padding); */
		header->padding = padding;
		if (total > bytes + padding + header_size + sizeof *node)
		{
			node = ((uint8_t*)res + bytes + padding);
			memset(node, 0x00, sizeof *node);
			node->value = total - bytes - padding - header_size;
			avl_insert(&tree->root, node);
		}
		else
		{
			header->padding = total - bytes - header_size;
		}
		return res;
	}
	else
	{
		const size_t m0 = tree->size;
		const size_t m1 = RESIZE_SCALE * m0 + bytes + RESIZE_OVERHEAD;
		/* printf("m1: %zu\n", m1); */
		// relativize & realloc
		/* uint8_t* memory = tree->memory; */
		const uint8_t root_flag = tree->root != NULL ? 1 : 0;
		size_t root_offset = 0;
		if (root_flag)
		{
			relative(tree);
		}
		/* uint8_t* temp = alloca(m0); */
		/* memcpy(temp, tree->memory, m0); */
		/* free(tree->memory); */
		uint8_t* memory = realloc(tree->memory, m1);
		/* memcpy(memory, tree->memory, m0); */
		/* free(tree->memory); */
		/* memcpy(memory, temp, m0); */
		/* free(temp); */
		/* uint8_t* memory = realloc(tree->memory, m1); */
		if (root_flag)
		{
			absolute(tree);
			avl_node_t* root = tree->root;
			if (root->left == NULL && root->right == NULL)
			{
				root->value = m1;
				goto retry;
			}
		}
		// YES, there are SOME leaks--MOTHER OF ALL OMLETTES, JACK!
		const size_t align = sizeof(avl_node_t);
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

/* void* free_tree_realloc(free_tree_t* tree, const void* ptr, const size_t bytes) */
/* { */
/* 	if (ptr != NULL) */
/* 	{ */
/* 		free_tree_free(tree, ptr); */
/* 	} */
/* 	return free_tree_alloc(tree, bytes); */
/* } */

// for arrays
void* free_tree_realloc(free_tree_t* tree, const void* ptr, const size_t bytes)
{
	if (ptr != NULL)
	{
		alloc_header_t* header = ((alloc_header_t*)ptr) - 1;
		const size_t size = header->size;
		const size_t padding = header->padding;
		if (bytes > size + padding)
		{
			uint8_t* temp = alloca(size);
			memcpy(temp, ptr, size);
			free_tree_free(tree, ptr);
			ptr = free_tree_alloc(tree, bytes);
			memcpy(ptr, temp, size);
		}
		else if (bytes >= size) // size <= bytes <= size + padding
		{
			header->padding -= bytes - size;
			header->size = bytes;
		}
		else // bytes < size
		{
			header->padding += size - bytes;
			header->size = bytes;
			const size_t align = sizeof(avl_node_t);
			if (header->padding >= align)
			{
				size_t modulo = ((size_t)ptr + bytes) % (align - 1);
				modulo = modulo > 0 ? align - modulo : 0;
				avl_node_t* node = (uint8_t*)ptr + bytes + modulo;
				memset(node, 0x00, align);
				node->value = padding - align;
				avl_insert(&tree->root, node);
				header->padding -= node->value;
			}
		}
		return ptr;
	}
	else
	{
		return free_tree_alloc(tree, bytes);
	}
}

void free_tree_free(free_tree_t* tree, const void* ptr)
{
	const alloc_header_t* header = ((alloc_header_t*)ptr) - 1;
	const size_t free_size = header->size + header->padding + sizeof *header;
	avl_node_t* node = header;
	memset(node, 0x00, sizeof *node);
	node->value = free_size;
	avl_insert(&tree->root, node);
	/* printf("freed: %zu, %zu\n", free_size, (size_t)node % sizeof *node); */
}

void free_tree_clear(free_tree_t* tree)
{
	avl_node_t* root = tree->memory;
	memset(root, 0x00, sizeof *root);
	root->value = tree->size;
	tree->root = root;
}

void free_tree_serialize(free_tree_t* tree, void* destination)
{
	uint8_t* dest = destination;
	const size_t len = sizeof tree->root + sizeof tree->size;
	relative(tree);
	memcpy(dest, tree, len);
	memcpy(dest + len, tree->memory, tree->size);
	absolute(tree);
}

void free_tree_deserialize(free_tree_t* tree, void* source)
{
	uint8_t* src = source;
	if (tree->memory != NULL)
	{
		free(tree->memory);
	}
	const size_t len = sizeof tree->root + sizeof tree->size;
	memcpy(tree, src, len);
	tree->memory = malloc(tree->size);
	memcpy(tree->memory, src + len, tree->size);
	absolute(tree);
	/* printf("utilization: %f%%\n", 100.f * free_tree_utilization(tree)); */
}

float free_tree_utilization(free_tree_t* tree)
{
	if (tree->root == NULL)
	{
		return 1.0f;
	}
	else
	{
		float cum = avl_value_sum(tree->root);
		return 1.0f - cum / (float)tree->size;
	}
}

int free_tree_test(int argc, char** argv)
{
	free_tree_t* foo = alloca(sizeof *foo);
	free_tree_init(foo, 1024);
	char* lmao = free_tree_alloc(foo, 69);
	lmao = free_tree_realloc(foo, lmao, 79);
	lmao = free_tree_realloc(foo, lmao, 1);
	lmao = free_tree_realloc(foo, lmao, 512);
	free_tree_free(foo, lmao);
	lmao = free_tree_alloc(foo, 2048);
	alloc_header_t* h = lmao - sizeof(alloc_header_t);
	char* bruh = free_tree_alloc(foo, 8);
	h = bruh - sizeof(alloc_header_t);
	free_tree_free(foo, lmao);
	free_tree_free(foo, bruh);
	free_tree_clear(foo);
	return 0;
}
