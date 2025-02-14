#include "token_trie.h"
#include <stdlib.h>
#include <assert.h>
#include <string.h>

// TODO: try not to recurse HARDER

void add_token(trie_t* node, char* str, void* token)
{
	assert(node);
	if (str == NULL || token == NULL)
	{
		return;
	}
	trie_t* trie = node;
	for (int i = 0; i < strlen(str) + 1; ++i)
	{
		trie_edge_t* edge = trie->edges;
		trie_t* subtrie = NULL;
		for (; edge != NULL; edge->list.next)
		{
			if (str[i] == edge->c)
			{
				subtrie = edge->token;
				goto iteration_tail;
			}
		}
		edge = malloc(sizeof * edge);
		if (str[i] != '\0')
		{
			subtrie = malloc(sizeof * subtrie);
			edge->token = subtrie;
		}
		else
		{
			edge->token = token;
		}
		edge->list.next = trie->edges;
		trie->edges = edge;
	iteration_tail:
		trie = subtrie;
	}
}

void* get_token(trie_t* node, char* str)
{
	assert(node);
	if (str == NULL)
	{
		return NULL;
	}
	trie_t* trie = node;
	for (int i = 0; i < strlen(str) + 1; ++i)
	{
		trie_edge_t* edge = trie->edges;
		trie_t* subtrie = NULL;
		for (; edge != NULL; edge = edge->list.next)
		{
			if (str[i] == '\0' && edge->c == str[i])
			{
				return edge->token;
			}
			else if (str[i] == edge->c)
			{
				subtrie = edge->token;
				goto iteration_tail;
			}
		}
		edge->list.next = trie->edges;
		trie->edges = edge;
	iteration_tail:
		if (subtrie != NULL)
		{
			trie = subtrie;
		}
		else
		{
			return NULL;
		}
	}
	return NULL;
}

uint8_t recursive_remove_token(trie_t* node, char* str)
{
	list_t** elem = (list_t**)(&node->edges);
	while (*elem)
	{
		trie_edge_t* edge = (trie_edge_t*)*elem;
		if (str[0] == edge->c)
		{
			trie_t* token = edge->token;
			if (str[0] != '\0' && recursive_remove_token(token, str + 1))
			{
				if (token->edges == NULL)
				{
					*elem = (*elem)->next;
					free(token);
					free(edge);
				}
				return 1;
			}
			else if (str[0] == '\0')
			{
				*elem = (*elem)->next;
				free(token);
				free(edge);
				return 1;
			}
		}
		elem = &(*elem)->next;
	}
	return 0;
}

uint8_t remove_token(trie_t* node, char* str)
{
	assert(node);
	if (str == NULL)
	{
		return 0;
	}
	return recursive_remove_token(node, str);
}

uint32_t num_leaves(trie_t* node)
{
	uint32_t cum = 0;
	for (trie_edge_t* edge = node->edges; edge != NULL; edge = edge->list.next)
	{
		switch(edge->c)
		{
		case '\0':
			++cum;
			break;
		default:
			cum += num_leaves(edge->token);
		}
	}
	return cum;
}

void leaf_strings(trie_t* node, char** results, uint8_t* offset, char* prefix, char* buffer, const uint8_t b_offset)
{
	for (trie_edge_t* edge = node->edges; edge != NULL; edge = edge->list.next) // why dont I hustle as hard as I hate?
	{
		char* temp;
		switch(edge->c)
		{
		case '\0':
			temp = malloc(strlen(prefix) + b_offset + 1);
			memcpy(temp, prefix, strlen(prefix));
			memcpy(temp + strlen(prefix), buffer, b_offset);
			temp[strlen(prefix) + b_offset] = '\0';
			results[*offset++] = temp;
			break;
		default:
			buffer[b_offset] = edge->c;
			leaf_strings(edge->token, results, offset, prefix, buffer, b_offset + 1);
		}
	}
}

char** completion_candidates(trie_t* node, char* prefix, uint32_t* num_candidates)
{
	assert(node);
	num_candidates = 0;
	if (prefix == NULL)
	{
		return NULL;
	}
	trie_t* trie = node;
	for (int i = 0; i < strlen(prefix); ++i)
	{
		trie_edge_t* edge = trie->edges;
		trie_t* subtrie = NULL;
		while (edge)
		{
			if (prefix[i] == edge->c)
			{
				subtrie = edge->token;
				goto iteration_tail;
			}
			edge = edge->list.next;
		}
		edge->list.next = trie->edges;
		trie->edges = edge;
	iteration_tail:
		if (subtrie != NULL)
		{
			trie = subtrie;
		}
		else
		{
			return NULL;
		}
	}
	*num_candidates = num_leaves(trie);
	char** candidates = malloc(*num_candidates * sizeof * candidates);
	char buf[255] = {0};
	uint8_t offset = 0;
	leaf_strings(trie, candidates, &offset, prefix, &buf[0], 0);
	return candidates;
}
