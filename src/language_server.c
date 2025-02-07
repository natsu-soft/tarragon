#include "language_server.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <jansson.h>
#include "jsonrpc.h"
#include "base_protocol.h"
#include "logger.h"
#include "arena.h"

#define KILOBYTE 1024

static Arena* scratch = NULL;
/* static Arena* response_arena = NULL; */

static Lsp lsp_singleton = {
	.state = LSP_UNINITIALIZED
};

Lsp* lsp()
{
	return &lsp_singleton;
}

void respond(FILE* out, json_t* obj)
{
	char* content = json_dumps(obj, JSON_COMPACT | JSON_ENCODE_ANY);
	bp_send(out, content);
	bp_send(log_fp, content);
}

json_t* boilerplate()
{
	json_t* o = json_object();
	json_object_set(o, "jsonrpc", json_string("2.0"));
	return o;
}

json_t* add_id(json_t* o, const Rpc* rpc)
{
	uint32_t id;
	if (sscanf(&rpc->id[0], "%ul", &id))
	{
		json_object_set(o, "id", json_integer(id));
	}
	else
	{
		json_object_set(o, "id", json_string(&rpc->id[0]));
	}
	return o;
}

void handle_initialize(FILE* out, const Rpc* rpc)
{
	json_t* o = add_id(boilerplate(), rpc);
	json_t* result_o = json_object();
	json_object_set(result_o, "capabilties", json_object());
	json_t* info_o = json_object();
	json_object_set(info_o, "name", json_string("tarragon"));
	json_object_set(info_o, "version", json_string("0.0.lol"));
	json_object_set(result_o, "serverInfo", info_o);
	json_object_set(o, "result", result_o);
	LOG("request id: %s", &rpc->id[0]);
	LOG("initialize processId: %d", rpc->params.initialize.process_id);
	lsp_singleton.state = LSP_INITIALIZING;
	respond(out, o);
}

void handle_initialized(FILE* out, const Rpc* rpc)
{
	lsp_singleton.state = LSP_INITIALIZED;
	LOG("received an initialized notice!");
}

void handle_shutdown(FILE* out, const Rpc* rpc)
{
	lsp_singleton.state = LSP_SHUTDOWN;
	LOG("received a shutdown request!");
	json_t* o = json_object();
	json_object_set(o, "id", json_string(&rpc->id[0]));
	json_object_set(o, "result", json_null());
	respond(out, o);
}

void handle_exit(FILE* out, const Rpc* rpc)
{
	LOG("received an exit notice!");
	switch(lsp_singleton.state)
	{
	case LSP_SHUTDOWN:
		lsp_singleton.state = LSP_EXIT_0;
		break;
	default:
		lsp_singleton.state = LSP_EXIT_1;
	}
}


/* TOO COOL */
/* static void (*handle[])(FILE* out, Rpc* rpc) = { */
/* 	handle_initialize, */
/* 	handle_initialized, */
/* 	handle_shutdown, */
/* 	handle_exit, */
/* 	handle_unknown */
/* }; */

/*  NOTE: HOPE U DONT OVER ALLOCATE LMAO */
static void* scratch_alloc(size_t bytes)
{
	return arena_alloc(scratch, bytes);
}

static void scratch_free(void* memory)
{
	// lol
}

void handle(FILE* out, const Rpc* rpc)
{
	// why YES I am changing the json memory functions willy-nilly
	json_set_alloc_funcs(&scratch_alloc, &scratch_free);
	scratch->size = 0;
	// why? because it's based
	switch(rpc->method)
	{
	case INITIALIZE:
		handle_initialize(out, rpc);
		break;
	case INITIALIZED:
		handle_initialized(out, rpc);
		break;
	case SHUTDOWN:
		handle_shutdown(out, rpc);
		break;
	case EXIT:
		handle_exit(out, rpc);
		break;
	default:
		LOG("unknown method!");
	}
}


int ls_listen(FILE* in,FILE* out)
{
	bp_init();
	rpc_init();
	scratch = arena_init(KILOBYTE * sizeof(json_t));
	/* for(uint32_t i = 0; i < 10; ++i) */
	for(/* LOL */ ; /* LMAO */ ; /* EVEN */ )
	{
		/* LOG("processing %d of 10", i); */
		bp_reset();
		int32_t length = bp_receive_nosleep(in);
		/* length = bp_receive(in); */
		if (length > 0)
		{
			/* LOG("lsp listen: %s", bp_content()); */
			LOG("content: %s", bp_content());
			const Rpc* rpc = rpc_parse(bp_content(), length);
			if (!rpc)
			{
				LOG("jansson decode failure--shutting down!");
				rpc_deinit();
				free(scratch->memory);
				free(scratch);
				return 1;
			}
			handle(out, rpc);
			fflush(in);
			/* bp_send(out, "{ \"method\": \"window/showMessage\",\"type\": 3,\"message\":\"based\"}"); */
		}
		else if (length == -1) {
			LOG("eof encounterd in bp_receive()");
			rpc_deinit();
			free(scratch->memory);
			free(scratch);
			return 1;
		}
		if (lsp_singleton.state > LSP_SHUTDOWN)
		{
			rpc_deinit();
			free(scratch->memory);
			free(scratch);
			return lsp_singleton.state - LSP_EXIT_0;
		}
	}
}
