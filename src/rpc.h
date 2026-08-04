#ifndef RPC_H
#define RPC_H

#include <stdint.h>

enum
{
	RPC_PARSE_ERROR = -32700,
	RPC_INVALID_REQUEST = -32600,
	RPC_METHOD_NOT_FOUND = -32601,
	RPC_INVALID_PARAMS = -32602,
	RPC_INTERNAL_ERROR = -32603,
};

#define RPC_METHODS \
	X(INITIALIZE, initialize) \
	X(INITIALIZED, initialized) \
	X(SHUTDOWN, shutdown) \
	X(EXIT, exit) \
	X(UNKNOWN, unknown)



enum rpc_method
{
	NO_RPC_METHOD = 0x00,
#define X(A, ...) RPC_##A,
	RPC_METHODS
#undef X
};


typedef struct rpc_t
{
	char id[16]; // MASSIVE
	enum rpc_method method;
	union
	{
		// initialize
		struct
		{
			uint32_t process_id;
		};
	} params; // I imagine this could easily get bigger than id EVENTUALLY
} rpc_t;


#warning wtf are these?
void rpc_init(void);

void rpc_deinit(void);

const rpc_t* rpc_parse(char* json, int len);

#endif /* End RPC_H */
