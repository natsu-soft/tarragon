#include "free_tree.h"
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "avl_tree.h"
#include "error_macros.h"
#include "pointer_memes.h"
#include "scratch.h"
#include "benchmark.h"

static uint8_t verbose_resize = 0x01;

typedef struct alloc_header_t
{
	size_t size;
	size_t padding;
} alloc_header_t;

static const size_t align(const void* ptr, const size_t pow2)
{
	const size_t modulo = (size_t)ptr & (pow2 - 1);
	return modulo > 0 ? pow2 - modulo : 0;
}

static void free_tree_relative(free_tree_t* tree)
{
	tree->valid_root = tree->root != NULL;
	if (tree->valid_root)
	{
		/* avl_relative_ptrs(tree->root); */
		avl_relative_ptrs(tree->root);
		/* avl_relative_stack(tree->root); */
		point_relative_offset(&tree->root, tree->memory);
	}
}

static void free_tree_absolute(free_tree_t* tree)
{
	if (tree->valid_root)
	{
		/* printf("root0: %#x\n", (size_t)tree->root); */
		point_absolute_offset(&tree->root, tree->memory);
		/* printf("root1: %#x\n", (size_t)tree->root); */
		/* avl_absolute_ptrs(tree->root); */
		avl_absolute_ptrs(tree->root);
	}
}

void free_tree_init (free_tree_t* tree, const size_t bytes)
{
	assertf(bytes > sizeof(avl_node_t), "allocation is too small, must be atleast %zu!", sizeof(avl_node_t));
	tree->size = bytes;
	tree->memory = malloc(bytes);
	free_tree_clear(tree);
}

// fragmentation, but MOTHER OF ALL OMLETTES, JACK!
void free_tree_grow(free_tree_t* tree, const size_t bytes)
{
	assertf(bytes > tree->size, "its called GROW");
	const size_t size0 = tree->size;
	free_tree_relative(tree);
	uint8_t* memory = realloc(tree->memory, bytes);
	/* printf("[%#zx, %#zx)\n", memory, memory + bytes); */
	assertf(memory != NULL, "Desired allocation size: %zu or %zuGB", bytes, bytes / (1024 * 1024 * 1024));
	tree->memory = memory;
	free_tree_absolute(tree);
	const uint8_t* m1 = memory + bytes;
	const size_t delta = bytes - size0;
	tree->size = bytes;
	if (delta > sizeof(avl_node_t) + sizeof(size_t))
	{
		const uint8_t* ptr = memory + size0;
		const size_t shift = align(ptr, sizeof(size_t));
		avl_node_t* node = ptr + shift;
		memset(node, 0x00, sizeof *node);
		assert(memory <= node && node < m1);
		node->value = delta - shift;
		avl_insert(&tree->root, node);
	}
}

void* free_tree_alloc(free_tree_t* tree, const size_t bytes)
{
	const size_t header_size = sizeof(alloc_header_t);
	const size_t req_size = bytes +  sizeof(alloc_header_t);
	avl_node_t* node = avl_find_min_gte(tree->root, req_size);
	/* avl_node_t* node = avl_first_gte(tree->root, req_size); */
	if (node != NULL)
	{
		const size_t total = node->value;
		avl_remove(&tree->root, node);
		alloc_header_t* header = node;
		uint8_t* ptr = header + 1;
		header->size = bytes;
		header->padding = total - bytes - sizeof *header;
		/* if (total - bytes - padding - header_size > node_size) */
		if (header->padding > sizeof *node + sizeof(size_t))
		{
			const size_t shift = align(ptr + bytes, sizeof(size_t));
			node = ptr + bytes + shift;
			memset(node, 0x00, sizeof *node);
			node->value = header->padding - shift;
			header->padding = shift;
			avl_insert(&tree->root, node);
		}
		return ptr;
	}
	else
	{
		return NULL;
	}
}

void* free_tree_dealloc(free_tree_t* tree, void* ptr, const size_t bytes)
{
	assertf(ptr != NULL, "CANNOT dealloc a null pointer!");
	alloc_header_t* header = (alloc_header_t*)ptr - 1;
	const size_t size = header->size;
	const size_t padding = header->padding;
	assertf(bytes <= size, "CANNOT dealloc to a larger size T^T");
	header->padding += size - bytes;
	header->size = bytes;
	if (header->padding > sizeof(avl_node_t) + sizeof(size_t))
	{
		const size_t shift = align(ptr + bytes, sizeof(size_t));
		avl_node_t* node = (uint8_t*)ptr + bytes + shift;
		memset(node, 0x00, sizeof *node);
		node->value = header->padding - shift;
		avl_insert(&tree->root, node);
		header->padding = shift;
	}
	return ptr;
}

