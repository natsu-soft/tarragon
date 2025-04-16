#include "trie.h"
/* #include <stdlib.h> */
/* #include <stdio.h> */
/* #include <assert.h> */
/* #include <string.h> */
/* #include "error_macros.h" */
/* #include "file_sys.h" */
/* #include "arena.h" */
/* #include "mem_sys.h" */

/* void trie_add_token(trie_t* node, char* str, void* token) */
/* { */
/* 	assert(node); */
/* 	if (str == NULL) */
/* 	{ */
/* 		return; */
/* 	} */
/* 	trie_t* trie = node; */
/* 	for (int i = 0; i < strlen(str) + 1; ++i) */
/* 	{ */
/* 		trie_t* subtrie = NULL; */
/* 		trie_edge_t* edge = trie->edges; */
/* 		for (; edge != NULL; edge = edge->list.next) */
/* 		/\* while (edge != NULL) *\/ */
/* 		{ */
/* 			if (str[i] == edge->c) */
/* 			{ */
/* 				subtrie = edge->token; */
/* 				goto iteration_tail; */
/* 			} */
/* 			/\* edge = (trie_edge_t*)edge->list.next; *\/ */
/* 		} */
/* 		edge = malloc(sizeof * edge); */
/* 		edge->c = str[i]; */
/* 		switch(str[i]) */
/* 		{ */
/* 		case '\0': */
/* 			edge->token = token; */
/* 			break; */
/* 		default: */
/* 			subtrie = malloc(sizeof * subtrie); */
/* 			subtrie->edges = NULL; */
/* 			edge->token = subtrie; */
/* 		} */
/* 		edge->list.next = trie->edges; */
/* 		trie->edges = edge; */
/* 	iteration_tail: */
/* 		trie = subtrie; */
/* 	} */
/* } */

/* void* trie_get_token(trie_t* node, char* str) */
/* { */
/* 	assert(node); */
/* 	if (str == NULL) */
/* 	{ */
/* 		return NULL; */
/* 	} */
/* 	trie_t* trie = node; */
/* 	for (int i = 0; i < strlen(str) + 1; ++i) */
/* 	{ */
/* 		trie_edge_t* edge = trie->edges; */
/* 		trie_t* subtrie = NULL; */
/* 		for (; edge != NULL; edge = edge->list.next) */
/* 		{ */
/* 			if (str[i] == '\0' && edge->c == str[i]) */
/* 			{ */
/* 				return edge->token; */
/* 			} */
/* 			else if (str[i] == edge->c) */
/* 			{ */
/* 				subtrie = edge->token; */
/* 				goto iteration_tail; */
/* 			} */
/* 		} */
/* 		edge->list.next = trie->edges; */
/* 		trie->edges = edge; */
/* 	iteration_tail: */
/* 		if (subtrie != NULL) */
/* 		{ */
/* 			trie = subtrie; */
/* 		} */
/* 		else */
/* 		{ */
/* 			return NULL; */
/* 		} */
/* 	} */
/* 	return NULL; */
/* } */

/* // TODO: try not to recurse HARDER */

/* static uint8_t recursive_remove_token(trie_t* node, char* str) */
/* { */
/* 	list_t** elem = (list_t**)(&node->edges); */
/* 	while (*elem) */
/* 	{ */
/* 		trie_edge_t* edge = (trie_edge_t*)*elem; */
/* 		if (str[0] == edge->c) */
/* 		{ */
/* 			trie_t* token = edge->token; */
/* 			if (str[0] != '\0' && recursive_remove_token(token, str + 1)) */
/* 			{ */
/* 				if (token->edges == NULL) */
/* 				{ */
/* 					*elem = (*elem)->next; */
/* 					free(token); */
/* 					free(edge); */
/* 				} */
/* 				return 1; */
/* 			} */
/* 			else if (str[0] == '\0') */
/* 			{ */
/* 				*elem = (*elem)->next; */
/* 				free(token); */
/* 				free(edge); */
/* 				return 1; */
/* 			} */
/* 		} */
/* 		elem = &(*elem)->next; */
/* 	} */
/* 	return 0; */
/* } */

