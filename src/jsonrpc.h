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


typedef enum method_t
{
	INITIALIZE = 0x00,
	INITIALIZED,
	SHUTDOWN,
	EXIT,
	METHOD_UNKNOWN
} method_t;


typedef struct initialize_params_t
{
	uint32_t process_id; // null represented w/ zero
} initialize_params_t;

/* these all have zero parameters so DONT FUCCN BOTHER TBQH */
/* typedef struct initializezd_params_t { */
/* } initialized_params_t; */

/* typedef struct shutdown_params_t { */
/* } shutdown_params_t; */

/* typedef struct exit_params_t { */
/* } exit_params_t; */

typedef struct rpc_t
{
	union
	{
		initialize_params_t initialize;
	} params; // I imagine this could easily get bigger than id EVENTUALLY
	char id[16]; // MASSIVE
	method_t method;
} rpc_t;

void rpc_init(void);

void rpc_deinit(void);

const rpc_t* rpc_parse(char* json, int len);

#endif /* End JSONRPC_H */