void free_tree_free(free_tree_t* tree, const void* ptr)
{
	const alloc_header_t* header = (alloc_header_t*)ptr - 1;
	const size_t free_size = header->size + header->padding + sizeof *header;
	avl_node_t* node = header;
	memset(node, 0x00, sizeof *node);
	node->value = free_size;
	avl_insert(&tree->root, node);
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
	free_tree_relative(tree);
	memcpy(dest, tree, len);
	memcpy(dest + len, tree->memory, tree->size);
	free_tree_absolute(tree);
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
	free_tree_absolute(tree);
	/* printf("utilization: %f%%\n", 100.f * free_tree_utilization(tree)); */
}

static int compare_address(const void* a, const void* b)
{
	return (size_t)(*(const avl_node_t**)a) - (size_t)(*(const avl_node_t**)b); // overflow? I barely know her
}

void free_tree_coalesce(free_tree_t* tree)
{
	if (tree->root != NULL)
	{
		const size_t num = avl_num_nodes(tree->root);
		const size_t tree_size = num * sizeof(avl_node_t);
		avl_node_t **list = scratch_alloc(num * sizeof *list);
		size_t i = 0;
		avl_populate_list(tree->root, list, &i);
		qsort(list, num, sizeof *list, compare_address);
		size_t swap = 0;
		const size_t merge_size = sizeof(size_t);
		for (i = swap + 1; i < num; ++i)
		{
			const size_t end_addr = (size_t)list[swap] + list[swap]->value;
			const size_t diff = (size_t)list[i] - end_addr;
			if (diff > merge_size)
			{
				swap++;
				list[swap] = list[i];
			}
			else
			{
				list[swap]->value += list[i]->value + diff;
			}
			/* if ((size_t)list[i] > merge_size + end_addr) */
			/* { */
			/* 	swap++; */
			/* } */
			/* else */
			/* { */
			/* 	list[swap]->value = (size_t)list[i] + list[i]->value - (size_t)list[swap]; */
			/* } */
		}
		tree->root = NULL;
		for (i = 0; i <= swap; ++i) // NOTE: <=
		{
			memset(list[i], 0x00, offsetof(avl_node_t, value));
			avl_insert(&tree->root, list[i]);
		}
		scratch_free(list);
	}
}

float free_tree_coverage(free_tree_t* tree)
{
	if (tree->root == NULL)
	{
		return 1.0f;
	}
	else
	{
		const float cum = avl_value_sum(tree->root);
		return  cum / (float)tree->size;
	}
}

