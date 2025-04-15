#include "array_trie.h"
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "error_macros.h"
#include "free_tree.h"
#include "arena.h"
#include "file_sys.h"
#include "mem_sys.h"

#define ARENA_RESIZE_OVERHEAD 1024

void trie_add_token(trie_t* trie, char* str, void* token)
{
	assert(trie);
	assert(trie->memory);
	if (str == NULL)
	{
		return;
	}
	free_tree_t* free_tree = trie->memory;
	if (trie->root == NULL)
	{
		trie->root = free_tree_alloc(free_tree, sizeof *trie->root);
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
		trie_edge_t* edge = alloca(sizeof * edge);
		edge->c = c;
		const int n = node->num_edges;
		node->edges = free_tree_realloc(free_tree, node->edges, (n + 1) * sizeof *edge);
		memcpy(node->edges + n, edge, sizeof *edge);
		node->num_edges++;
		edge = node->edges + n;
		if (c != '\0')
		{
			node = free_tree_alloc(free_tree, sizeof *node);
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
					node->edges = free_tree_realloc(trie->memory, node->edges, (n - 1) * sizeof *edge);
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
	const size_t arena_size = n * (sizeof(char*) + strlen(prefix) + 16);
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
	subtrie_leaf_strings(subtrie, prefix, strlen(prefix), &str_arena, &char_arena);
	return (char**)str_arena.memory;
}

static void relative (trie_node_t* node)
{
	for (int i = 0; i < node->num_edges; ++i)
	{
		trie_edge_t* edge = node->edges + i;
		if (edge->c != '\0')
		{
			relative(edge->token);
		}
		edge->token = (void*)((size_t)edge->token - (size_t)&edge->token);
	}
	node->edges = (void*)((size_t)node->edges - (size_t)&node->edges);
}

static void absolute (trie_node_t* node)
{
	node->edges = (void*)((size_t)node->edges + (size_t)&node->edges);
	for (int i = 0; i < node->num_edges; ++i)
	{
		trie_edge_t* edge = node->edges + i;
		edge->token = (void*)((size_t)edge->token + (size_t)&edge->token);
		if (edge->c != '\0')
		{
			absolute(edge->token);
		}
	}
}

void trie_serialize(trie_t* trie, char* filename)
{
	assert(trie);
	free_tree_t* memory = trie->memory;
	size_t root_offset = (size_t)trie->root - (size_t)&trie->root;
	size_t len = 3 * sizeof(size_t) + memory->size;
	filemap_t filemap = {0};
	size_t* map = ms_map_file2(filename, len, &filemap);
	memcpy(map, &root_offset, sizeof root_offset);
	relative(trie->root);
	free_tree_serialize(memory, map + 1);
	absolute(trie->root);
	ms_munmap_file2(&filemap);
}

void trie_deserialize(trie_t* trie, char* filename)
{
	assert(trie);
	assert(trie->memory == NULL);
	size_t len = fs_size(filename);
	filemap_t filemap = {0};
	size_t root_offset = 0;
	size_t* map = ms_map_file2(filename, len, &filemap);
	memcpy(&root_offset, map, sizeof root_offset);
	trie->memory = malloc(sizeof *trie->memory);
	trie->memory->memory = NULL;
	free_tree_deserialize(trie->memory, map + 1);
	trie->root = (void*)(root_offset + (size_t)&trie->root);
	absolute(trie->root);
	ms_munmap_file2(&filemap);
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
