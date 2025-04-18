#include "array_trie.h"
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "error_macros.h"
#include "free_tree.h"
#include "arena.h"
#include "file_sys.h"
#include "mem_sys.h"
#include "pointer_memes.h"

#define RESIZE_SCALE 1.618
#define RESIZE_OVERHEAD 4096

static void trie_traverse_relative(trie_node_t* node)
{
	for (int i = 0; i < node->num_edges; ++i)
	{
		const trie_edge_t* edge = node->edges + i;
		const char c = edge->c;
		if (c != '\0')
		{
			trie_traverse_relative(edge->token);
			point_relative_offset(&edge->token, node);
		}
		else
		{
			// TODO
		}
	}
	point_relative(&node->edges);
}

static void trie_traverse_absolute(trie_node_t* node)
{
	point_absolute(&node->edges);
	for (int i = 0; i < node->num_edges; ++i)
	{
		const trie_edge_t* edge = node->edges + i;
		const char c = edge->c;
		if (c != '\0')
		{
			point_absolute_offset(&edge->token, node);
			trie_traverse_absolute(edge->token);
		}
		else
		{
			// TODO
		}
	}
}

static void trie_relative_ptrs(trie_t* trie)
{
	trie_traverse_relative(trie->root);
	point_relative_offset(&trie->root, trie->support->memory);
}

static void trie_absolute_ptrs(trie_t* trie)
{
	point_absolute_offset(&trie->root, trie->support->memory);
	trie_traverse_absolute(trie->root);
}

static void* trie_alloc(trie_t* trie, const size_t size, void** work_ptr)
{
	free_tree_t* free_tree = trie->support;
	void* res = free_tree_alloc(free_tree, size);
	uint8_t work_flag = work_ptr != NULL;
	if (res == NULL)
	{
		/* printf(">> RESIZING!!!\n"); */

		trie_relative_ptrs(trie);
		if (work_flag)
		{
			point_relative_offset(work_ptr, free_tree->memory);
		}
		do
		{
			free_tree_grow(free_tree, RESIZE_SCALE * free_tree->size + RESIZE_OVERHEAD);
			res = free_tree_alloc(free_tree, size);
		}
		while (res == NULL);
		/* printf(">> POST-RESIZE RELATIVE ROOT: %zu\n", (size_t)trie->root); */
		trie_absolute_ptrs(trie);
		if (work_flag)
		{
			point_absolute_offset(work_ptr, free_tree->memory);
		}
	}
	return res;
}

void trie_add_token(trie_t* trie, char* str, void* token)
{
	assert(trie);
	assert(trie->support);
	if (str == NULL)
	{
		return;
	}
	free_tree_t* free_tree = trie->support;
	trie_node_t* node = trie->root;
	const size_t node_size = sizeof *node;
	if (node == NULL)
	{
		trie->root = trie_alloc(trie, node_size, NULL);
		node = trie->root;
		memset(trie->root, 0x00, node_size);
		/* printf("\n>> INITIAL RELATIVE ROOT: %ld\n", (size_t)trie->root - (size_t)free_tree->memory); */
	}
	const size_t edge_size = sizeof *node->edges;
	for (int i = 0; i < strlen(str) + 1; ++i)
	{
		const char c = str[i];
		/* printf("c: %c\n", c); */
		const int n = node->num_edges;
		for (int j = 0; j < n; ++j)
		{
			const trie_edge_t* edge = node->edges + j;
			if (c == edge->c)
			{
				node = edge->token;
				goto next_char;
			}
		}
		trie_edge_t* edge = trie_alloc(trie, (n + 1) * edge_size, &node);
		if (n > 0)
		{
			memcpy(edge, node->edges, n * node_size); // if this segfaults im SCREAMING
			free_tree_free(free_tree, node->edges);
		}
		node->num_edges = n + 1;
		node->edges = edge;
		edge = edge + n;
		edge->c = '\0'; // stops invalid traversal in below trie_alloc()
		if (c != '\0')
		{
			const char c0 = c0;
			node = trie_alloc(trie, node_size, &edge);
			edge->c = c;
			memset(node, 0x00, node_size);
			edge->token = node;
		}
		else
		{
			edge->token = token;
		}
	next_char: /* :^) */;
	}
}

void* trie_get_token(trie_t* trie, char* str)
{
	assert(trie);
	assert(trie->root);
	if (str == NULL)
	{
		return NULL;
	}
	trie_node_t* node = trie->root;
	for (int i = 0; i < strlen(str) + 1; ++i)
	{
		const char c = str[i];
		for (int j = 0; j < node->num_edges; ++j)
		{
			const trie_edge_t* edge = node->edges + j;
			if (c == edge->c)
			{
				if (c != '\0')
				{
					node = edge->token;
					goto next_char;
				}
				else
				{
					return edge->token;
				}
			}
		}
		return NULL;
	next_char: /* :^) */;
	}
	return NULL;
}

