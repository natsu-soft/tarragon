#include "rpc.h"
#include <stdio.h>
#include <string.h>
/* #include <tree_sitter/api.h> */
#include "json.h"
#include "error_macros.h"
/* NOTE: NOT DECREFING JSON STUFF BECAUSE IM BALLING */
#include "arena.h"
#include "scratch.h"
#include "logger.h"

static rpc_t request_buffer;

static void initialize_params(rpc_t* rpc, json_t* json);

// NOTE: assume the LSP client is to SPEC
static void parse_rpc (char* str, int len, rpc_t* rpc)
{
	json_t json = {0};
	SCRATCH* scratch = scratch_checkout();
	json_id_t res = json_parse(&json, scratch, str, len);
	if ((res & JSON_SUCCESS) != JSON_SUCCESS)
	{
		ERROR("failed to decode:\n%s", str);
		return;
	}
	else if ((res & JSON_OBJECT) != JSON_OBJECT)
	{
		ERROR("rpc is not a json object:\n%s", str);
		return;
	}
	char* key;
	json_t tmp = {0};
	rpc->id[0] = '\0';
	while ((res = json_next_key(&json, &key)) & JSON_KEY)
	{
		if (strncmp(key, "id", 2) == 0)
		{
			res = json_next_value(&json, &tmp); // assume the LSP client is to spec.
			memcpy(rpc->id, *tmp.tokens, strlen(*tmp.tokens));
		}
		else if (strncmp(key, "method", 6) == 0)
		{
			res = json_next_value(&json, &tmp); // assume the LSP client is to spec.
			char* method;
			assertf(json_c(&method, &tmp), "Failed to convert token \"%s\" to a char*\n", tmp.tokens[0]);
			if (strncmp(method, "initialize", 10) == 0)
			{
				rpc->method = INITIALIZE;
			}
			else if (strncmp(method, "initialized", 11) == 0)
			{
				rpc->method = INITIALIZED;
			}
			else if (strncmp(method, "shutdown", 8) == 0)
			{
				rpc->method = SHUTDOWN;
			}
			else if (strncmp(method, "exit", 4) == 0)
			{
				rpc->method = EXIT;
			}
			else
			{
				rpc->method = UNKNOWN;
			}
		}
		else if (strncmp(key, "params", 6) == 0)
		{
			res = json_next_value(&json, &tmp);
			switch(rpc->method)
			{
			case INITIALIZE:
				initialize_params(rpc, &tmp);
				break;
			default: break;
			}
		}
	}
	if (rpc->id[0] == '\0')
	{
		ERROR("rpc does not contain an id!");
		return;
	}
	if (rpc->method == NO_METHOD)
	{
		ERROR("rpc does not contain a method key!");
		return;
	}
	else if (rpc->method == UNKNOWN)
	{
		ERROR("rpc contains an unknown method!");
		return;
	}
	// TODO: param checking.
}

static void initialize_params(rpc_t* rpc, json_t* json)
{
	// the process with PID 0 is responsible for paging, ergo it is NOT the parent process id (so keep using ur unsigned int u degenerate)
	json_id_t res = {0};
	char* key;
	json_t tmp = {0};
	while ((res = json_next_key(json, &key)) & JSON_KEY)
	{
		if (strncmp(key, "processId", 9) == 0)
		{
			res = json_next_value(json, &tmp);
			json_c(&rpc->params.process_id, &tmp);
		}
	}
}

const rpc_t* rpc_parse(char* json, int len)
{
	memset(&request_buffer, 0x00, sizeof request_buffer);
	request_buffer.id[0] = '\0';
	parse_rpc(json, len, &request_buffer);
	return &request_buffer;
}
