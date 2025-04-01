#include "free_rb.h"
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define RESIZE_SCALE 1.618
#define RESIZE_OVERHEAD 4096

struct addr_size
{
	size_t addr;
	size_t size;
};

static uint32_t count_nodes (free_node_t* node)
{
	uint32_t sum = 1;
	if (node->lhs != NULL)
	{
		sum += count_nodes(node->lhs);
	}
	if (node->rhs != NULL)
	{
		sum += count_nodes(node->rhs);
	}
	return sum;
}

static void insert_allocation(free_node_t** root, size_t size)
{

}

static void insert(free_node_t** root, free_node_t* node)
{
}

static void populate_array(struct addr_size* dest, free_node_t* node, uint32_t* index)
{
	int i = *index;
	dest[i].addr = (size_t)node;
	dest[i].size = node->size;
	if (node->lhs != NULL)
	{
		*index = ++i;
		populate_array(dest, node->lhs, index);
	}
	if (node->rhs != NULL)
	{
		*index = ++i;
		populate_array(dest, node->rhs, index);
	}
}

static free_node_t* find_node(free_rb_t* rb, size_t bytes)
{

}

static void* alloc_first (free_rb_t* rb, size_t bytes)
{
	if (rb->root == NULL)
	{
	}
	return NULL;
}

static void* alloc_best(free_rb_t* rb, size_t bytes)
{
	return NULL;
}

void free_rb_init(free_rb_t* rb, size_t bytes)
{
	rb->memory = malloc(bytes);
	rb->cap = bytes;
	free_rb_free_all(rb);
}

void* free_rb_alloc(free_rb_t* rb, size_t bytes)
{
	void* ptr = rb->policies | BEST_FIT ? alloc_best(rb, bytes) : alloc_first(rb, bytes);
	if (ptr == NULL && rb->policies | RESIZEABLE)
	{
		void* tmp = realloc(rb->memory, RESIZE_SCALE * rb->size + RESIZE_OVERHEAD + bytes);
		assert(tmp);
		rb->memory = tmp;
		ptr = rb->policies | BEST_FIT ? alloc_best(rb, bytes) : alloc_first(rb, bytes);
	}
	return ptr;
}

void free_rb_free(free_rb_t* rb, void* addr)
{
	// find the appropriate header
	alloc_header_t* header = (uint8_t*)addr - sizeof *header;
	// turn all that back into a node
	free_node_t* node = header;
	node->size = header->size + header->padding;
	node->parent = NULL;
	node->lhs = NULL;
	node->rhs = NULL;
	// insert it into the tree
	insert(&rb->root, node);
	free_rb_coalesce(rb);
}

void free_rb_free_all(free_rb_t* rb)
{
	rb->size = 0;
	free_node_t* root = rb->memory;
	root->size = rb->cap;
	root->parent = NULL;
	root->lhs = NULL;
	root->rhs = NULL;
}

static int comp_addr(const void* a, const void* b)
{
	return (int64_t)((struct addr_size*)a)->addr - (int64_t)((struct addr_size*)b)->addr;
}

void free_rb_coalesce(free_rb_t* rb)
{
	uint32_t num_nodes = count_nodes(rb->root);
	struct addr_size nodes[num_nodes];
	populate_array(nodes, rb->root, 0);
	qsort(nodes, num_nodes, sizeof *nodes, comp_addr);
	uint32_t swap = 0;
	for (int i = 1; i < num_nodes; ++i)
	{
		struct addr_size n0 = nodes[swap];
		struct addr_size n1 = nodes[i];
		if (n0.addr + n0.size < n1.addr)
		{
			swap++;
			nodes[swap] = nodes[i];
		}
		else
		{
			nodes[swap].size += n1.size;
		}
	}
	num_nodes = swap + 1;
	free_rb_free_all(rb);
	for (int i = 0; i < num_nodes; ++i)
	{
	}
}
