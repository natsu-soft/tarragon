#ifndef TOKEN_TRIE_H
#define TOKEN_TRIE_H

typedef unsigned char uint8_t;
typedef unsigned long uint32_t;

typedef struct token_trie_t {
	/* struct token_child_t* children; */
	void* children; // absolute devilish tactic right here, fam
	char c;
} token_trie_t;

// TODO: custom mallocs

token_trie_t* add_token(token_trie_t* node, char* str);

token_trie_t* find_token(token_trie_t* node, char* str);

uint8_t remove_token(token_trie_t* node, char* str);

char** completion_candidates(token_trie_t* node, char* prefix, uint32_t* num_candidates);

#endif /* End TOKEN_TRIE_H */
