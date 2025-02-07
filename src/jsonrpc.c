#include "jsonrpc.h"
#include <stdio.h>
#include <string.h>
/* #include <tree_sitter/api.h> */
#include <jansson.h>
/* NOTE: NOT DECREFING JSON STUFF BECAUSE IM BALLING */
#include "arena.h"
#include "logger.h"

/* extern unsigned char client_tsquery[]; */
/* extern unsigned int client_tsquery_len; */
/* extern unsigned char params_tsquery[]; */
/* extern unsigned int params_tsquery_len; */

/* static TSParser* parser = NULL; */
/* static TSQuery* method_query = NULL; */
/* static TSQuery* params_query = NULL; */

/* const TSLanguage *tree_sitter_json(void); */

/* const char err_matching[5][32] = { */
/* 	"UNMATCHED ERROR TYPE", */
/* 	"Syntax", */
/* 	"Node Type", */
/* 	"Field", */
/* 	"Capture", */
/* }; */

/* typedef enum NodeTextAllocation { */
/* 	FIXED_ARRAY, */
/* 	SCRATCH, */
/* } NodeTextAllocation; */

/* static char parse_array[128]; */
static Arena* scratch = NULL;
static Arena* request_arena = NULL;

/* #define QUERY_ASSERT(A) if (A) { fprintf(stderr, "[TSQUERY ERROR] %s:%d  -- %s\n", __FILE__, __LINE__, err_matching[A]); exit(EXIT_FAILURE); } */

/* TSQuery* make_query(const char* query, const int len) */
/* { */
/* 	TSQueryError err; */
/* 	uint32_t err_offset; */
/* 	TSQuery* ts_q = ts_query_new( */
/* 		tree_sitter_json(), */
/* 		query, */
/* 		len, */
/* 		&err_offset, */
/* 		&err); */
/* 	QUERY_ASSERT(err); */
/* 	return ts_q; */
/* } */

/* char* node_text(const TSNode* n, const char* str, NodeTextAllocation allocation) */
/* { */
/* 	uint32_t b0 = ts_node_start_byte(*n); */
/* 	uint32_t b1 = ts_node_end_byte(*n); */
/* 	switch(allocation) */
/* 	{ */
/* 	case FIXED_ARRAY: */
/* 		memmove(&parse_array, str + b0, b1 - b0); */
/* 		parse_array[b1 - b0] = '\0'; */
/* 		return &parse_array[0]; */
/* 	case SCRATCH: */
/* 		if (b0 - b1 + 1 > scratch->cap) */
/* 		{ */
/* 			free(scratch->memory); */
/* 			scratch->size = 0; */
/* 			scratch->memory = malloc(b0 - b1 + 1024); */
/* 		} */
/* 		memmove(scratch->memory, str + b0, b1 - b0); */
/* 		scratch->memory[b1 - b0] = '\0'; */
/* 		scratch->size = b1 - b0 + 1; */
/* 		return scratch->memory; */
/* 	} */
/* 	char* res = malloc(b1 - b0 + 1); */
/* 	strncpy(res, str + b0, b1 - b0); */
/* 	res[b1 - b0] = '\0'; */
/* 	return res; */
/* } */

/*  NOTE: HOPE U DONT OVER ALLOCATE LMAO */
static void* scratch_alloc(size_t bytes)
{
	return arena_alloc(scratch, bytes);
}

static void scratch_free(void* memory)
{
	// lol
}

void rpc_init(void)
{
	/* parser = ts_parser_new(); */
	/* ts_parser_set_language(parser, tree_sitter_json()); */
	/* method_query = make_query(&client_tsquery[0], client_tsquery_len); params_query = make_query(&params_tsquery[0], params_tsquery_len); */
	scratch = arena_init(1024 * 1024 * sizeof(json_t));
	/* scratch = arena_init(12); */
	/* json_malloc_t = &scratch_alloc; */
	request_arena = arena_init(sizeof(Rpc));
}

void rpc_deinit(void)
{
	/* ts_query_delete(method_query); */
	/* ts_query_delete(params_query); */
	/* ts_parser_delete(parser); */
	free(scratch->memory);
	free(scratch);
	free(request_arena->memory);
	free(request_arena);
}

void initialize_params(json_t* params_o, Rpc* rpc);