/* uint8_t trie_remove_token(trie_t* node, char* str) */
/* { */
/* 	assert(node); */
/* 	if (str == NULL) */
/* 	{ */
/* 		return 0; */
/* 	} */
/* 	return recursive_remove_token(node, str); */
/* } */

/* uint32_t trie_num_leaves(trie_t* node) */
/* { */
/* 	uint32_t cum = 0; */
/* 	for (trie_edge_t* edge = node->edges; edge != NULL; edge = edge->list.next) */
/* 	{ */
/* 		switch(edge->c) */
/* 		{ */
/* 		case '\0': */
/* 			++cum; */
/* 			break; */
/* 		default: */
/* 			cum += trie_num_leaves(edge->token); */
/* 		} */
/* 	} */
/* 	return cum; */
/* } */

/* void leaf_strings(trie_t* node, char** results, uint8_t* offset, char* prefix, char* buffer, const uint8_t b_offset) */
/* { */
/* 	for (trie_edge_t* edge = node->edges; edge != NULL; edge = edge->list.next) // why dont I hustle as hard as I hate? */
/* 	{ */
/* 		char* temp; */
/* 		switch(edge->c) */
/* 		{ */
/* 		case '\0': */
/* 			temp = malloc(strlen(prefix) + b_offset + 1); */
/* 			memcpy(temp, prefix, strlen(prefix)); */
/* 			memcpy(temp + strlen(prefix), buffer, b_offset); */
/* 			temp[strlen(prefix) + b_offset] = '\0'; */
/* 			results[*offset] = temp; */
/* 			*offset += 1; */
/* 			/\* printf("leaf: %s, offset: %hhu\n", temp, *offset); *\/ */
/* 			break; */
/* 		default: */
/* 			/\* printf("edge->c: %c\n", edge->c); *\/ */
/* 			buffer[b_offset] = edge->c; */
/* 			leaf_strings(edge->token, results, offset, prefix, buffer, b_offset + 1); */
/* 		} */
/* 	} */
/* } */

/* char** old_trie_candidates(trie_t* node, arena_t* candidate_arena, const char* prefix, uint32_t* num_candidates) */
/* { */
/* 	assert(node); */
/* 	*num_candidates = 0; */
/* 	if (prefix == NULL) */
/* 	{ */
/* 		prefix = ""; */
/* 	} */
/* 	trie_t* trie = node; */
/* 	for (int i = 0; i < strlen(prefix); ++i) */
/* 	{ */
/* 		trie_edge_t* edge = trie->edges; */
/* 		trie_t* subtrie = NULL; */
/* 		while (edge) */
/* 		{ */
/* 			if (prefix[i] == edge->c) */
/* 			{ */
/* 				subtrie = edge->token; */
/* 				goto iteration_tail; */
/* 			} */
/* 			edge = edge->list.next; */
/* 		} */
/* 		edge->list.next = trie->edges; */
/* 		trie->edges = edge; */
/* 	iteration_tail: */
/* 		if (subtrie != NULL) */
/* 		{ */
/* 			trie = subtrie; */
/* 		} */
/* 		else */
/* 		{ */
/* 			return NULL; */
/* 		} */
/* 	} */
/* 	*num_candidates = trie_num_leaves(trie); */
/* 	char** candidates = malloc(*num_candidates * sizeof * candidates); */
/* 	char buf[255] = {0}; */
/* 	uint8_t offset = 0; */
/* 	leaf_strings(trie, candidates, &offset, prefix, &buf[0], 0); */
/* 	return candidates; */
/* } */

/* void trie_free_nonlinear(trie_t* trie) */
/* { */
/* 	trie_edge_t* edge = trie->edges; */
/* 	while (edge != NULL) */
/* 	{ */
/* 		if (edge->c != '\0') */
/* 		{ */
/* 			trie_free_nonlinear(edge->token); */
/* 		} */
/* 		trie_edge_t* e = edge; */
/* 		edge = edge->list.next; */
/* 		free(e); */
/* 	} */
/* } */

/* void trie_information(trie_t* node, uint32_t* num_nodes, uint32_t* num_edges) */
/* { */
/* 	*num_nodes += 1; */
/* 	for (trie_edge_t* edge = node->edges; edge != NULL; edge = edge->list.next) */
/* 	{ */
/* 		*num_edges += 1; */
/* 		if (edge->c != '\0') */
/* 		{ */
/* 			trie_information(edge->token, num_nodes, num_edges); */
/* 		} */
/* 	} */
/* } */

