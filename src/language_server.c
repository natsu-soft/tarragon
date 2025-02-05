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

void lsp_response(FILE* out, char* id, char* result_or_error, bool result_flag)
{
	char* response = &response_scratch[0]; // JUST USE IT ALL FUCKIT
	if (result_flag)
	{
		sprintf(response, "{\"jsonrpc\":\"2.0\",\"id\":%s,\"result\":%s}", id, result_or_error);
	}
	else {
		sprintf(response, "{\"jsonrpc\":\"2.0\",\"id\":%s,\"error\":%s}", id, result_or_error);
	}
	bp_send(out, response);
	bp_send(log_fp, response);
}

void respond_initialize(FILE* out, char* id, InitializeParams* params)
{
	LOG("request id: %d", params->process_id);
	LOG("initialize processId: %d", params->process_id);
	lsp_singleton.state = LSP_INITIALIZING;
	lsp_response(out, id,   "{\"capabilities\":{},\"serverInfo\":{\"name\":\"TARRAGON\",\"version\":\"0.0.0-lmao\"}}", true);
	/* LOG("%s", response); */
}

void respond_initialized(FILE* out, char* id, InitializedParams* params)
{
	lsp_singleton.state = LSP_INITIALIZED;
	/* char* response =  "{\"capabilities\": {}, \"serverInfo\": {\"name\": \"TARRAGON\", \"version\": \"0.0.0-lmao\"}}"; */
	/* bp_send(out, response); */
	/* bp_send(log_fp, response); */
	/* LOG("%s", response); */
}

void respond_shutdown(FILE* out, char* id, ShutdownParams* params)
{
	LOG("received shutdown request!");
	lsp_singleton.state = LSP_SHUTDOWN;
}

void respond_exit(FILE* out, char* id, ExitParams* params)
{
	LOG("received an exit request!");
	switch(lsp_singleton.state)
	{
	case LSP_SHUTDOWN:
		lsp_singleton.state = LSP_EXIT_0;
		break;
	default:
		lsp_singleton.state = LSP_EXIT_1;
	}
}

void respond(FILE* out, Request* request)
{
	switch(request->method)
	{
	case INITIALIZE:
		respond_initialize(out, &request->id[0], &request->params.initialize);
		break;
	case INITIALIZED:
		respond_initialized(out, &request->id[0], &request->params.initialized);
		break;
	case SHUTDOWN:
		respond_shutdown(out, &request->id[0], &request->params.shutdown);
	case EXIT:
		respond_exit(out, &request->id[0], &request->params.exit);
	default:
		LOG("unrecognized request method: %d", request->method);
	}
}

int ls_listen(FILE* in,FILE* out)
{
	int32_t length = 0;
	/* static struct timespec eep = { .tv_nsec = 1000 }; */
	bp_init();
	rpc_init();
	for(uint32_t i = 0; i < 10; ++i)
	{
		LOG("processing %d of 10", i);
		bp_reset();
		/* length = bp_receive_nosleep(in); */
		length = bp_receive(in);
		if (length > 0)
		{
			/* LOG("lsp listen: %s", bp_content()); */
			LOG("content: %s", bp_content());
			respond(out, rpc_parse(bp_content(), length));
			/* bp_send(out, "{ \"method\": \"window/showMessage\",\"type\": 3,\"message\":\"based\"}"); */
		}
		else {
			LOG("eof encounterd in bp_receive()");
		}
		if (lsp_singleton.state > LSP_SHUTDOWN)
		{
			rpc_deinit();
			return lsp_singleton.state - LSP_EXIT_0;
		}
	}
}