int free_tree_test(int argc, char** argv)
{
	#define INIT_SIZE 1024
	verbose_resize = 0x00;
	free_tree_t* tree = alloca(sizeof *tree);
	free_tree_init(tree, INIT_SIZE);
	void* other = malloc(INIT_SIZE);
	/* printf("POST INIT\n"); */
	/* avl_print(tree->root, 0); */

	printf("  -- allocation: ");
	char* foo = free_tree_alloc(tree, 512);
	foo = free_tree_alloc(tree, 256);
	foo = free_tree_alloc(tree, 128);
	foo = free_tree_alloc(tree, 64);
	foo = free_tree_alloc(tree, 1);
	foo = free_tree_alloc(tree, 1);
	foo = free_tree_alloc(tree, 1);
	printf("GOOD!\n");
	/* printf("\n\nPOST ALLOCS\n"); */
	/* avl_print(tree->root, 0); */

	printf("  -- clear: ");
	free_tree_clear(tree);
	printf("GOOD!\n");

	printf("  -- free: ");
	foo = free_tree_alloc(tree, 512);
	char* bar = free_tree_alloc(tree, 256);
	free_tree_free(tree, bar);
	free_tree_free(tree, foo);
	printf("GOOD!\n");

	foo = free_tree_alloc(tree, 2048);
	assert(foo == NULL);
	free_tree_clear(tree);

	printf("  -- translation: ");
	memcpy(other, tree->memory, INIT_SIZE);
	free_tree_relative(tree);
	free(tree->memory);
	tree->memory = other;
	free_tree_absolute(tree);
	assert(tree->root->value == INIT_SIZE);
	printf("GOOD!\n");

	printf("  -- grow0: ");
	free_tree_grow(tree, 4096);

	foo = free_tree_alloc(tree, 2048);
	assert(foo != NULL);

	free_tree_clear(tree);
	assert(tree->root->value == 4096);
	printf("GOOD!\n");
	printf("  -- grow1: ");
	const char spinner[4] = { '\\', '-', '/', '|' };
	fflush(stdout);
	#define RESIZE_SCALE 1.618f
	#define RESIZE_OVERHEAD 4096
	const size_t LIMIT = 0x800000000;
	for (int i = 0; i <= 99999; ++i)
	{
		printf("[%c]", spinner[i % 4]);
		fflush(stdout);
		/* printf("%d: %zu\n", i, RESIZE_SCALE * tree->size + RESIZE_OVERHEAD); */
		const size_t new_size = RESIZE_SCALE * tree->size + RESIZE_OVERHEAD;
		if (new_size > LIMIT)
		{
			printf("\b\b\b");
			break;
		}
		free_tree_grow(tree, RESIZE_SCALE * tree->size + RESIZE_OVERHEAD);
		printf("\b\b\b");
		/* avl_print(tree->root, 0); */
	}
	printf("GOOD!\n");
	printf("  -- relative & absolute: ");
	fflush(stdout);
	free_tree_relative(tree);
	free_tree_absolute(tree);
	printf("GOOD!\n");
#define ALLOC_EXTREME 10000000
#ifdef ALLOC_EXTREME
	printf("  -- fragmenting: ");
	/* printf("working"); */
	fflush(stdout);
	for (unsigned int i = 0; i < ALLOC_EXTREME; ++i)
	{
		/* avl_print(tree->root, 0); */
		printf("%06.2f%%", 100.0f * (float)i / (float)ALLOC_EXTREME);
		/* printf("[%d]", i % 10); */
		fflush(stdout);
		void* ptr = free_tree_alloc(tree, 32);
		if (ptr == NULL)
		{
			printf("\b\b\b\b\b\b\b");
			break;
		}
		ptr = free_tree_dealloc(tree, ptr, 16);
		free_tree_free(tree, ptr);
		printf("\b\b\b\b\b\b\b");
	}
	/* printf("\b\b\b\b\b\b\b"); */
	printf("GOOD!    \n");
	if (tree->root)
	{
		printf("  >> tree depth: %zu\n", tree->root->depth);
		printf("  >> tree sum: %zu\n", avl_value_sum(tree->root));
		/* avl_print(tree->root, 0); */
	}
	else
	{
		printf("  >> 100%% ALLOCATED!\n");
	}
	/* printf("  >> fragmentation: %f%%\n", 100.f * (1.0f - free_tree_coverage(tree))); */
	printf("  -- relative & absolute: ");
	fflush(stdout);
	free_tree_relative(tree);
	free_tree_absolute(tree);
	printf("GOOD!\n");
	fflush(stdout);
	/* avl_print(tree->root, 0); */
#endif
	printf("  -- coalesce: ");
	fflush(stdout);
	free_tree_coalesce(tree);
	free_tree_relative(tree);
	free_tree_absolute(tree);
	printf("GOOD!\n");
	if (tree->root != NULL)
	{
		printf("  >> tree depth: %zu\n", tree->root->depth);
		printf("  >> tree sum: %zu\n", avl_value_sum(tree->root));
	}
	/* avl_print(tree->root, 0); */
/* #define EUROPEAN_EXTREME 16 */
#ifdef EUROPEAN_EXTREME
	printf("  >> tree size: %zu GB\n", tree->size / 1073741824u);
	printf("  -- EUROPEAN EXTREME: ");
	fflush(stdout);
	const size_t root_val = tree->root->value;
	const size_t tree_size = tree->size;
	for (int i = 0; i < EUROPEAN_EXTREME; ++i)
	{
		printf("[%c]", spinner[i % 4]);
		fflush(stdout);
		uint8_t* temp = malloc(tree_size);
		free_tree_relative(tree);
		memcpy(temp, tree->memory, tree_size);
		free(tree->memory);
		tree->memory = temp;
		free_tree_relative(tree);
		assert(tree->root->value == root_val);
		printf("\b\b\b");
		fflush(stdout);
	}
	printf("GOOD!\n");
#endif
	/* avl_print(tree->root, 0); */
	verbose_resize = 0x01;
	free(tree->memory);
	return 0;
}