/* static trie_t* trie_defrag_recursive(trie_t* node, arena_t* node_arena, arena_t* edge_arena) */
/* { */
/* 	trie_t* t = arena_alloc(node_arena, sizeof *t); */
/* 	t->edges = node->edges != NULL ? edge_arena->memory + edge_arena->size : NULL; */
/* 	for (trie_edge_t* edge = node->edges; edge != NULL; edge = edge->list.next) */
/* 	{ */
/* 		trie_edge_t* e = arena_calloc(edge_arena, sizeof *e); */
/* 		e->c = edge->c; */
/* 		// TODO: leaf handling BUT FOR NOW THEY DONT MATTER SO LIFE IS GOOD! */
/* 		e->token = edge->c != '\0' ? trie_defrag_recursive(edge->token, node_arena, edge_arena) : NULL; */
/* 		e->list.next = edge->list.next != NULL ? edge_arena->memory + edge_arena->size: NULL; */
/* 	} */
/* 	return t; */
/* } */


/* void trie_defrag(trie_t** root) */
/* { */
/* 	trie_t* node = *root; */
/* 	uint32_t num_nodes = 0; */
/* 	uint32_t num_edges = 0; */
/* 	trie_information(node, &num_nodes, &num_edges); */
/* 	void* addr = calloc(num_nodes * sizeof(trie_t) + num_edges * sizeof(trie_edge_t), 1); */
/* 	arena_t node_arena = */
/* 	{ */
/* 		.cap = num_nodes * sizeof(trie_t), */
/* 		.memory = addr */
/* 	}; */
/* 	arena_t edge_arena = */
/* 	{ */
/* 		.cap = num_edges * sizeof(trie_edge_t), */
/* 		.memory = node_arena.memory + node_arena.cap */
/* 	}; */
/* 	*root = trie_defrag_recursive(node, &node_arena, &edge_arena); */
/* 	trie_free_nonlinear(node); */
/* } */

/* static void trie_defrag_with_info(trie_t** root, uint32_t* num_nodes, uint32_t* num_edges) */
/* { */
/* 	trie_t* node = *root; */
/* 	trie_information(node, num_nodes, num_edges); */
/* 	void* addr = calloc(*num_nodes * sizeof(trie_t) + *num_edges * sizeof(trie_edge_t), 1); */
/* 	arena_t node_arena = */
/* 	{ */
/* 		.cap = *num_nodes * sizeof(trie_t), */
/* 		.memory = addr */
/* 	}; */
/* 	arena_t edge_arena = */
/* 	{ */
/* 		.cap = *num_edges * sizeof(trie_edge_t), */
/* 		.memory = node_arena.memory + node_arena.cap */
/* 	}; */
/* 	*root = trie_defrag_recursive(node, &node_arena, &edge_arena); */
/* 	trie_free_nonlinear(node); */
/* } */

/* void trie_serialize_map(trie_t* root, char* filename) */
/* { */
/* 	uint32_t num_nodes = 0; */
/* 	uint32_t num_edges = 0; */
/* 	trie_defrag_with_info(&root, &num_nodes, &num_edges); */
/* 	size_t size_trie = num_nodes * sizeof(trie_t) + num_edges * sizeof(trie_edge_t); */
/* 	int fd = 0; */
/* 	uint32_t* nums = ms_map_file(filename, 2 * sizeof(uint32_t) + size_trie, &fd); */
/* 	assertf(nums > 0, "ms_mapfile FAILED!\n"); */
/* 	nums[0] = num_nodes; */
/* 	nums[1] = num_edges; */
/* 	memcpy(nums + 2, root, size_trie); */
/* 	trie_t* nodes = nums + 2; */
/* 	const size_t base_addr = (size_t)root; */
/* 	#define REL_ADDR(P) ((size_t)P - base_addr) */
/* 	for (uint32_t i = 0; i < num_nodes; ++i) */
/* 	{ */
/* 		nodes[i].edges = nodes[i].edges != NULL ? (void*)REL_ADDR(nodes[i].edges) : 0x00; */
/* 	} */
/* 	trie_edge_t* edges = nodes + num_nodes; */
/* 	for (uint32_t i = 0; i < num_edges; ++i) */
/* 	{ */
/* 		edges[i].list.next = edges[i].list.next != NULL ? (void*)REL_ADDR(edges[i].list.next) : 0; */
/* 		edges[i].token = edges[i].token != NULL ? (void*)REL_ADDR(edges[i].token) : 0; */
/* 	} */
/* 	assert(ms_munmap_file(nums, 2 * sizeof(uint32_t) + size_trie, fd) == 0); */
/* 	#undef REL_ADDR */
/* } */

