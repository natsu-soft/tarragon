#ifndef TRIE_H
#define TRIE_H

/* #include "list.h" */

/* #ifdef _WIN32 */
/* typedef struct _iobuf FILE; */
/* #else */
/* typedef struct _IO_FILE FILE; */
/* #endif */

/* typedef struct arena_t arena_t; */
/* typedef unsigned char uint8_t; */
/* typedef unsigned int uint32_t; */

/* typedef struct trie_t { */
/* 	struct trie_edge_t* edges; */
/* } trie_t; */

/* typedef struct trie_edge_t */
/* { */
/* 	list_t list; // HAS to be first for BASED casting purposes */
/* 	void* token; /\* >:^) *\/ */
/* 	char c; */
/* }trie_edge_t; */
/* /\* } __attribute__((packed))trie_edge_t; *\/ // NOTE: reduces size by 7 bytes but FUCKS alignment */


/* void trie_add_token(trie_t* node, char* str, void* token); */

/* void* trie_get_token(trie_t* node, char* str); */

/* uint8_t trie_remove_token(trie_t* node, char* str); */

/* uint32_t trie_num_leaves(trie_t* node); */

/* char** old_trie_candidates(trie_t* node, struct arena_t* candidate_arena, const char* prefix, uint32_t* num_candidates); */

/* void trie_free_nonlinear(trie_t* trie); */

/* void trie_information(trie_t* node, uint32_t* num_nodes, uint32_t* num_edges); */

/* void trie_defrag(trie_t** root); */

/* void trie_serialize(trie_t* root, FILE* fp); */

/* void trie_serialize_map(trie_t* root, char* filename); */

/* trie_t* trie_deserialize(FILE* fp); */

/* void trie_dump_dot(FILE* buf, trie_t* node, char* root_label); */

#endif /* End TRIE_H */
