#include "trie.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "file_sys.h"
#include "arena.h"

void trie_add_token(trie_t* node, char* str, void* token)
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

void* trie_get_token(trie_t* node, char* str)
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

static uint8_t recursive_remove_token(trie_t* node, char* str)
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

uint8_t trie_remove_token(trie_t* node, char* str)
{
	assert(node);
	if (str == NULL)
	{
		return 0;
	}
	return recursive_remove_token(node, str);
}

uint32_t trie_num_leaves(trie_t* node)
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
			cum += trie_num_leaves(edge->token);
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

char** trie_candidates(trie_t* node, uint32_t* num_candidates, char* prefix)
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
	*num_candidates = trie_num_leaves(trie);
	char** candidates = malloc(*num_candidates * sizeof * candidates);
	char buf[255] = {0};
	uint8_t offset = 0;
	leaf_strings(trie, candidates, &offset, prefix, &buf[0], 0);
	return candidates;
}

void trie_free_nonlinear(trie_t* trie)
{
	trie_edge_t* edge = trie->edges;
	while (edge != NULL)
	{
		if (edge->c != '\0')
		{
			trie_free_nonlinear(edge->token);
		}
		trie_edge_t* e = edge;
		edge = edge->list.next;
		free(e);
	}
}

void trie_information(trie_t* node, uint32_t* num_nodes, uint32_t* num_edges)
{
	*num_nodes += 1;
	for (trie_edge_t* edge = node->edges; edge != NULL; edge = edge->list.next)
	{
		*num_edges += 1;
		if (edge->c != '\0')
		{
			trie_information(edge->token, num_nodes, num_edges);
		}
	}
}

/* size_t trie_size(trie_t* node) */
/* { */
/* 	size_t res = 0; */
/* 	for (trie_edge_t* edge = node->edges; edge != NULL; edge = edge->list.next) */
/* 	{ */
/* 		res += sizeof *edge; */
/* 		res += 3 * sizeof edge->list; // do this for alignment */
/* 		if (edge->c != '\0') // NOTE: going to have to rejigger this for ACTUAL tries */
/* 		{ */
/* 			res += trie_size(edge->token); */
/* 		} */
/* 	} */
/* 	return res; */
/* } */

/* trie_t* trie_lineup(arena_t* arena, trie_t* node) */
/* { */
/* 	trie_t* t = arena_alloc_aligned(arena, sizeof *t, sizeof *t); */
/* 	t->edges = node->edges != NULL ? arena->memory + arena->size : NULL; */
/* 	trie_edge_t* e; */
/* 	for (trie_edge_t* edge = node->edges; edge != NULL; edge = edge->list.next) */
/* 	{ */
/* 		e = arena_alloc_aligned(arena, sizeof *e, sizeof e->list); */
/* 		e->c = edge->c; */
/* 		e->list.next = edge->list.next != NULL ? arena->memory + arena->size : NULL; */
/* 	} */
/* 	e = t->edges; */
/* 	for (trie_edge_t* edge = t->edges; edge != NULL; edge = edge->list.next) */
/* 	{ */
/* 		if (e->c != '\0') */
/* 		{ */
/* 			e->token = trie_lineup(arena, edge->token); */
/* 		} */
/* 		else */
/* 		{ */
/* 			e->token = NULL; */
/* 		} */
/* 		e = e->list.next; */
/* 	} */
/* 	return t; */
/* } */

