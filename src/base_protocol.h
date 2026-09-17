#ifndef BASE_PROTOCOL_H
#define BASE_PROTOCOL_H

#include <stdint.h>

/* #include <stdint.h> */

/* typedef unsigned int uint32_t; */

#ifdef _WIN32
typedef struct _iobuf FILE;
#else
typedef struct _IO_FILE FILE;
#endif

typedef struct arena_t arena_t;

/* void bp_reset(void); */

void bp_init(void);

const uint32_t bp_send(FILE* stream, const char* content);

const uint32_t bp_sendn(FILE* stream, const char* content, const uint32_t n);

int32_t bp_receive_nosleep(FILE* stream, char** out_json);

void echo(uint32_t n, FILE* in, FILE* out);


#endif /* End BASE_PROTOCOL_H */