uint8_t trie_remove_token(trie_t* trie, char* str)
{
	assert(trie);
	assert(trie->root);
	trie_node_t* node = trie->root;
	free_tree_t* free_tree = trie->support;
	for (int i = 0; i < strlen(str) + 1; ++i)
	{
		const char c = str[i];
		for (int j = 0; j < node->num_edges; ++j)
		{
			const trie_edge_t* edge = node->edges + j;
			if (c == edge->c)
			{
				if (c != '\0')
				{
					node = edge->token;
					goto next_char;
				}
				else
				{
					const int n = node->num_edges;
					const size_t edge_size = sizeof *edge;
					memmove(node->edges + j, node->edges + j + 1, (n - j -1) * edge_size);
					node->edges = free_tree_dealloc(free_tree, node->edges, edge_size);
					node->num_edges--;
					return 1;
				}
			}
		}
		return 0;
	next_char: /* :^) */;
	}
	return 0;
}

void num_leaves(const trie_node_t* node, uint32_t* cum)
{
	for (int i = 0; i < node->num_edges; ++i)
	{
		const trie_edge_t* edge = node->edges + i;
		switch (edge->c)
		{
		case '\0':
			*cum += 1;
			break;
		default:
			num_leaves(edge->token, cum);
		}
	}
}

uint32_t trie_num_leaves(const trie_t* trie)
{
	uint32_t cum = 0;
	if (trie != NULL && trie->root != NULL)
	{
		num_leaves(trie->root, &cum);
	}
	return cum;
}

void subtrie_leaf_strings(const trie_node_t* node, const char* prefix, const uint32_t prefix_len, arena_t* str_arena, arena_t* char_arena)
{
	for (int i = 0; i < node->num_edges; ++i)
	{
		const trie_edge_t* edge= node->edges + i;
		const char c = edge->c;
		const uint32_t n = prefix_len + 1;
		char* str = alloca(n);
		memcpy(str, prefix, prefix_len);
		str[prefix_len] = c;
		if (c != '\0')
		{
			subtrie_leaf_strings(edge->token, str, n, str_arena, char_arena);
		}
		else
		{
			if (char_arena->size + n > char_arena->cap)
			{
				const size_t char_offset = (size_t)char_arena->memory - (size_t)str_arena->memory;
				for (size_t i = 0; i < str_arena->size; i += sizeof(char*))
				{
					char* temp = str_arena->memory + i;
					temp = (void*)((size_t)temp - (size_t)str_arena->memory);
					memcpy(str_arena->memory + i, temp, sizeof(char*));
				}
				str_arena->memory = realloc(str_arena->memory, str_arena->cap + char_arena->size + n + RESIZE_OVERHEAD);
				for (size_t i = 0; i < str_arena->size; i += sizeof(char*))
				{
					char* temp = str_arena->memory + i;
					temp = (void*)((size_t)temp + (size_t)str_arena->memory);
					memcpy(str_arena->memory + i, temp, sizeof(char*));
				}
				char_arena->memory = str_arena->memory + char_offset;
				char_arena->cap = char_arena->size + n + RESIZE_OVERHEAD;
			}
			char* leaf_str = arena_alloc(char_arena, n);
			memcpy(leaf_str, str, n);
			char** string = arena_alloc(str_arena, sizeof *string);
			*string = leaf_str;
		}
	}
}

trie_node_t* get_subtrie(trie_node_t* node, const char* prefix)
{
	const int n = strlen(prefix);
	for (int i = 0; i < n; ++i)
	{
		const char c = prefix[i];
		for (int j = 0; j < node->num_edges; ++j)
		{
			const trie_edge_t* edge = node->edges + j;
			if (c == edge->c)
			{
				node = edge->token;
				goto found_node;
			}
		}
		return NULL;
	found_node:
		if (i == n - 1)
		{
			return node;
		}
	}
	return NULL;
}

// GOD I LOVE ARENA MEMES
char** trie_candidates(const trie_t* trie, arena_t* arena, const char* prefix, uint32_t* num_candidates)
{
	assert(trie);
	assert(trie->root);
	assert(arena);
	*num_candidates = 0;
	const trie_node_t* subtrie = prefix != NULL ? get_subtrie(trie->root, prefix) : trie->root;
	num_leaves(subtrie, num_candidates);
	const int n = *num_candidates;
	const size_t prefix_len = prefix != NULL ? strlen(prefix) : 0;
	const size_t arena_size = n * (sizeof(char*) + prefix_len + 16);
	if (arena->cap < arena_size)
	{
		arena->memory = realloc(arena->memory, arena_size + RESIZE_OVERHEAD);
	}
	arena->size = 0;
	arena_t str_arena =
	{
		.cap = n * sizeof(char*),
		.memory = arena->memory
	};
	arena_t char_arena =
	{
		.cap = arena_size - n * sizeof(char*),
		.memory = arena->memory + n * sizeof(char*)
	};
	subtrie_leaf_strings(subtrie, prefix, prefix_len, &str_arena, &char_arena);
	return (char**)str_arena.memory;
}