/* // NOTE: ASSUME node > base_addr -- THIS is why you lined-up the trie before serializing */
/* uint8_t trie_serialize_relative(FILE* fp, trie_t* node, size_t base_addr) */
/* { */
/* 	#define RPTR(PTR) (void*)((size_t)PTR - base_addr) */
/* 	if (node->edges != NULL) */
/* 	{ */
/* 		void* data = RPTR(node->edges); */
/* 		if (fwrite(&data, sizeof(size_t), 1, fp) < sizeof(size_t)) goto ret0; */
/* 	} */
/* 	else */
/* 	{ */
/* 		if (fwrite(NULL, sizeof(size_t), 1, fp) < sizeof(size_t)) goto ret0; */
/* 	} */
/* 	for (trie_edge_t* edge = node->edges; edge != NULL; edge = edge->list.next) */
/* 	{ */
/* 		void* data[2]; */
/* 		data[0] = edge->list.next != NULL ? RPTR(edge->list.next) : NULL; */
/* 		data[1] = edge->c != '\0' ? RPTR(edge->token) : NULL; */
/* 		if (fwrite(&data, sizeof(size_t), 2, fp) < 2 * sizeof(size_t)) goto ret0; */
/* 		if (fwrite(&(edge->c), sizeof edge->c, 1, fp) < sizeof(char)) goto ret0; */
/* 		if (edge->c != '\0') */
/* 		{ */
/* 			if (!trie_serialize_relative(fp, edge->token, base_addr)) goto ret0; */
/* 		} */
/* 	} */
/* 	return 1; */
/* ret0: */
/* 	return 0; */
/* 	#undef RELATIVE_PTR */
/* } */

/* uint8_t trie_serialize(FILE* fp, trie_t** root) */
/* { */
/* 	arena_t a = {0}; */
/* 	trie_t* node = *root; */
/* 	size_t trie_bytes = trie_size(node); */
/* 	arena_init(&a, trie_bytes); */
/* 	*root = trie_lineup(&a, node); */
/* 	trie_free(node); */
/* 	node = *root; */
/* 	fwrite(&trie_bytes, sizeof trie_bytes, 1, fp); */
/* 	return trie_serialize_relative(fp, node, (size_t)node); */
/* } */

static trie_t* trie_defrag_recursive(trie_t* node, arena_t* node_arena, arena_t* edge_arena)
{
	trie_t* t = arena_alloc(node_arena, sizeof *t);
	t->edges = node->edges != NULL ? edge_arena->memory + edge_arena->size : NULL;
	for (trie_edge_t* edge = node->edges; edge != NULL; edge = edge->list.next)
	{
		trie_edge_t* e = arena_calloc(edge_arena, sizeof *e);
		e->c = edge->c;
		// TODO: leaf handling BUT FOR NOW THEY DONT MATTER SO LIFE IS GOOD!
		e->token = edge->c != '\0' ? trie_defrag_recursive(edge->token, node_arena, edge_arena) : NULL;
		e->list.next = edge->list.next != NULL ? edge_arena->memory + edge_arena->size: NULL;
	}
	return t;
}

void trie_defrag(trie_t** root, uint32_t* num_nodes, uint32_t* num_edges)
{
	trie_t* node = *root;
	trie_information(node, num_nodes, num_edges);
	void* addr = calloc(*num_nodes * sizeof(trie_t) + *num_edges * sizeof(trie_edge_t), 1);
	arena_t node_arena =
	{
		.cap = *num_nodes * sizeof(trie_t),
		.memory = addr
	};
	arena_t edge_arena =
	{
		.cap = *num_edges * sizeof(trie_edge_t),
		.memory = node_arena.memory + node_arena.cap
	};
	*root = trie_defrag_recursive(node, &node_arena, &edge_arena);
	trie_free_nonlinear(node);
}

void trie_serialize(FILE* fp, trie_t** root)
{
	uint32_t num_nodes = 0;
	uint32_t num_edges = 0;
	trie_defrag(root, &num_nodes, &num_edges);
	fwrite(&num_nodes, sizeof num_nodes, 1, fp);
	fwrite(&num_edges, sizeof num_edges, 1, fp);
	trie_t* trie = *root;
	const size_t base_addr = (size_t)*root;
	// GOD IF ONLY I COULD MMAP
	#define REL_ADDR(P) ((size_t)P - base_addr)
	trie_t nodes[num_nodes];
	memcpy(&nodes[0], trie, num_nodes * sizeof *nodes);
	for (uint32_t i = 0; i < num_nodes; ++i)
	{
		nodes[i].edges = nodes[i].edges != NULL ? (void*)REL_ADDR(nodes[i].edges) : 0;
	}
	fwrite(nodes, sizeof *trie, num_nodes, fp);
	trie_edge_t edges[num_edges];
	memcpy(&edges[0], trie + num_nodes, num_edges * sizeof *edges);
	for (uint32_t i = 0; i < num_edges; ++i)
	{
		edges[i].list.next = edges[i].list.next != NULL ? (void*)REL_ADDR(edges[i].list.next) : 0;
		edges[i].token = edges[i].token != NULL ? (void*)REL_ADDR(edges[i].token) : 0;
	}
	fwrite(edges, sizeof *edges, num_edges, fp);
	#undef REL_ADDR
}

