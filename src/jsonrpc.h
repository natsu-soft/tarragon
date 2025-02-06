#ifndef JSONRPC_H
#define JSONRPC_H

#include <stdint.h>

enum
{
	RPC_PARSE_ERROR = -32700,
	RPC_INVALID_REQUEST = -32600,
	RPC_METHOD_NOT_FOUND = -32601,
	RPC_INVALID_PARAMS = -32602,
	RPC_INTERNAL_ERROR = -32603,
};


typedef enum
{
	INITIALIZE = 0x00,
	INITIALIZED,
	SHUTDOWN,
	EXIT,
	METHOD_UNKNOWN
} Method;


typedef struct InitializeParams
{
	uint32_t process_id;
} InitializeParams;

typedef struct InitializedParams {
} InitializedParams;

typedef struct ShutdownParams {
} ShutdownParams;

typedef struct ExitParams {
} ExitParams;

typedef struct Rpc
{
	char id[16]; // MASSIVE
	union
	{
		InitializeParams initialize;
		InitializedParams initialized;
		ShutdownParams shutdown;
		ExitParams exit;
	} params;
	Method method;
} Rpc;

void rpc_init(void);

void rpc_deinit(void);

Rpc* rpc_parse(char* json, int len);

#endif /* End JSONRPC_H */