void trie_serialize(trie_t* trie, char* filename)
{
	assert(trie);
	free_tree_t* free_tree = trie->support;
	const size_t len = sizeof trie->root + sizeof free_tree->root + sizeof free_tree->size + free_tree->size;
	trie_relative_ptrs(trie);
	filemap_t filemap = {0};
	uint8_t* map = ms_map_file2(filename, len, &filemap);
	memcpy(map, &trie->root, sizeof trie->root);
	free_tree_serialize(free_tree, map + sizeof trie->root);
	ms_munmap_file2(&filemap);
	trie_absolute_ptrs(trie);
}

void trie_deserialize(trie_t* trie, char* filename)
{
	assert(trie);
	assert(trie->support == NULL);
	trie->support = malloc(sizeof *trie->support);
	memset(trie->support, 0x00, sizeof *trie->support);
	const size_t len = fs_size(filename);
	filemap_t filemap = {0};
	uint8_t* map = ms_map_file2(filename, len, &filemap);
	memcpy(&trie->root, map, sizeof trie->root);
	free_tree_deserialize(trie->support, map + sizeof trie->root);
	const free_tree_t* free_tree = trie->support;
	ms_munmap_file2(&filemap);
	trie_absolute_ptrs(trie);
}

static void dump_dot(FILE* dest, trie_node_t* node, uint32_t* id)
{
	const uint32_t node_id = *id;
	for (int i = 0; i < node->num_edges; ++i)
	{
		const trie_edge_t* edge = node->edges + i;
		if (edge->c != '\0')
		{
			*id += 1;
			fprintf(dest, "\t%d[label=\"%c\"];\n", *id, edge->c);
			fprintf(dest, "\t%d -- %d\n", node_id, *id);
			dump_dot(dest, edge->token, id);
		}
	}
}

void trie_dump_dot(FILE* dest, trie_t* trie, const char* root_label)
{
	assert(dest != NULL);
	assert(trie != NULL);
	fprintf(dest, "strict graph {\n");
	fprintf(dest, "\t0[label=\"%s\"];\n", root_label);
	uint32_t id = 0;
	dump_dot (dest, trie->root, &id);
	fprintf(dest, "}\n");
}

int array_trie_test(int argc, char** argv)
{
	/* printf("testing trie behavior\n"); */
	/* printf("sizeof(trie_t): %lu\nsizeof(trie_node_t): %lu\nsizeof(trie_edge_t): %lu\n", sizeof(trie_t), sizeof(trie_node_t), sizeof(trie_edge_t)); */
	FILE* fp = fopen("test_stuff/fruits.txt", "r");
	free_tree_t free_tree = {};
	free_tree_init(&free_tree, 4096);
	trie_t fruit_trie =
	{
		.support = &free_tree
	};
	size_t lmao = 69;
	char fruit[lmao];
	printf("  -- insertion: ");
	while (fs_getline(fruit, lmao, fp) > 0)
	{
		trie_add_token(&fruit_trie, fruit, NULL);
	}
	printf("GOOD!\n");
	fflush(fp);
	fclose(fp);
	/* THIS NEEDS TO WORK */
	const uint8_t cache_num = fruit_trie.root->num_edges;
	printf("  -- translation: ");
	trie_serialize(&fruit_trie, "fruit_fmap.bin");
	trie_t io_trie = {};
	trie_deserialize(&io_trie, "fruit_fmap.bin");
	assert(io_trie.root->num_edges == cache_num);
	printf("GOOD!\n");
	printf("  -- dot: ");
	FILE* buf = fopen("fruits.dot", "w");
	if (buf != NULL)
	{
		trie_dump_dot(buf, &fruit_trie, "WE OUT HERE");
		fflush(buf);
		fclose(buf);
	}
	printf("GOOD!\n");
	printf("  -- candidate: ");
	uint32_t num_fruits = 0;
	arena_t candidate_arena = {};
	arena_init(&candidate_arena, 512);
	char** all_fruits = trie_candidates(&io_trie, &candidate_arena, NULL, &num_fruits);
	/* for (int i = 0; i < num_fruits; ++i) */
	/* { */
	/* 	printf("%d: %s\n", i, all_fruits[i]); */
	/* } */
	assert(num_fruits == 51);
	assert(strcmp(all_fruits[0], "Apple") == 0);
	assert(strcmp(all_fruits[50], "Yuzu") == 0);
	printf("GOOD!\n");
	printf("> using %f%% of %zu bytes\n", 100.f * (1.0f - free_tree_coverage(&free_tree)), free_tree.size);
	printf("  -- removal: ");
	assert(trie_remove_token(&fruit_trie, "Yuzu"));
	assert(trie_remove_token(&fruit_trie, "Yuzu") == 0);
	printf("GOOD!\n");
	free(candidate_arena.memory);
	free(io_trie.support);
	free(free_tree.memory);
	return 0;
}