trie_t* trie_deserialize(FILE* fp)
{
	uint32_t num_nodes = 0;
	uint32_t num_edges = 0;
	fread(&num_nodes, sizeof(uint32_t), 1, fp);
	fread(&num_edges, sizeof(uint32_t), 1, fp);
	trie_t* trie = malloc(num_nodes * sizeof(trie_t) + num_edges * sizeof(trie_edge_t));
	trie_edge_t* edge = trie + num_nodes;
	// can't mmap but this is kinda hot tbqh
	fread(trie, sizeof(trie_t), num_nodes, fp);
	fread(edge, sizeof(trie_edge_t), num_edges, fp);
	const size_t base_addr = (size_t)trie;
	#define ABS_PTR(ADDR) (void*)((size_t)ADDR + base_addr)
	if (num_nodes > 0)
	{
		for (uint32_t i = 0; i < num_nodes; ++i)
		{
			trie[i].edges = trie[i].edges > 0 ? ABS_PTR(trie[i].edges) : NULL;
		}
		for (uint32_t i = 0; i < num_edges; ++i)
		{
			edge[i].list.next = edge[i].list.next > 0 ? ABS_PTR(edge[i].list.next) : NULL;
			edge[i].token = edge[i].token > 0 ? ABS_PTR(edge[i].token) : NULL;
		}
		return trie;
	}
	else
	{
		return NULL;
	}
	#undef ABS_PTR
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

void trie_dump_dot(FILE* buf, trie_t* node, char* root_label)
{
	assert(buf != NULL);
	assert(node != NULL);
	fprintf(buf, "strict graph {\n");
	fprintf(buf, "\t0[label=\"%s\"];\n", root_label);
	uint32_t id = 0;
	dump_dot_recursive(buf, node, &id);
	fprintf(buf, "}\n");
}

int trie_test(int argc, char** argv)
{
	printf("testing trie behavior\n");
	printf("sizeof(trie_t): %lu, sizeof(trie_edge_t): %lu\n", sizeof(trie_t), sizeof(trie_edge_t));
	trie_edge_t* e = malloc(sizeof *e);
	printf("e: %lu, e+1: %lu\n", (size_t)e, (size_t)(e + 1));
	free(e);
	FILE* fp = fopen("test_stuff/fruits.txt", "r");
	trie_t fruit_trie = { 0 }; // u havin a giggle m8?
	size_t lmao = 80;
	char* fruit = malloc(lmao);
	while(fs_getline(fruit, lmao, fp) > 0)
	{
		trie_add_token(&fruit_trie, fruit, NULL);
	}
	FILE* buf = fopen("fruits.dot", "w");
	if (buf != NULL)
	{
		trie_dump_dot(buf, &fruit_trie, "FRUIT TRIE LMAO"); // works!
		fflush(buf);
		fclose(buf);
	}
	uint32_t num_fruits = 0;
	char** all_fruits = trie_candidates(&fruit_trie, &num_fruits, NULL);
	assert(num_fruits == 51);
	assert(strcmp(all_fruits[0], "Yuzu") == 0);
	trie_remove_token(&fruit_trie, "Ap");
	trie_remove_token(&fruit_trie, "Yuzu");
	assert(trie_num_leaves(&fruit_trie) == 50);
	trie_remove_token(&fruit_trie, "Apple");
	assert(trie_num_leaves(&fruit_trie) == 49);
	free(fruit);
	fclose(fp);
	return 0;
}
