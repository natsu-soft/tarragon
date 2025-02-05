#include "jsonrpc.h"
#include <stdio.h>
#include <string.h>
#include <tree_sitter/api.h>
#include "arena.h"
#include "logger.h"
/* #include <stdlib.h> */

extern unsigned char request_tsquery[];
extern unsigned int request_tsquery_len;
extern unsigned char params_tsquery[];
extern unsigned int params_tsquery_len;

static TSParser* parser = NULL;
static TSQuery* method_query = NULL;
static TSQuery* params_query = NULL;

const TSLanguage *tree_sitter_json(void);

const char err_matching[5][32] = {
	"UNMATCHED ERROR TYPE",
	"Syntax",
	"Node Type",
	"Field",
	"Capture",
};

typedef enum NodeTextAllocation {
	FIXED_ARRAY,
	SCRATCH,
} NodeTextAllocation;

static char parse_array[128];
static Arena* scratch = NULL;
static Arena* request_arena = NULL;

#define QUERY_ASSERT(A) if (A) { fprintf(stderr, "[TSQUERY ERROR] %s:%d  -- %s\n", __FILE__, __LINE__, err_matching[A]); exit(EXIT_FAILURE); }

TSQuery* make_query(const char* query, const int len)
{
	TSQueryError err;
	uint32_t err_offset;
	TSQuery* ts_q = ts_query_new(
		tree_sitter_json(),
		query,
		len,
		&err_offset,
		&err);
	QUERY_ASSERT(err);
	return ts_q;
}

char* node_text(const TSNode* n, const char* str, NodeTextAllocation allocation)
{
	uint32_t b0 = ts_node_start_byte(*n);
	uint32_t b1 = ts_node_end_byte(*n);
	switch(allocation)
	{
	case FIXED_ARRAY:
		memmove(&parse_array, str + b0, b1 - b0);
		parse_array[b1 - b0] = '\0';
		return &parse_array[0];
	case SCRATCH:
		if (b0 - b1 + 1 > scratch->cap)
		{
			free(scratch->memory);
			scratch->size = 0;
			scratch->memory = malloc(b0 - b1 + 1024);
		}
		memmove(scratch->memory, str + b0, b1 - b0);
		scratch->memory[b1 - b0] = '\0';
		scratch->size = b1 - b0 + 1;
		return scratch->memory;
	}
	char* res = malloc(b1 - b0 + 1);
	strncpy(res, str + b0, b1 - b0);
	res[b1 - b0] = '\0';
	return res;
}

void rpc_init(void)
{
	parser = ts_parser_new();
	ts_parser_set_language(parser, tree_sitter_json());
	method_query = make_query(&request_tsquery[0], request_tsquery_len);
	params_query = make_query(&params_tsquery[0], params_tsquery_len);
	scratch = arena_init(1024);
	request_arena = arena_init(sizeof(Request));
}

void rpc_deinit(void)
{
	ts_query_delete(method_query);
	ts_query_delete(params_query);
	ts_parser_delete(parser);
	free(scratch->memory);
	free(scratch);
	free(request_arena->memory);
	free(request_arena);
}

void parse_params(char* json, int len, Request* request);

void parse_initialize(char* json, TSQueryCursor* cursor, TSQueryMatch* match, Request* request);

void parse_initialized(char* json, int len, Request* request);

void parse_shutdown(char* json, int len, Request* request);

void parse_exit(char* json, int len, Request* request);

void parse_method (char* json, int len, Request* request)
{
	TSTree *tree = ts_parser_parse_string(
		parser,
		NULL,
		json,
		len);
	TSQueryCursor* cursor = ts_query_cursor_new();
	ts_query_cursor_exec(cursor, method_query, ts_tree_root_node(tree));
	TSNode node;
	TSQueryMatch match;
	while(ts_query_cursor_next_match(cursor, &match))
	{
		// TODO switch
		if(match.pattern_index == 0)
		{
			// parse the id
			node = match.captures[0].node;
			char* text = node_text(&node, json, FIXED_ARRAY);
			memcpy(request->id, text, strlen(text) + 1);
			// parse the method name
			node = match.captures[1].node;
			text = node_text(&node, json, FIXED_ARRAY);
			if (strcmp(text, "initialize") == 0)
			{
				request->method = INITIALIZE;
				LOG("received an initialize request!");
			}
			else if (strcmp(text, "initialized") == 0)
			{
				request->method = INITIALIZED;
				LOG("received an initialized request!");
			}
			else if (strcmp(text, "shutdown") == 0)
			{
				request->method = SHUTDOWN;
				LOG("received a shutdown request!");
			}
			else if (strcmp(text, "exit") == 0)
			{
				request->method = EXIT;
				LOG("received an exit request!");
			}
			else
			{
				LOG("unrecognized method: %s", text);
				request->method = -1;
			}
			// parse method parameters
			node = match.captures[2].node;
			uint32_t b0 = ts_node_start_byte(node);
			uint32_t b1 = ts_node_end_byte(node);
			parse_params(json + b0, b1 - b0, request);
		}
	}
	ts_query_cursor_delete(cursor);
	ts_tree_delete(tree);
}

void parse_params(char* json, int len, Request* request)
{
	TSTree *tree = ts_parser_parse_string(
		parser,
		NULL,
		json,
		len);
	TSQueryCursor* cursor = ts_query_cursor_new();
	ts_query_cursor_exec(cursor, params_query, ts_tree_root_node(tree));
	TSNode node;
	TSQueryMatch match;
	switch(request->method)
	{
	case INITIALIZE:
		parse_initialize(json, cursor, &match, request);
		break;
	default:
	}
	ts_query_cursor_delete(cursor);
	ts_tree_delete(tree);
}

void parse_initialize(char* json, TSQueryCursor* cursor, TSQueryMatch* match, Request* request)
{
	TSNode node;
	while(ts_query_cursor_next_match(cursor, match))
	{
		node = match->captures[0].node;
		char* text = node_text(&node, json, FIXED_ARRAY);
		if (strcmp(text, "processId") == 0)
		{
			node = match->captures[1].node;
			text = node_text(&node, json, FIXED_ARRAY);
			LOG("jsonrcp processId: %s", text);
			uint32_t process_id = 0;
			int res = sscanf(text, "%ud", &process_id);
			LOG("parsed process_id in initialize parameters: %d", process_id);
			request->params.initialize.process_id = process_id;
		}
	}
}

void parse_initialized(char* json, int len, Request* request)
{
	/* TODO */
}

void parse_shutdown(char* json, int len, Request* request)
{
	/* TODO */
}

void parse_exit(char* json, int len, Request* request)
{
	/* TODO */
}



Request* rpc_parse(char* json, int len)
{
	request_arena->size = 0;
	Request* request = arena_alloc(request_arena, sizeof *request);
	parse_method(json, len, request);
	return request;
}
