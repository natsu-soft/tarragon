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

char* bp_content(void);

void bp_init(void);

void bp_send(FILE* stream, char* content);

int32_t bp_receive(FILE* stream);


#endif /* End RPC_H */
