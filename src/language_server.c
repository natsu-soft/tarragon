#include "language_server.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include "error_macros.h"
#include "perfect_json.h"
#include "rpc.h"
#include "file_sys.h"
#include "base_protocol.h"
#include "logger.h"
#include "allocators/arena.h"
#include "allocators/scratch.h"
#include "allocators/workpad.h"
#include "strings.h"

#define KILOBYTE 1024

	/* JS_OBJECT(JS_MEMBER(jsonrpc, "2.0", COMMA) JS_MEMBER_VAR(id, rpc->id, COMMA) BODY) */
#define JSON_RPC(...)													\
	PJ_OBJ( \
		PJ_MEMBER(jsonrpc, PJ_LIT("2.0"));		\
		PJ_MEMBER(id, PJ_STR(rpc->id));			\
		__VA_ARGS__)

/* extern unsigned char default_config_json[]; */
/* extern unsigned int default_config_json_len; */
/* static Arena* response_arena = NULL; */

/* static lsp_t lsp_singleton = { */
/* 	.state = LSP_UNINITIALIZED, */
/* 	.documents = NULL, */
/* 	.out = NULL, */
/* 	.num_documents = 0 */
/* }; */



static struct
{
	FILE* out;
	lsp_state_t state;
} lsp = { .state = LSP_UNINITIALIZED };

/* lsp_t* lsp(void) */
/* { */
/* 	return &lsp_singleton; */
/* } */

static void respond(FILE* out, char* msg, const uint32_t n)
{
	bp_sendn(out, msg, n);
	fflush(out);
}

// I do NOT thing this one works (mb just not with eglot)
void log_client(char* message)
{
	/* json_t* o = boilerplate(); */
	/* json_object_set(o, "method", json_string("window/logMessage")); */
	/* json_t* params_o = json_object(); */
	/* json_object_set(params_o, "type", json_integer(4)); */
	/* json_object_set(params_o, "message", json_string(message)); */
	/* json_object_set(o, "params", params_o); */
	/* respond(lsp_singleton.out, o); */
}


static void handle_initialize(FILE* out, const rpc_t* rpc)
{
	PLAIN_LOG(" > handle_initialize()");
	arena_t* scratch = scratch_arena(0);
	#define JSON_DEST out
	JSON_RPC(PJ_MEMBER(capabilities, PJ_OBJ())
			 PJ_LAST_MEMBER(serverInfo, PJ_OBJ(
								PJ_MEMBER(name, PJ_LIT("tarragon"))
								PJ_LAST_MEMBER("version", PJ_STR("0")))));
	fflush(JSON_DEST);
	#undef JSON_DEST
	/* string msg = DUMP_JSON; */
	/* #undef JS_ARENA */
	/* LOG("initialize response: %s\n", msg); */
	/* respond(out, msg, scratch->size - 1); */
	lsp.state = LSP_INITIALIZING;
}

static void handle_initialized(FILE* out, const rpc_t* rpc)
{
	PLAIN_LOG(" > handle_initialized()");
	lsp.state = LSP_INITIALIZED;
	/* log_client("tarragon is initialized!"); */
}

static void handle_shutdown(FILE* out, const rpc_t* rpc)
{
	PLAIN_LOG(" > handle_shutdown()");
	lsp.state = LSP_SHUTDOWN;
	/* json_t* o = add_id(boilerplate(), rpc); */
	/* json_object_set(o, "result", json_null()); */
	/* respond(out, o); */
	/* log_client("tarragon shutting down NOW!"); */
	// just shutdown tbqh
	lsp.state = LSP_EXIT_0;
}

static void handle_exit(FILE* out, const rpc_t* rpc)
{
	PLAIN_LOG(" > handle_exit()");
	switch(lsp.state)
	{
	case LSP_SHUTDOWN:
		lsp.state = LSP_EXIT_0;
		break;
	default:
		lsp.state = LSP_EXIT_1;
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

static void handle_rpc(FILE* out, const rpc_t* rpc)
{
	// why YES I am changing the json memory functions willy-nilly
	/* json_set_alloc_funcs(&scratch_alloc, &scratch_free); */
	/* scratch.size = 0; */
	// why? because it's based
	switch(rpc->method)
	{
	case RPC_INITIALIZE:
		handle_initialize(out, rpc);
		break;
	case RPC_INITIALIZED:
		handle_initialized(out, rpc);
		break;
	case RPC_SHUTDOWN:
		handle_shutdown(out, rpc);
		break;
	case RPC_EXIT:
		handle_exit(out, rpc);
		break;
	/* default: */
		/* LOG("unknown method!"); */
	}
}



int ls_listen(FILE* in, FILE* out)
{
	/* arena_init(&scratch, KILOBYTE * sizeof(json_t)); */
	lsp.out = out;
	/* for(uint32_t i = 0; i < 10; ++i) */
	for(/* LOL */ ; /* LMAO */ ; /* EVEN */ )
	{
		workpad_checkin_all();
		/* LOG("processing %d of 10", i); */
		char* client_json;
		fflush(in);
		int32_t length = bp_receive_nosleep(in, &client_json);
		/* length = bp_receive(in); */
		if (length > 0)
		{
			char* content = scratch_alloc(length + 1);
			memcpy(content, client_json, length);
			content[length] = '\0';
			/* LOG("lsp listen: %s", bp_content()); */
			LOG("client content: %s", content);
			const rpc_t* rpc = rpc_parse(client_json, length);
			if (!rpc)
			{
				ERROR("json parse failure--shutting down!");
				return 1;
			}
			handle_rpc(out, rpc);
			/* bp_send(out, "{ \"method\": \"window/showMessage\",\"type\": 3,\"message\":\"based\"}"); */
		}
		else if (length == -1) {
			LOG("eof encounterd in bp_receive()");
			lsp.state = LSP_EXIT_1;
		}
		if (lsp.state > LSP_SHUTDOWN)
		{
			LOG("exiting with code: %d", lsp.state - LSP_EXIT_0);
			return lsp.state - LSP_EXIT_0;
		}
	}
	return 1;
}

/* char* ls_generate_config(const char* dir) */
/* { */
/* 	char* config_name = fs_join(dir, CONFIG_NAME); */
/* 	fs_write(config_name, (char*)&default_config_json[0]); */
/* 	return config_name; */
/* } */
