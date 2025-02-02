#ifndef RPC_H
#define RPC_H

#include <stdint.h>

/* #include <stdint.h> */

/* typedef unsigned int uint32_t; */

#ifdef _WIN32
typedef struct _iobuf FILE;
#else
typedef struct _IO_FILE FILE;
#endif

char* rpc_message(void);

void rpc_recycle(void);

void rpc_encode(FILE* stream, char* content);

uint32_t rpc_decode(FILE* stream);


#endif /* End RPC_H */
