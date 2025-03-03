#include "token_trie.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "file_sys.h"

void add_token(trie_t* node, char* str, void* token)
{
	assert(node);
	if (str == NULL)
	{
		return;
	}
	trie_t* trie = node;
	for (int i = 0; i < strlen(str) + 1; ++i)
	{
		trie_t* subtrie = NULL;
		trie_edge_t* edge = trie->edges;
		for (; edge != NULL; edge = edge->list.next)
		/* while (edge != NULL) */
		{
			if (str[i] == edge->c)
			{
				subtrie = edge->token;
				goto iteration_tail;
			}
			/* edge = (trie_edge_t*)edge->list.next; */
		}
		edge = malloc(sizeof * edge);
		edge->c = str[i];
		switch(str[i])
		{
		case '\0':
			edge->token = token;
			break;
		default:
			subtrie = malloc(sizeof * subtrie);
			subtrie->edges = NULL;
			edge->token = subtrie;
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

// TODO: try not to recurse HARDER

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
			results[*offset] = temp;
			*offset += 1;
			/* printf("leaf: %s, offset: %hhu\n", temp, *offset); */
			break;
		default:
			/* printf("edge->c: %c\n", edge->c); */
			buffer[b_offset] = edge->c;
			leaf_strings(edge->token, results, offset, prefix, buffer, b_offset + 1);
		}
	}
}

char** completion_candidates(trie_t* node, uint32_t* num_candidates, char* prefix)
{
	assert(node);
	*num_candidates = 0;
	if (prefix == NULL)
	{
		prefix = "";
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

void dump_dot_recursive(FILE* buf, trie_t* node, uint32_t* id)
{
	uint32_t node_id = *id;
	for (trie_edge_t* edge = node->edges; edge != NULL; edge = edge->list.next)
	{
		if (edge->c != '\0')
		{
			*id += 1;
			fprintf(buf, "\t%d[label=\"%c\"];\n", *id, edge->c);
			fprintf(buf, "\t%d -- %d\n", node_id, *id);
			dump_dot_recursive(buf, edge->token, id);
		}
	}
}

void dump_dot(FILE* buf, trie_t* node, char* root_label)
{
	assert(buf != NULL);
	assert(node != NULL);
	fprintf(buf, "strict graph {\n");
	fprintf(buf, "\t0[label=\"%s\"];\n", root_label);
	uint32_t id = 0;
	dump_dot_recursive(buf, node, &id);
	fprintf(buf, "}\n");
}

int token_trie_test(int argc, char** argv)
{
	printf("rudimentary trie testing\n");
	FILE* fp = fopen("test_stuff/fruits.txt", "r");
	trie_t fruit_trie = { 0 }; // u havin a giggle m8?
	size_t lmao = 80;
	char* fruit = malloc(lmao);
	/* while(getline(&fruit, &lmao, fp) > 0) */
	while(fs_getline(fruit, lmao, fp) > 0)
	{
		fruit[strlen(fruit) - 1] = '\0'; // yeah I HATE newlines
		add_token(&fruit_trie, fruit, NULL);
	}
	/* printf("num leaves: %d\n", num_leaves(&fruit_trie)); */
	FILE* buf = fopen("fruits.dot", "w");
	if (buf != NULL)
	{
		dump_dot(buf, &fruit_trie, "FRUIT TRIE LMAO"); // works!
	}
	uint32_t num_fruits = 0;
	char** all_fruits = completion_candidates(&fruit_trie, &num_fruits, NULL);
	/* printf("num_fruits: %d\n", num_fruits); */
	assert(num_fruits == 51);
	assert(strcmp(all_fruits[0], "Yuzu") == 0);
	/* for (uint32_t i = 0; i < num_fruits; ++i) */
	/* { */
	/* 	printf("fruit %hhu: %s\n", i, all_fruits[i]); */
	/* } */
	remove_token(&fruit_trie, "Ap");
	remove_token(&fruit_trie, "Yuzu");
	assert(num_leaves(&fruit_trie) == 50);
	remove_token(&fruit_trie, "Apple");
	assert(num_leaves(&fruit_trie) == 49);
	/* printf("leaves post removal: %d\n", num_leaves(&fruit_trie)); */
	free(fruit);
	fclose(fp);
	return 0;
}