/* void trie_serialize(trie_t* root, FILE* fp) */
/* { */
/* 	uint32_t num_nodes = 0; */
/* 	uint32_t num_edges = 0; */
/* 	trie_defrag_with_info(&root, &num_nodes, &num_edges); */
/* 	fwrite(&num_nodes, sizeof num_nodes, 1, fp); */
/* 	fwrite(&num_edges, sizeof num_edges, 1, fp); */
/* 	const size_t base_addr = (size_t)root; */
/* 	// GOD IF ONLY I COULD MMAP */
/* 	#define REL_ADDR(P) ((size_t)P - base_addr) */
/* 	trie_t nodes[num_nodes]; */
/* 	memcpy(nodes, root, num_nodes * sizeof *nodes); */
/* 	for (uint32_t i = 0; i < num_nodes; ++i) */
/* 	{ */
/* 		if (root[i].edges == NULL) */
/* 		{ */
/* 			printf("BRUH: %d\n", i); */
/* 		} */
/* 		nodes[i].edges = nodes[i].edges != NULL ? (void*)REL_ADDR(nodes[i].edges) : 0x00; */
/* 	} */
/* 	assert(fwrite(nodes, sizeof *root, num_nodes, fp) == num_nodes); */
/* 	trie_edge_t edges[num_edges]; */
/* 	memcpy(edges, root + num_nodes, num_edges * sizeof *edges); */
/* 	for (uint32_t i = 0; i < num_edges; ++i) */
/* 	{ */
/* 		edges[i].list.next = edges[i].list.next != NULL ? (void*)REL_ADDR(edges[i].list.next) : 0; */
/* 		edges[i].token = edges[i].token != NULL ? (void*)REL_ADDR(edges[i].token) : 0; */
/* 	} */
/* 	assert(fwrite(edges, sizeof *edges, num_edges, fp) == num_edges); */
/* 	fflush(fp); */
/* 	#undef REL_ADDR */
/* } */

/* trie_t* trie_deserialize(FILE* fp) */
/* { */
/* 	uint32_t num_nodes = 0; */
/* 	uint32_t num_edges = 0; */
/* 	fread(&num_nodes, sizeof(uint32_t), 1, fp); */
/* 	fread(&num_edges, sizeof(uint32_t), 1, fp); */
/* 	trie_t* trie = malloc(num_nodes * sizeof(trie_t) + num_edges * sizeof(trie_edge_t)); */
/* 	trie_edge_t* edge = trie + num_nodes; */
/* 	// can't mmap but this is kinda hot tbqh */
/* 	assert(fread(trie, sizeof(trie_t), num_nodes, fp) == num_nodes); */
/* 	assert(fread(edge, sizeof(trie_edge_t), num_edges, fp) == num_edges); */
/* 	const size_t base_addr = (size_t)trie; */
/* 	#define ABS_PTR(ADDR) (void*)((size_t)ADDR + base_addr) */
/* 	if (num_nodes > 0) */
/* 	{ */
/* 		for (uint32_t i = 0; i < num_nodes; ++i) */
/* 		{ */
/* 			trie[i].edges = trie[i].edges > 0 ? ABS_PTR(trie[i].edges) : NULL; */
/* 		} */
/* 		for (uint32_t i = 0; i < num_edges; ++i) */
/* 		{ */
/* 			edge[i].list.next = edge[i].list.next > 0 ? ABS_PTR(edge[i].list.next) : NULL; */
/* 			edge[i].token = edge[i].token > 0 ? ABS_PTR(edge[i].token) : NULL; */
/* 		} */
/* 		return trie; */
/* 	} */
/* 	else */
/* 	{ */
/* 		return NULL; */
/* 	} */
/* 	#undef ABS_PTR */
/* } */

