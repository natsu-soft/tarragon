#include "free_tree.h"
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "avl_tree.h"
#include "error_macros.h"
#include "pointer_memes.h"
#include "benchmark.h"

static uint8_t verbose_resize = 0x01;

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

static void free_tree_relative(free_tree_t* tree)
{
	tree->valid_root = tree->root != NULL;
	if (tree->valid_root)
	{
		/* avl_relative_ptrs(tree->root); */
		avl_relative_ptrs(tree->root);
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
	assertf(bytes > tree->size, "fuccu");
	free_tree_relative(tree);
	uint8_t* memory = realloc(tree->memory, bytes);
	/* printf("[%#zx, %#zx)\n", memory, memory + bytes); */
	assertf(memory != NULL, "Desired allocation size: %zu or %zuGB", bytes, bytes / (1024 * 1024 * 1024));
	/* if (verbose_resize && memory != tree->memory) */
	/* { */
	/* 	printf("NEW ALLOCATION: [%#zx, %#zx)\n", (size_t)memory, (size_t)memory + bytes); */
	/* } */
	const size_t size0 = tree->size;
	tree->size = bytes;
	tree->memory = memory;
	free_tree_absolute(tree);
	const size_t node_size = sizeof *tree->root;
	const size_t modulo = (size_t)(memory + size0) % node_size;
	const size_t padding = modulo > 0 ? node_size - modulo : 0;
	if (bytes + node_size > size0 + padding)
	{
		avl_node_t* node = memory + size0 + padding;
		memset(node, 0x00, sizeof *node);
		node->value = bytes - size0 - padding;
		avl_insert(&tree->root, node);
	}
}

void* free_tree_alloc(free_tree_t* tree, const size_t bytes)
{
	const size_t header_size = sizeof(alloc_header_t);
	avl_node_t* node = avl_find_min_gte(tree->root, bytes + 2 * header_size - 1); // 2x to account for any possible padding
	if (node != NULL)
	{
		/* printf("node size: %zu\n", node->value); */
		const size_t total = node->value;
		avl_remove(&tree->root, node);
		alloc_header_t* header = node;
		void* res = header + 1;
		header->size = bytes;
		header->padding = total - bytes - header_size;
		const size_t node_size = sizeof *node;
		const size_t modulo = ((size_t)res + bytes) % node_size; // NOT A POWER OF 2!
		const size_t padding = modulo > 0 ? node_size - modulo: 0;
		if (total - bytes - padding - header_size > node_size)
		{
			node = ((uint8_t*)res + bytes + padding);
			/* printf("alloc cleave: %#zx\n", node); */
			memset(node, 0x00, sizeof *node);
			node->value = total - bytes - padding - header_size;
			header->padding = padding;
			/* printf("alloc insert: %zu %zu\n", (size_t)node % spacing, node->value); */
			avl_insert(&tree->root, node);
		}
		return res;
	}
	else
	{
		/* printf("ALLOCATION FAILED: %zu!\n", bytes); */
		return NULL;
	/* 	const size_t m0 = tree->size; */
	/* 	const size_t m1 = RESIZE_SCALE * m0 + bytes + RESIZE_OVERHEAD; */
	/* 	/\* printf("m1: %zu\n", m1); *\/ */
	/* 	// relativize & realloc */
	/* 	/\* uint8_t* memory = tree->memory; *\/ */
	/* 	const uint8_t root_flag = tree->root != NULL ? 1 : 0; */
	/* 	size_t root_offset = 0; */
	/* 	if (root_flag) */
	/* 	{ */
	/* 		relative(tree); */
	/* 	} */
	/* 	/\* uint8_t* temp = alloca(m0); *\/ */
	/* 	/\* memcpy(temp, tree->memory, m0); *\/ */
	/* 	/\* free(tree->memory); *\/ */
	/* 	uint8_t* memory = realloc(tree->memory, m1); */
	/* 	/\* memcpy(memory, tree->memory, m0); *\/ */
	/* 	/\* free(tree->memory); *\/ */
	/* 	/\* memcpy(memory, temp, m0); *\/ */
	/* 	/\* free(temp); *\/ */
	/* 	/\* uint8_t* memory = realloc(tree->memory, m1); *\/ */
	/* 	if (root_flag) */
	/* 	{ */
	/* 		absolute(tree); */
	/* 		avl_node_t* root = tree->root; */
	/* 		if (root->left == NULL && root->right == NULL) */
	/* 		{ */
	/* 			root->value = m1; */
	/* 			goto retry; */
	/* 		} */
	/* 	} */
	/* 	// YES, there are SOME leaks--MOTHER OF ALL OMLETTES, JACK! */
	/* 	const size_t align = sizeof(avl_node_t); */
	/* 	const size_t modulo = ((size_t)memory + m0) & (align - 1); */
	/* 	const size_t padding = modulo > 0 ? align - modulo : 0; */
	/* 	node = memory + m0 + padding; */
	/* 	memset(node, 0x00, sizeof *node); */
	/* 	node->value = m1 - m0 - padding; */
	/* 	avl_insert(&tree->root, node); */
	/* retry: */
	/* 	tree->size = m1; */
	/* 	tree->memory = memory; */
	/* 	return free_tree_alloc(tree, bytes); */
	}
}

// for arrays
// HAD to do it to maboi ;_;
/* void* free_tree_realloc(free_tree_t* tree, void* ptr, const size_t bytes) */
/* { */
/* 	assertf(ptr != NULL, "lmao u dingus"); */
/* 	if (ptr != NULL) */
/* 	{ */
/* 		alloc_header_t* header = ((alloc_header_t*)ptr) - 1; */
/* 		const size_t size = header->size; */
/* 		const size_t padding = header->padding; */
/* 		if (bytes > size + padding) */
/* 		{ */
/* 			// IF ONLY ;_; */
/* 			/\* uint8_t* temp = alloca(size); *\/ */
/* 			/\* memcpy(temp, ptr, size); *\/ */
/* 			/\* free_tree_free(tree, ptr); *\/ */
/* 			/\* ptr = free_tree_alloc(tree, bytes); *\/ */
/* 			/\* if (ptr != NULL) *\/ */
/* 			/\* { *\/ */
/* 			/\* 	memcpy(ptr, temp, size); *\/ */
/* 			/\* } *\/ */
/* 			return NULL; */
/* 		} */
/* 		else if (bytes >= size) // size <= bytes <= size + padding */
/* 		{ */
/* 			return NULL; */
/* 			/\* header->padding -= bytes - size; *\/ */
/* 			/\* header->size = bytes; *\/ */
/* 		} */
/* 		else // bytes < size */
/* 		{ */
/* 			header->padding += size - bytes; */
/* 			header->size = bytes; */
/* 			const size_t align = sizeof(avl_node_t); */
/* 			if (header->padding >= align) */
/* 			{ */
/* 				size_t modulo = ((size_t)ptr + bytes) % (align - 1); */
/* 				modulo = modulo > 0 ? align - modulo : 0; */
/* 				avl_node_t* node = (uint8_t*)ptr + bytes + modulo; */
/* 				memset(node, 0x00, align); */
/* 				node->value = padding - align; */
/* 				avl_insert(&tree->root, node); */
/* 				header->padding -= node->value; */
/* 			} */
/* 		} */
/* 		return ptr; */
/* 	} */
/* 	else */
/* 	{ */
/* 		return NULL; */
/* 		/\* return free_tree_alloc(tree, bytes); *\/ */
/* 	} */
/* } */

void* free_tree_dealloc(free_tree_t* tree, void* ptr, const size_t bytes)
{
	assertf(ptr != NULL, "CANNOT dealloc a null pointer!");
	alloc_header_t* header = ((alloc_header_t*)ptr) - 1;
	const size_t size = header->size;
	const size_t padding = header->padding;
	assertf(bytes < size + padding, "CANNOT dealloc to a larger size T^T");
	header->padding += size - bytes;
	header->size = bytes;
	const size_t node_size = sizeof(avl_node_t);
	if (header->padding >= node_size)
	{
		// HAVE TO USE % SINCE spacing IS NOT A POWER OF 2
		size_t modulo = ((size_t)ptr + bytes) % node_size;
		modulo = modulo > 0 ? node_size - modulo : 0;
		avl_node_t* node = (uint8_t*)ptr + bytes + modulo;
		memset(node, 0x00, node_size);
		node->value = header->padding - node_size;
		/* printf("inserting node: %zu\n", node->value); */
		avl_insert(&tree->root, node);
		header->padding = modulo;
	}
	return ptr;
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
	/* printf("\n\nPOST CLEAR\n"); */
	/* avl_print(tree->root, 0); */

	foo = free_tree_alloc(tree, 512);
	char* bar = free_tree_alloc(tree, 256);
	printf("  -- free: ");
	free_tree_free(tree, bar);
	free_tree_free(tree, foo);
	printf("GOOD!\n");
	/* printf("\n\nPOST FREE\n"); */
	/* avl_print(tree->root, 0); */

	foo = free_tree_alloc(tree, 2048);
	assert(foo == NULL);
	free_tree_clear(tree);
	/* printf("\n\nPOST CLEAR\n"); */
	/* avl_print(tree->root, 0); */

	/* printf("\n"); */
	/* printf("  -- translation: "); */
	/* memcpy(other, tree->memory, INIT_SIZE); */
	/* /\* printf("root0: %zu\n", (size_t)tree->root); *\/ */
	/* free_tree_relative(tree); */
	/* /\* printf("root1: %zu\n", (size_t)tree->root); *\/ */
	/* free(tree->memory); */
	/* tree->memory = other; */
	/* free_tree_absolute(tree); */
	/* assert(tree->root->value == INIT_SIZE); */
	/* printf("GOOD!\n"); */
	/* printf("root2: %zu\n", (size_t)tree->root); */
	/* printf("\n\nPOST MOVE\n"); */
	/* avl_print(tree->root, 0); */

	printf("  -- grow(ez): ");
	free_tree_grow(tree, 4096);
	/* printf("\n\nPOST GROW\n"); */
	/* avl_print(tree->root, 0); */

	foo = free_tree_alloc(tree, 2048);
	/* printf("\n\nPOST ALLOC\n"); */
	/* avl_print(tree->root, 0); */
	assert(foo != NULL);

	free_tree_clear(tree);
	assert(tree->root->value == 4096);
	printf("GOOD!\n");
	/* printf("\n\nPOST CLEAR\n"); */
	/* avl_print(tree->root, 0); */
	printf("  -- grow (HARD): ");
	fflush(stdout);
	#define RESIZE_SCALE 1.618
	#define RESIZE_OVERHEAD 1024
	const size_t LIMIT = 0x200000000;
	for (int i = 0; i <= 9999; ++i)
	{
		/* printf("%d: %zu\n", i, RESIZE_SCALE * tree->size + RESIZE_OVERHEAD); */
		const size_t new_size = RESIZE_SCALE * tree->size + RESIZE_OVERHEAD;
		if (new_size > LIMIT)
		{
			/* printf("!!! HOT DAMN !!! (break before attempting to allocate > 8GB at iteration %d)\n", i); */
			break;
		}
		free_tree_grow(tree, RESIZE_SCALE * tree->size + RESIZE_OVERHEAD);
		/* printf("\n----  %d ----\n\n", i); */
		/* avl_print(tree->root, 0); */
	}
	printf("GOOD!\n");
	const char spinner[4] = { '\\', '-', '/', '|' };
#define ALLOC_EXTREME 99999
#ifdef ALLOC_EXTREME
	printf("  -- extreme alloc, dealloc, & free: ");
	free_tree_clear(tree);
	printf("working");
	fflush(stdout);
	for (unsigned int i = 0; i < ALLOC_EXTREME; ++i)
	{
		void* ptr = free_tree_alloc(tree, 32);
		if (ptr == NULL)
		{
			break;
		}
		ptr = free_tree_dealloc(tree, ptr, 16);
		free_tree_free(tree, ptr);
	}
	printf("\b\b\b\b\b\b\b");
	printf("GOOD!\n");
	printf("  >> fragmentation: %f%%\n", 100.f * (1.0f - free_tree_coverage(tree)));
	/* free_tree_relative(tree); */
	/* free_tree_absolute(tree); */
#endif
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
