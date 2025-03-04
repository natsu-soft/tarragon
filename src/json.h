#ifndef JSON_H
#define JSON_H

#include <stdint.h>
#include "list.h"

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


typedef struct json_tokens_t
{
	struct list_t list;
	char* value;
} json_tokens_t;

typedef struct json_t
{
	json_tokens_t* tokens;
	uint32_t length;
} json_t;


int8_t json_parse_filep(FILE* fp, json_t* json);

int8_t json_parse_str(char* src, json_t* json, const int n);

int8_t next_elem(json_t* json);


int8_t json_lex(char* src, json_t* json, const int n);


#endif /* End JSON_H */
