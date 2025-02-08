#include "language_server.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <jansson.h>
#include "jsonrpc.h"
#include "file_sys.h"
#include "base_protocol.h"
#include "logger.h"
#include "arena.h"

#define KILOBYTE 1024

extern unsigned char default_config_json[];
extern unsigned int default_config_json_len;

static arena_t* scratch = NULL;
/* static Arena* response_arena = NULL; */

static lsp_t lsp_singleton = {
	.state = LSP_UNINITIALIZED,
	.documents = NULL,
	.out = NULL,
	.num_documents = 0
};

lsp_t* lsp(void)
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

json_t* add_id(json_t* o, const rpc_t* rpc)
{
	// NOTE: SPEED IS LIFE
	// rpc->id[0] ^ '\0'
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

// I do NOT thing this one works (mb just not with eglot)
void log_client(char* message)
{
	scratch->size = 0;
	json_t* o = boilerplate();
	json_object_set(o, "method", json_string("window/logMessage"));
	json_t* params_o = json_object();
	json_object_set(params_o, "type", json_integer(4));
	json_object_set(params_o, "message", json_string(message));
	json_object_set(o, "params", params_o);
	respond(lsp_singleton.out, o);
}


void handle_initialize(FILE* out, const rpc_t* rpc)
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

void handle_initialized(FILE* out, const rpc_t* rpc)
{
	lsp_singleton.state = LSP_INITIALIZED;
	LOG("received an initialized notice!");
	/* log_client("tarragon is initialized!"); */
}

void handle_shutdown(FILE* out, const rpc_t* rpc)
{
	lsp_singleton.state = LSP_SHUTDOWN;
	LOG("received a shutdown request!");
	json_t* o = add_id(boilerplate(), rpc);
	json_object_set(o, "result", json_null());
	respond(out, o);
	/* log_client("tarragon shutting down NOW!"); */
	// just shutdown tbqh
	lsp_singleton.state = LSP_EXIT_0;
}

void handle_exit(FILE* out, const rpc_t* rpc)
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

void handle(FILE* out, const rpc_t* rpc)
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

void cleanup(void)
{
	rpc_deinit();
	free(scratch->memory);
	free(scratch);
}


int ls_listen(FILE* in,FILE* out)
{
	bp_init();
	rpc_init();
	scratch = arena_init(KILOBYTE * sizeof(json_t));
	lsp_singleton.out = out;
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
			LOG("content %s", bp_content());
			const rpc_t* rpc = rpc_parse(bp_content(), length);
			if (!rpc)
			{
				LOG("jansson decode failure--shutting down!");
				cleanup();
				return 1;
			}
			handle(out, rpc);
			fflush(in);
			/* bp_send(out, "{ \"method\": \"window/showMessage\",\"type\": 3,\"message\":\"based\"}"); */
		}
		else if (length == -1) {
			LOG("eof encounterd in bp_receive()");
			lsp_singleton.state = LSP_EXIT_1;
		}
		if (lsp_singleton.state > LSP_SHUTDOWN)
		{
			cleanup();
			LOG("exiting with code: %d", lsp_singleton.state - LSP_EXIT_0);
			return lsp_singleton.state - LSP_EXIT_0;
		}
	}
}

char* ls_generate_config(const char* dir)
{
	char* config_name = fs_join(dir, CONFIG_NAME);
	fs_write(config_name, (char*)&default_config_json[0]);
	return config_name;
}
