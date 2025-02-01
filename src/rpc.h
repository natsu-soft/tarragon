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

void rpc_encode(FILE* stream, char* content);

void rpc_decode(FILE* stream, uint32_t* content_len, char** content);


#endif /* End RPC_H */