/* void dump_dot_recursive(FILE* buf, trie_t* node, uint32_t* id) */
/* { */
/* 	uint32_t node_id = *id; */
/* 	for (trie_edge_t* edge = node->edges; edge != NULL; edge = edge->list.next) */
/* 	{ */
/* 		if (edge->c != '\0') */
/* 		{ */
/* 			*id += 1; */
/* 			fprintf(buf, "\t%d[label=\"%c\"];\n", *id, edge->c); */
/* 			fprintf(buf, "\t%d -- %d\n", node_id, *id); */
/* 			dump_dot_recursive(buf, edge->token, id); */
/* 		} */
/* 	} */
/* } */

/* void trie_dump_dot(FILE* buf, trie_t* node, char* root_label) */
/* { */
/* 	assert(buf != NULL); */
/* 	assert(node != NULL); */
/* 	fprintf(buf, "strict graph {\n"); */
/* 	fprintf(buf, "\t0[label=\"%s\"];\n", root_label); */
/* 	uint32_t id = 0; */
/* 	dump_dot_recursive(buf, node, &id); */
/* 	fprintf(buf, "}\n"); */
/* } */

/* int trie_test(int argc, char** argv) */
/* { */
/* 	printf("testing trie behavior\n"); */
/* 	printf("sizeof(trie_t): %lu\nsizeof(trie_edge_t): %lu\n", sizeof(trie_t), sizeof(trie_edge_t)); */
/* 	trie_edge_t* e = malloc(sizeof *e); */
/* 	printf("e: %lu, e+1: %lu\n", (size_t)e, (size_t)(e + 1)); */
/* 	free(e); */
/* 	FILE* fp = fopen("test_stuff/fruits.txt", "r"); */
/* 	trie_t fruit_trie = {}; // u havin a giggle m8? */
/* 	size_t lmao = 80; */
/* 	char fruit[lmao]; */
/* 	while(fs_getline(fruit, lmao, fp) > 0) */
/* 	{ */
/* 		trie_add_token(&fruit_trie, fruit, NULL); */
/* 	} */
/* 	fflush(fp); */
/* 	fclose(fp); */
/* 	/\* trie_serialize_map(&fruit_trie, "fruit_trie.bin"); *\/ */
/* 	/\* trie_serialize_map(&fruit_trie, "fruit_trie.bin"); *\/ */
/* 	fp = fopen("fruit_fp.bin", "w"); */
/* 	trie_serialize(&fruit_trie, fp); */
/* 	fclose(fp); */
/* 	fp = fopen("fruit_fp.bin", "r"); */
/* 	trie_t* io_trie = trie_deserialize(fp); */
/* 	fflush(fp); */
/* 	fclose(fp); */
/* 	FILE* buf = fopen("fruits.dot", "w"); */
/* 	if (buf != NULL) */
/* 	{ */
/* 		trie_dump_dot(buf, io_trie, "DESERIALIZED TRIE LMAO"); // works! */
/* 		fflush(buf); */
/* 		fclose(buf); */
/* 	} */
/* 	uint32_t num_fruits = 0; */
/* 	arena_t arena = {}; */
/* 	char** all_fruits = old_trie_candidates(io_trie, &arena, NULL, &num_fruits); */
/* 	// NOTE: you CANNOT remove tokens from deserialized tries */
/* 	// this is NOT enterprise shit I am NOT coping.  This is the just fruits of one FAT malloc */
/* 	/\* assert(num_fruits == 51); *\/ */
/* 	/\* assert(strcmp(all_fruits[0], "Yuzu") == 0); *\/ */
/* 	/\* /\\* trie_remove_token(io_trie, "Ap"); *\\/ *\/ */
/* 	/\* trie_remove_token(io_trie, "Yuzu"); *\/ */
/* 	/\* assert(trie_num_leaves(io_trie) == 50); *\/ */
/* 	/\* trie_remove_token(io_trie, "Apple"); *\/ */
/* 	/\* assert(trie_num_leaves(io_trie) == 49); *\/ */
/* 	free(io_trie); */
/* 	return 0; */
/* } */
