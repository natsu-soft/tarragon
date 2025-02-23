#ifndef TOKEN_TRIE_H
#define TOKEN_TRIE_H

#include "list.h"

#ifdef _WIN32
typedef struct _iobuf FILE;
#else
typedef struct _IO_FILE FILE;
#endif

typedef unsigned char uint8_t;
typedef unsigned int uint32_t;

typedef struct trie_t {
	struct trie_edge_t* edges;
} trie_t;

typedef struct trie_edge_t
{
	list_t list; // HAS to be first for BASED casting purposes
	void* token; /* >:^) */
	char c;
} trie_edge_t;


// TODO: custom mallocs & frees

void add_token(trie_t* node, char* str, void* token);

void* get_token(trie_t* node, char* str);

uint8_t remove_token(trie_t* node, char* str);

uint32_t num_leaves(trie_t* node);

char** completion_candidates(trie_t* node, uint32_t* num_candidates, char* prefix);

void dump_dot(FILE* buf, trie_t* node, char* root_label);

#endif /* End TOKEN_TRIE_H */
