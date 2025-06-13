#include "language_server.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include "error_macros.h"
#include "json.h"
#include "rpc.h"
#include "file_sys.h"
#include "base_protocol.h"
#include "logger.h"
#include "arena.h"
#include "scratch.h"
#include "json.h"

#define KILOBYTE 1024

	/* JS_OBJECT(JS_MEMBER(jsonrpc, "2.0", COMMA) JS_MEMBER_VAR(id, rpc->id, COMMA) BODY) */
#define JSON_RPC(BODY)													\
	arena_dynamic_push(JS_ARENA, "{\"jsonrpc\":\"2.0\",\"id\":", 22);	\
	JS_VAR(rpc->id, COMMA)										\
	BODY															\
	arena_dynamic_push(JS_ARENA, "}", 1);							\

extern unsigned char default_config_json[];
extern unsigned int default_config_json_len;
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
	bp_sendn(log_fp, msg, n);
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


void handle_initialize(FILE* out, const rpc_t* rpc)
{
	LOG("received an initialize request!");
	arena_t* scratch = scratch_checkout();
	LOG("scratch: %x, allocation: %x", scratch, scratch->allocation);
	#define JS_ARENA scratch
/* 	JSON_RPC( */
/* 		JS_MEMBER_MACRO(capabilities, JS_OBJECT(), COMMA) */
/* 		JS_MEMBER_MACRO(serverInfo, JS_OBJECT( */
/* 							JS_MEMBER(name, "tarragon") */
/* 							JS_MEMBER(version, "0.0.lmao") */
/* 							))) */
/* 	/\* respond(out, JS_DUMP, scratch->size - 1); *\/ */
/* 		LOG("intiialize response:\n%s", JS_DUMP); */
/* #undef JS_ARENA */
	/* JS_OBJECT() */
		/* char* bar = JS_DUMP; */
	#undef JS_ARENA
        LOG("foo!");
	lsp.state = LSP_INITIALIZING;
	/* scratch_checkin(scratch); */
}

void handle_initialized(FILE* out, const rpc_t* rpc)
{
	lsp.state = LSP_INITIALIZED;
	LOG("received an initialized notice!");
	/* log_client("tarragon is initialized!"); */
}

void handle_shutdown(FILE* out, const rpc_t* rpc)
{
	lsp.state = LSP_SHUTDOWN;
	LOG("received a shutdown request!");
	/* json_t* o = add_id(boilerplate(), rpc); */
	/* json_object_set(o, "result", json_null()); */
	/* respond(out, o); */
	/* log_client("tarragon shutting down NOW!"); */
	// just shutdown tbqh
	lsp.state = LSP_EXIT_0;
}

void handle_exit(FILE* out, const rpc_t* rpc)
{
	LOG("received an exit notice!");
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

void handle(FILE* out, const rpc_t* rpc)
{
	// why YES I am changing the json memory functions willy-nilly
	/* json_set_alloc_funcs(&scratch_alloc, &scratch_free); */
	/* scratch.size = 0; */
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



int ls_listen(FILE* in, FILE* out)
{
	/* arena_init(&scratch, KILOBYTE * sizeof(json_t)); */
	lsp.out = out;
	/* for(uint32_t i = 0; i < 10; ++i) */
	for(/* LOL */ ; /* LMAO */ ; /* EVEN */ )
	{
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
			handle(out, rpc);
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

char* ls_generate_config(const char* dir)
{
	char* config_name = fs_join(dir, CONFIG_NAME);
	fs_write(config_name, (char*)&default_config_json[0]);
	return config_name;
}

int language_server_test(int argc, char** argv)
{
	printf("testing language server functionality\n");
	char* cwd = fs_dirname(argv[0]);
	file_context_t* context = fs_find_upward("bruh", cwd);
	printf("testing config file generation\n");
	int config_len = 0;
	char* default_config_file = fs_abs_path("default_config.json");
	assertf(default_config_file, "default config file DNE!");
	#ifdef _WIN32
	printf("WINDOWS_HACK:\nthis is the baked string for the default config json:\n%s\n", &default_config_json); // the assert test does not work on windows, BUT AS WE CAN SEE IT OUTPUTS PROPERLY
	printf("does it look good?\n");
	#else
	assertf(!strcmp(fs_read(default_config_file, &config_len), &default_config_json[0]), "default config file does not match hex dump!");
	#endif
	char* config_path = ls_generate_config(context->dir);
	file_context_t* config_context = fs_find_upward(CONFIG_NAME, cwd);
	assertf(config_context, "find_upwards failed to find %s!", CONFIG_NAME);
	return 0;
}
