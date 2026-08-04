#include "rpc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* #include <tree_sitter/api.h> */
#include "perfect_json.h"
#include "strings.h"
#include "error_macros.h"
/* NOTE: NOT DECREFING JSON STUFF BECAUSE IM BALLING */
#include "allocators/arena.h"
#include "allocators/scratch.h"
#include "allocators/workpad.h"
#include "logger.h"

static rpc_t request_buffer;

#define X(_, A) static void rpc_##A##_params(rpc_t* rpc, json_t* json);
RPC_METHODS
#undef X

static void rpc_unknown_params(rpc_t* rpc, json_t* json) {}
static void rpc_shutdown_params(rpc_t* rpc, json_t* json) {}
static void rpc_initialized_params(rpc_t* rpc, json_t* json) {}
static void rpc_exit_params(rpc_t* rpc, json_t* json) {}

// NOTE: assume the LSP client is to SPEC
static void parse_rpc (char* src, int len, rpc_t* rpc)
{
	/* LOG("parsing rpc"); */
	json_t json = {0};
	WORKPAD* workpad = workpad_checkout(0);
	json_id_t res = pj_parse(&json, workpad, src, len);
	/* if ((res & JSON_SUCCESS) != JSON_SUCCESS) */
	if (res != JSON_OBJECT)
	{
		ERROR("failed to decode:\n%.*s", len, src);
		return;
	}
	/* else if ((res & JSON_OBJECT) != JSON_OBJECT) */
	/* { */
	/* 	ERROR("rpc is not a json object:\n%s", str); */
	/* 	return; */
	/* } */
	string s;
	uint32_t key_len;
	json_t tmp = {0};
	rpc->id[0] = '\0';
	while(pj_next_pair(&s, &json, &tmp))
	{
		if (stringeq(STRING("id"), s))
		{
			res = pj_str(&s, &tmp);
			memcpy(rpc->id, s.chars, s.length);
			rpc->id[s.length] = '\0';
		}
		else if (stringeq(STRING("method"), s))
		{
			res = pj_str(&s, &tmp);
			PLAIN_LOG(" > rpc method: %.*s", s.length, s.chars);
			if(stringeq(STRING("initialize"), s))
			{
				rpc->method = RPC_INITIALIZE;
			}
			else if(stringeq(STRING("initialized"), s))
			{
				rpc->method = RPC_INITIALIZED;
			}
			else if(stringeq(STRING("shutdown"), s))
			{
				rpc->method = RPC_SHUTDOWN;
			}
			else if(stringeq(STRING("exit"), s))
			{
				rpc->method = RPC_EXIT;
			}
			else
			{
				ERROR("rpc contains an unknown method: %.*s", s.length, s.chars);
				rpc->method = RPC_UNKNOWN;
			}
		}
		else if (stringeq(STRING("params"), s))
		{
			switch(rpc->method)
			{
#define X(ENUM, A) case RPC_##ENUM: rpc_##A##_params(rpc, &tmp); break;
                        case RPC_INITIALIZE:
                          rpc_initialize_params(rpc, &tmp);
                          break;
                        case RPC_INITIALIZED:
                          rpc_initialized_params(rpc, &tmp);
                          break;
                        case RPC_SHUTDOWN:
                          rpc_shutdown_params(rpc, &tmp);
                          break;
                        case RPC_EXIT:
                          rpc_exit_params(rpc, &tmp);
                          break;
                        case RPC_UNKNOWN:
                          rpc_unknown_params(rpc, &tmp);
                          break;
#undef X
			}
		}
	}
	if (rpc->method == NO_RPC_METHOD)
	{
		ERROR("rpc does not contain a method key!");
		return;
	}
}

static void rpc_initialize_params(rpc_t* rpc, json_t* json)
{
	// the process with PID 0 is responsible for paging, ergo it is NOT the parent process id (so keep using ur unsigned int u degenerate)
	json_id_t res = {0};
	string s;
	json_t tmp = {0};
	while ((res = pj_next_pair(&s, json, &tmp)))
	{
		if (stringeq(STRING("processId"), s))
		{
			// TODO
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
