#include "language_server.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdbool.h>
#include "jsonrpc.h"
#include "base_protocol.h"
#include "logger.h"

static Lsp lsp_singleton = {
	.state = LSP_UNINITIALIZED
};

Lsp* lsp()
{
	return &lsp_singleton;
}

static char response_scratch[8192];

void respond(FILE* out, char* id, char* result_or_error, bool result_flag)
{
	char* response = &response_scratch[0]; // JUST USE IT ALL FUCKIT
	if (result_flag)
	{
		sprintf(response, "{\"jsonrpc\":\"2.0\",\"id\":%s,\"result\":%s}", id, result_or_error);
	}
	else {
		sprintf(response, "{\"jsonrpc\":\"2.0\",\"id\":%s,\"error\":%s}", id, result_or_error);}
	bp_send(out, response);
	fflush(out);
	bp_send(log_fp, response);
	fflush(log_fp);
}

void handle_initialize(FILE* out, Rpc* rpc)
{
	InitializeParams params = rpc->params.initialize;
	LOG("request id: %s", &rpc->id[0]);
	LOG("initialize processId: %d", params.process_id);
	lsp_singleton.state = LSP_INITIALIZING;
	respond(out, &rpc->id[0],   "{\"capabilities\":{},\"serverInfo\":{\"name\":\"TARRAGON\",\"version\":\"0.0.0-lmao\"}}", true);
	/* LOG("%s", response); */
}

void handle_initialized(FILE* out, Rpc* rpc)
{
	lsp_singleton.state = LSP_INITIALIZED;
}

void handle_shutdown(FILE* out, Rpc* rpc)
{
	lsp_singleton.state = LSP_SHUTDOWN;
	respond(out, &rpc->id[0], "null", true);
}

void handle_exit(FILE* out, Rpc* rpc)
{
	switch(lsp_singleton.state)
	{
	case LSP_SHUTDOWN:
		lsp_singleton.state = LSP_EXIT_0;
		break;
	default:
		lsp_singleton.state = LSP_EXIT_1;
	}
}

void handle_unknown(FILE* out, Rpc* rpc)
{
	// throw an error lmao
	LOG("unknown method");
}


static void (*handle[])(FILE* out, Rpc* rpc) = {
	handle_initialize,
	handle_initialized,
	handle_shutdown,
	handle_exit,
	handle_unknown
};

int ls_listen(FILE* in,FILE* out)
{
	bp_init();
	rpc_init();
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
			Rpc* rpc = rpc_parse(bp_content(), length);
			if (!rpc)
			{
				LOG("jansson decode failure--shutting down!");
				rpc_deinit();
				return 1;
			}
			handle[rpc->method](out, rpc);
			fflush(in);
			/* bp_send(out, "{ \"method\": \"window/showMessage\",\"type\": 3,\"message\":\"based\"}"); */
		}
		else if (length == -1) {
			LOG("eof encounterd in bp_receive()");
			rpc_deinit();
			return 1;
		}
		if (lsp_singleton.state > LSP_SHUTDOWN)
		{
			rpc_deinit();
			return lsp_singleton.state - LSP_EXIT_0;
		}
	}
}
