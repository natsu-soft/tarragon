#include "token_trie.h"
#include <stdlib.h>
#include <assert.h>

// TODO: try not to recurse

// intrusive list too much work :(
typedef struct token_child_t {
	struct token_child_t* next;
	token_trie_t trie;
} token_child_t;


token_trie_t* add_token(token_trie_t* node, char* str)
{
	assert(node);
	if (str == NULL)
	{
		return NULL;
	}
	token_child_t* child = node->children;
	token_trie_t* subtrie;
	while (child)
	{
		if (child->trie.c == str[0])
		{
			subtrie = &child->trie;
			goto recurse;
		}
		child = child->next;
	}
	child = malloc(sizeof * child);
	child->trie.c = str[0];
	child->next = node->children;
	node->children = child;
	subtrie = &child->trie;
recurse:
	return str[0] != '\0' ? add_token(subtrie, str + 1) : subtrie;
}

token_trie_t* find_token(token_trie_t* node, char* str)
{
	assert(node);
	if (str == NULL)
	{
		return NULL;
	}
	token_child_t* child = node->children;
	while (child)
	{
		if (child->trie.c == str[0])
		{
			return find_token(&child->trie, str + 1);
		}
		child = child->next;
	}
	return NULL;
}

uint8_t remove_token(token_trie_t* node, char* str)
{
	assert(node);
	if (str == NULL)
	{
		return 0;
	}
	else if (str[0] == '\n' && node->c == '\n')
	{
		// free THIS node & its children (actually a gd_elem_t and not a linked list)
		free(node->children);
		free(node);
		return 1;
	}
	else if (node->c == str[0])
	{
		token_child_t* child = node->children;
		while (child)
		{
			if (remove_token(&child->trie, str + 1))
			{
				// remove child from node->children
				token_child_t** c = &node->children;
				while(*c != child)
				{
					c = &(*c)->next;
				}
				*c = child->next;
				return 1;
			}
			child = child->next;
		}
	}
	return 0;
}

token_trie_t* find_token_excluding_null(token_trie_t* node, char* str)
{
	assert(node);
	if (str == NULL)
	{
		return NULL;
	}
	token_child_t* child = node->children;
	while (child)
	{
		if (child->trie.c == str[0])
		{
			return str[1] != '\n' ? find_token(&child->trie, str + 1) : &child->trie;
		}
		child = child->next;
	}
	return NULL;
}

uint32_t num_leaves(token_trie_t* node)
{
	if (node->c == '\0')
	{
		return 1;
	}
	else
	{
		uint32_t res = 0;
		token_child_t* child = node->children;
		while (child)
		{
			res += num_leaves(&child->trie);
			child = child->next;
		}
		return res;
	}
}

char** trie_strings(token_trie_t* node, uint32_t* num_strs)
{
	char** res;
	uint32_t n = num_leaves(node);
	res = malloc(n * sizeof *res);
	// depth first search
	*num_strs = n;
	return res;
}

char** completion_candidates(token_trie_t* node, char* prefix, uint32_t* num_candidates)
{
	assert(node);
	if (prefix == NULL)
	{
		num_candidates = 0;
		return NULL;
	}
	token_trie_t* subtrie = find_token_excluding_null(node, prefix);
	if (subtrie)
	{
		return trie_strings(subtrie, num_candidates);
	}
	else
	{
		num_candidates = 0;
		return NULL;
	}
}
