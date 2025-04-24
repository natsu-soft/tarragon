#ifndef ARRAY_TRIE_H
#define ARRAY_TRIE_H

#include <stdint.h>

#ifdef _WIN32
typedef struct _iobuf FILE;
#else
typedef struct _IO_FILE FILE;
#endif

typedef struct arena_t arena_t;

typedef struct trie_t
{
	struct trie_node_t* root;
	struct free_tree_t* support;
} trie_t;

typedef struct trie_node_t {
	struct trie_edge_t* edges;
	uint8_t num_edges;
} trie_node_t;

typedef struct trie_edge_t
{
	void* token; /* >:^) */
	char c;
} trie_edge_t;

/* void trie_init(trie_t* trie, uint32_t alloc_size); */

/* void trie_deinit(trie_t* trie); */

void trie_add_token(trie_t* trie, char* str, void* token);

void* trie_get_token(trie_t* trie, char* str);

uint8_t trie_remove_token(trie_t* trie, char* str);

uint32_t trie_num_leaves(const trie_t* node);

char** trie_candidates(const trie_t* trie, arena_t* arena, const char* prefix, uint32_t* num_candidates);

void trie_serialize(trie_t* trie, char* filename);

void trie_deserialize(trie_t* trie, struct free_tree_t* support, char* filename);

void trie_dump_dot(FILE* dest, trie_t* trie, const char* root_label);

#endif /* End ARRAY_TRIE_H */
