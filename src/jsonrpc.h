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
	ID_NULL,
	ID_INTEGER,
	ID_STRING,
} IdType;

typedef enum
{
	INITIALIZE = 0x00,
	INITIALIZED,
	SHUTDOWN,
	EXIT,
	METHOD_COUNT
} RequestMethod;


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

typedef struct Request
{
	char id[32]; // MASSIVE
	union
	{
		InitializeParams initialize;
		InitializedParams initialized;
		ShutdownParams shutdown;
		ExitParams exit;
	} params;
	RequestMethod method;
} Request;

void rpc_init(void);

void rpc_deinit(void);

Request* rpc_parse(char* json, int len);

#endif /* End JSONRPC_H */
