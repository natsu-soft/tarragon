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

#define ARENA_RESIZE_OVERHEAD 2048


void trie_add_token(trie_t* trie, char* str, void* token)
{
	assert(trie);
	assert(trie->support);
	if (str == NULL)
	{
		return;
	}
	free_tree_t* free_tree = trie->support;
	if (trie->root == NULL)
	{
		trie->root = free_tree_alloc(free_tree, sizeof *trie->root);
		memset(trie->root, 0x00, sizeof *trie->root);
		printf("WHAT: %ld\n", (size_t)trie->root - (size_t)free_tree->memory);
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
				node = edge->token;
				goto next_char;
			}
		}
		const int n = node->num_edges;
		node->edges = free_tree_realloc(free_tree, node->edges, (n + 1) * sizeof *node->edges);
		node->num_edges++;
		trie_edge_t* edge = node->edges + n;
		edge->c = c;
		if (c != '\0')
		{
			node = free_tree_alloc(free_tree, sizeof *node);
			memset(node, 0x00, sizeof *node);
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
					trie_edge_t* temp = alloca((n - 1) * sizeof *edge);
					memcpy(temp, node->edges, j * sizeof *edge);
					memcpy(temp + j, node->edges + j + 1, (n - j -1) * sizeof *edge);
					node->edges = free_tree_realloc(trie->support, node->edges, (n - 1) * sizeof *edge);
					memcpy(node->edges, temp, (n - 1) * sizeof *edge);
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
				str_arena->memory = realloc(str_arena->memory, str_arena->cap + char_arena->size + n + ARENA_RESIZE_OVERHEAD);
				for (size_t i = 0; i < str_arena->size; i += sizeof(char*))
				{
					char* temp = str_arena->memory + i;
					temp = (void*)((size_t)temp + (size_t)str_arena->memory);
					memcpy(str_arena->memory + i, temp, sizeof(char*));
				}
				char_arena->memory = str_arena->memory + char_offset;
				char_arena->cap = char_arena->size + n + ARENA_RESIZE_OVERHEAD;
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
		free(arena->memory);
		arena->memory = malloc(arena_size + ARENA_RESIZE_OVERHEAD);
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

static void relative_ptrs(trie_node_t* node, const void* base)
{
	for (int i = 0; i < node->num_edges; ++i)
	{
		trie_edge_t* edge = node->edges + i;
		if (edge->c != '\0')
		{
			relative_ptrs(edge->token, base);
		}
		if (edge->token != NULL)
		{
			point_relative(&edge->token);
		}
		/* edge->token = (void*)((size_t)edge->token - (size_t)&edge->token); */
	}
	point_relative(&node->edges);
	/* node->edges = (void*)((size_t)node->edges - (size_t)&node->edges); */
}

static void absolute_ptrs(trie_node_t* node, const void* base)
{
	/* assertf(node != NULL, "B R U H"); */
	point_absolute(&node->edges);
	/* printf("num_edges: %d\n", node->num_edges); */
	/* node->edges = (void*)((size_t)node->edges + (size_t)&node->edges); */
	for (int i = 0; i < node->num_edges; ++i)
	{
		trie_edge_t* edge = node->edges + i;
		if (edge->token != NULL)
		{
			point_absolute(&edge->token);
		}
		/* edge->token = (void*)((size_t)edge->token + (size_t)&edge->token); */
		if (edge->c != '\0')
		{
			absolute_ptrs(edge->token, base);
		}
	}
}

void trie_serialize(trie_t* trie, char* filename)
{
	assert(trie);
	printf("edges0: %zu\n", trie->root->num_edges);
	free_tree_t* support = trie->support;
	const size_t len = sizeof trie->root + sizeof support->root + sizeof support->size + support->size;
	relative_ptrs(trie->root, support->memory);
	printf("absolute trie root0: %zu\n", (size_t)trie->root);
	/* point_relative_to(&trie->root, trie->support->memory); */
	point_relative_to(&trie->root, support->memory);
	printf("support0: %zu\n", (size_t)support->memory);
	printf("relative trie root0: %ld\n", (int64_t)trie->root);
	filemap_t filemap = {0};
	uint8_t* map = ms_map_file2(filename, len, &filemap);
	memcpy(map, &trie->root, sizeof trie->root);
	free_tree_serialize(trie->support, map + sizeof trie->root);
	ms_munmap_file2(&filemap);
	point_absolute_to(&trie->root, trie->support->memory);
	/* memcpy(trie->root, temp, sizeof *trie->root); */
	printf("edges1: %zu\n", trie->root->num_edges);
	absolute_ptrs(trie->root, trie->support->memory);
	printf("absolute trie root1: %zu\n", (size_t)trie->root);
	/* printf("util0: %f%%\n", 100.0f * free_tree_utilization(trie->memory)); */
}

void trie_deserialize(trie_t* trie, char* filename)
{
	assert(trie);
	assert(trie->support == NULL);
	trie->support = malloc(sizeof *trie->support);
	memset(trie->support, 0x00, sizeof *trie->support);
	const size_t len = fs_size(filename);
	/* free_tree_init(trie->memory, len - sizeof trie->root); */
	filemap_t filemap = {0};
	uint8_t* map = ms_map_file2(filename, len, &filemap);
	memcpy(&trie->root, map, sizeof trie->root);
	free_tree_deserialize(trie->support, map + sizeof trie->root);
	free_tree_t* support = trie->support;
	printf("support1: %zu\n", (size_t)support->memory);
	/* free_tree_t* support = trie->support; */
	printf("relative trie root1: %ld\n", (int64_t)trie->root);
	ms_munmap_file2(&filemap);
	point_absolute_to(&trie->root, trie->support->memory);
	printf("absolute trie root2: %zu\n", (size_t)trie->root);
	/* printf("lmao: %zx\n", (int32_t)trie->root->edges); */
	printf("edges2: %zu\n", trie->root->num_edges);
	absolute_ptrs(trie->root, trie->support->memory);
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
	printf("testing trie behavior\n");
	/* printf("sizeof(trie_t): %lu\nsizeof(trie_node_t): %lu\nsizeof(trie_edge_t): %lu\n", sizeof(trie_t), sizeof(trie_node_t), sizeof(trie_edge_t)); */
	FILE* fp = fopen("test_stuff/fruits.txt", "r");
	free_tree_t free_tree = {};
	free_tree_init(&free_tree, 1024);
	trie_t fruit_trie =
	{
		.support = &free_tree
	};
	size_t lmao = 69;
	char fruit[lmao];
	while (fs_getline(fruit, lmao, fp) > 0)
	{
		trie_add_token(&fruit_trie, fruit, NULL);
	}
	fflush(fp);
	fclose(fp);
	printf("WHAT2: %ld\n", (size_t)fruit_trie.root - (size_t)fruit_trie.support->memory);
	/* THIS NEEDS TO WORK */
	printf("serialization test\n");
	trie_serialize(&fruit_trie, "fruit_fmap.bin");
	trie_t io_trie = {};
	printf("deserialization test\n");
	trie_deserialize(&io_trie, "fruit_fmap.bin");
	printf("dot test\n");
	FILE* buf = fopen("fruits.dot", "w");
	if (buf != NULL)
	{
		trie_dump_dot(buf, &fruit_trie, "WE OUT HERE");
		fflush(buf);
		fclose(buf);
	}
	printf("candidate test\n");
	uint32_t num_fruits = 0;
	arena_t candidate_arena = {};
	arena_init(&candidate_arena, 512);
	char** all_fruits = trie_candidates(&fruit_trie, &candidate_arena, NULL, &num_fruits);
	for (int i = 0; i < num_fruits; ++i)
	{
		printf("%d: %s\n", i, all_fruits[i]);
	}
	printf("> using %f%% of %zu bytes\n", 100.f * free_tree_utilization(&free_tree), free_tree.size);
	printf("removal test\n");
	assert(trie_remove_token(&fruit_trie, "Yuzu"));
	assert(trie_remove_token(&fruit_trie, "Yuzu") == 0);
	free(candidate_arena.memory);
	/* free(io_trie.memory); */
	free(free_tree.memory);
	return 0;
}