void parse_rpc (char* str, int len, Rpc* rpc)
{
	json_error_t error;
	json_t* rpc_o = json_loadb(str, len, 0, &error);
	if(!rpc_o)
	{
		ERROR("failed to decode:\n%s", str);
		rpc = NULL;
		return;
	}
	else if (!json_is_object(rpc_o))
	{
		ERROR("rpc is not a json object:\n%s", str);
		rpc = NULL;
	}
	json_t* method_o = json_object_get(rpc_o, "method");
	if (!method_o)
	{
		ERROR("rpc does not contain a method key!");
		rpc = NULL;
		return;
	}
	json_t* id_o = json_object_get(rpc_o, "id");
	if (!id_o)
	{
		rpc->id[0] = '\0';
	}
	else if (json_is_string(id_o))
	{
		const char* id = json_string_value(id_o);
		memcpy(&rpc->id[0], id, strlen(id) + 1);
		json_decref(id_o);
	}
	else if (json_is_integer(id_o))
	{
		sprintf(&rpc->id[0], "%lld", json_integer_value(id_o));
		json_decref(id_o);
	}
	const char* method = json_string_value(method_o);
	if (strcmp(method, "initialize") == 0)
	{
		rpc->method = INITIALIZE;
		json_t* params_o = json_object_get(rpc_o, "params");
		if (!params_o)
		{
			ERROR("Failed to parse initialize parameters!");
			rpc = NULL;
			return;
		}
		else if (!json_is_object(params_o))
		{
			ERROR("Initialize parameters are not a json object!");
			rpc = NULL;
			return;
		}
		initialize_params(params_o, rpc);
		json_decref(params_o);
	}
	else if (strcmp(method, "initialized") == 0)
	{
		rpc->method = INITIALIZED;
	}
	else if (strcmp(method, "shutdown") == 0)
	{
		rpc->method = SHUTDOWN;
	}
	else if (strcmp(method, "exit") == 0)
	{
		rpc->method = EXIT;
	}
	else {
		rpc->method = METHOD_UNKNOWN;
	}
	/* TSTree *tree = ts_parser_parse_string( */
	/* 	parser, */
	/* 	NULL, */
	/* 	json, */
	/* 	len); */
	/* TSQueryCursor* cursor = ts_query_cursor_new(); */
	/* ts_query_cursor_exec(cursor, method_query, ts_tree_root_node(tree)); */
	/* TSNode node; */
	/* TSQueryMatch match; */
	/* if(ts_query_cursor_next_match(cursor, &match)) // just try to match once lmao */
	/* { */
	/* 	// TODO switch */
	/* 	if(match.pattern_index == 0) // client request */
	/* 	{ */
	/* 		// parse the id */
	/* 		node = match.captures[0].node; */
	/* 		char* text = node_text(&node, json, FIXED_ARRAY); */
	/* 		memcpy(rpc->id, text, strlen(text) + 1); */
	/* 		// parse the method name */
	/* 		node = match.captures[1].node; */
	/* 		text = node_text(&node, json, FIXED_ARRAY); */
	/* 		if (strcmp(text, "initialize") == 0) */
	/* 		{ */
	/* 			rpc->method = INITIALIZE; */
	/* 			LOG("received an initialize request!"); */
	/* 		} */
	/* 		else if (strcmp(text, "shutdown") == 0) */
	/* 		{ */
	/* 			rpc->method = SHUTDOWN; */
	/* 			LOG("received a shutdown request!"); */
	/* 		} */
	/* 		else */
	/* 		{ */
	/* 			LOG("unrecognized method: %s", text); */
	/* 			rpc->method = METHOD_UNKNOWN; */
	/* 		} */
	/* 		// parse method parameters */
	/* 		node = match.captures[2].node; */
	/* 		uint32_t b0 = ts_node_start_byte(node); */
	/* 		uint32_t b1 = ts_node_end_byte(node); */
	/* 		parse_params(json + b0, b1 - b0, rpc); */
	/* 	} */
	/* 	else if(match.pattern_index == 1) // client notification (no id) */
	/* 	{ */
	/* 		*rpc->id = '\0'; */
	/* 		node = match.captures[0].node; */
	/* 		char* text = node_text(&node, json, FIXED_ARRAY); */
	/* 		if (strcmp(text, "initialized") == 0) */
	/* 		{ */
	/* 			rpc->method = INITIALIZED; */
	/* 			LOG("received an initialized notice!"); */
	/* 		} */
	/* 		else if (strcmp(text, "exit") == 0) */
	/* 		{ */
	/* 			rpc->method = EXIT; */
	/* 			LOG("received an exit notice!"); */
	/* 		} */
	/* 		else */
	/* 		{ */
	/* 			rpc->method = METHOD_UNKNOWN; */
	/* 			LOG("unrecognized method: %s", text); */
	/* 		} */
	/* 	} */
	/* } */
	/* else */
	/* { */
	/* 	ERROR("Failed to parse a method from %s", json); */
	/* } */
	/* ts_query_cursor_delete(cursor); */
	/* ts_tree_delete(tree); */
}

/* void parse_initialize(char* json, TSQueryCursor* cursor, TSQueryMatch* match, Rpc* request) */
/* { */
/* 	TSNode node; */
/* 	while(ts_query_cursor_next_match(cursor, match)) */
/* 	{ */
/* 		node = match->captures[0].node; */
/* 		char* text = node_text(&node, json, FIXED_ARRAY); */
/* 		if (strcmp(text, "processId") == 0) */
/* 		{ */
/* 			node = match->captures[1].node; */
/* 			text = node_text(&node, json, FIXED_ARRAY); */
/* 			LOG("jsonrcp processId: %s", text); */
/* 			uint32_t process_id = 0; */
/* 			int res = sscanf(text, "%ud", &process_id); */
/* 			LOG("parsed process_id in initialize parameters: %d", process_id); */
/* 			request->params.initialize.process_id = process_id; */
/* 		} */
/* 	} */
/* } */


void initialize_params(json_t* params_o, Rpc* rpc)
{
	// the process with PID 0 is responsible for paging, ergo it is NOT the parent process id (so keep using ur unsigned int u degenerate)
	json_t* pid_o = json_object_get(params_o, "processId");
	if (json_is_null(pid_o))
	{
		rpc->params.initialize.process_id = 0;
	}
	else
	{
		rpc->params.initialize.process_id = json_integer_value(pid_o);
	}
	arena_free(scratch, sizeof * pid_o);
}

const Rpc* rpc_parse(char* json, int len)
{
	json_set_alloc_funcs(&scratch_alloc, &scratch_free);
	scratch->size = 0;
	request_arena->size = 0;
	Rpc* rpc = arena_alloc(request_arena, sizeof *rpc);
	parse_rpc(json, len, rpc);
	return rpc;
}
