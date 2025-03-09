#ifndef JSON_H
#define JSON_H

#include <stdint.h>

#ifdef _WIN32
typedef struct _iobuf FILE;
#else
typedef struct _IO_FILE FILE;
#endif

// NOTE: pls dont lex

#define JSON_STRUCTURE_LIST	\
	JSON_X('{', JSON_OBJECT_START) \
	JSON_X('}', JSON_OBJECT_END) \
	JSON_X('[', JSON_ARRAY_START) \
	JSON_X(']', JSON_ARRAY_END) \
	JSON_X(':', JSON_NAME_SEP) \
	JSON_X(',', JSON_VALUE_SEP)

#define JSON_IDS								       \
	JSON_X(JSON_ERROR,        0x0000)	\
	JSON_X(JSON_OBJECT_START, 0x0004)	\
	JSON_X(JSON_OBJECT_END,   0x0008)	\
	JSON_X(JSON_ARRAY_START,  0x0010) \
	JSON_X(JSON_ARRAY_END,    0x0020)	\
	JSON_X(JSON_NAME_SEP,     0x0040)	\
	JSON_X(JSON_VALUE_SEP,    0x0080)	\
	JSON_X(JSON_NUMBER,       0x0100)	\
	JSON_X(JSON_INTEGER,      0x0101)	\
	JSON_X(JSON_FLOAT,        0x0102)	\
	JSON_X(JSON_STRING,       0x0200)	\
	JSON_X(JSON_NULL,         0x0400)	\
	JSON_X(JSON_BOOL,         0x0800)	\
	JSON_X(JSON_TRUE,         0x0801)	\
	JSON_X(JSON_FALSE,        0x0802)	\
	JSON_X(JSON_SUCCESS,      0x1000) \
	/* GOTTA GO FAST */ \
	JSON_X(JSON_KEY,   0x2000) \
	JSON_X(JSON_VALUE, 0x4000)
	/* JSON_X(JSON_ARRAY_ITEM,   0x8000) \ */


typedef enum json_id_t
{
	#define JSON_X(NAME, VALUE) NAME = VALUE,
	JSON_IDS
	#undef JSON_X
	JSON_NESTED_START = JSON_OBJECT_START | JSON_ARRAY_START,
	JSON_SINGLE_TOKEN_VALUE = JSON_NUMBER | JSON_STRING | JSON_NULL | JSON_BOOL,
	JSON_ALL_VALUES = JSON_NESTED_START | JSON_SINGLE_TOKEN_VALUE
} json_id_t;

// some useful shorthands that definitely wont confuse me later
#define JSON_OBJECT JSON_OBJECT_START
#define JSON_ARRAY JSON_ARRAY_START

typedef struct json_t
{
	char** tokens;
	json_id_t* ids;
	uint32_t num_tokens;
	uint32_t pos;
} json_t;

json_id_t json_parse(json_t* json, const char* src);

json_id_t json_next_key(json_t* json, char** key);

json_id_t json_next_value(json_t* json, json_t* value); // for both objects AND arrays

uint8_t json_c_value(void* dest, const json_t* json);

void json_free(json_t* json); // NOTE: have to free **tokens and *ids individually!

/* NOTE
 * These might come to bite me in the ass, but for now I like them.
 * The real test is whether or not I can _encode_ rpc responses easily
 * In which case, I see the value of either fprintf IMMEADIATELY &
 * introducing (object/array)(open/close) macros
 */

// NOTE: https://xkcd.com/297/
#define JS_OBJECT(BODY, ...) "{" BODY "}" __VA_ARGS__
#define JS_ARRAY(BODY, ...) "[" BODY "]" __VA_ARGS__
#define JS_MEMBER(KEY, VALUE, ...) #KEY ":" VALUE __VA_ARGS__
#define JS_VALUE(VALUE, ...) #VALUE __VA_ARGS__

#endif /* End JSON_H */
