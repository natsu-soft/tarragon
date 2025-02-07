#include "jsonrpc.h"
#include <stdio.h>
#include <string.h>
/* #include <tree_sitter/api.h> */
#include <jansson.h>
/* NOTE: NOT DECREFING JSON STUFF BECAUSE IM BALLING */
#include "arena.h"
#include "logger.h"

#define KILOBYTE 1024
#define MEGABYTE 1048576


static arena_t* scratch = NULL;
static arena_t* request_arena = NULL;

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
	scratch = arena_init(MEGABYTE * sizeof(json_t));
	request_arena = arena_init(sizeof(rpc_t));
}

void rpc_deinit(void)
{
	free(scratch->memory);
	free(scratch);
	free(request_arena->memory);
	free(request_arena);
}

void initialize_params(json_t* params_o, rpc_t* rpc);

void parse_rpc (char* str, int len, rpc_t* rpc)
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
}

void initialize_params(json_t* params_o, rpc_t* rpc)
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

const rpc_t* rpc_parse(char* json, int len)
{
	json_set_alloc_funcs(&scratch_alloc, &scratch_free);
	scratch->size = 0;
	request_arena->size = 0;
	rpc_t* rpc = arena_alloc(request_arena, sizeof *rpc);
	parse_rpc(json, len, rpc);
	return rpc;
}
