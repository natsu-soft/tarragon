#ifndef JSON_H
#define JSON_H

#include <stdint.h>

#ifdef _WIN32
typedef struct _iobuf FILE;
#else
typedef struct _IO_FILE FILE;
#endif

// NOTE: pls dont lex

enum
{
	JSON_ERROR = 0x00,
	JSON_OBJECT,
	JSON_ARRAY,
	JSON_STRING,
	JSON_NUMBER,
	JSON_NULL,
	JSON_END_OBJECT = 10,
	JSON_END_ARRAY,
	JSON_READY = 69,
	JSON_SUCCESS = 99
};



typedef struct json_t
{
	struct tokens_t* tokens;
	uint32_t length;
} json_t;



#endif /* End JSON_H */
