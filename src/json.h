#ifndef JSON_H
#define JSON_H

#include <stdint.h>
#include <string.h>
#include "token.h"

#ifdef _WIN32
typedef struct _iobuf FILE;
#else
typedef struct _IO_FILE FILE;
#endif

#define JSON_STRUCTURE_LIST	\
	X('{', JSON_OBJECT_START) \
	X('}', JSON_OBJECT_END) \
	X('[', JSON_ARRAY_START) \
	X(']', JSON_ARRAY_END) \
	X(':', JSON_NAME_SEP) \
	X(',', JSON_VALUE_SEP)

#define JSON_IDS								       \
	X(JSON_ERROR,        0x0000)	\
	X(JSON_OBJECT_START, 0x0004)	\
	X(JSON_OBJECT_END,   0x0008)	\
	X(JSON_ARRAY_START,  0x0010)   \
	X(JSON_ARRAY_END,    0x0020)	\
	X(JSON_NAME_SEP,     0x0040)	\
	X(JSON_VALUE_SEP,    0x0080)	\
	X(JSON_NUMBER,       0x0100)	\
	X(JSON_INTEGER,      0x0101)	\
	X(JSON_FLOAT,        0x0102)	\
	X(JSON_STRING,       0x0200)	\
	X(JSON_NULL,         0x0400)	\
	X(JSON_BOOL,         0x0800)	\
	X(JSON_TRUE,         0x0801)	\
	X(JSON_FALSE,        0x0802)	\
	X(JSON_SUCCESS,      0x1000) \
	/* GOTTA GO FAST */ \
	X(JSON_KEY,   0x2000) \
	X(JSON_VALUE, 0x4000)
	/* JSON_X(JSON_ARRAY_ITEM,   0x8000) \ */


typedef enum json_id_t
{
	#define X(NAME, VALUE) NAME = VALUE,
	JSON_IDS
	#undef X
	JSON_NESTED_START = JSON_OBJECT_START | JSON_ARRAY_START,
	JSON_SINGLE_TOKEN_VALUE = JSON_NUMBER | JSON_STRING | JSON_NULL | JSON_BOOL,
	JSON_ALL_VALUES = JSON_NESTED_START | JSON_SINGLE_TOKEN_VALUE
} json_id_t;

// some useful shorthands that definitely wont confuse me later
#define JSON_OBJECT JSON_OBJECT_START
#define JSON_ARRAY JSON_ARRAY_START

typedef struct json_t
{
	token_t* tokens;
	json_id_t* ids;
	uint32_t num_tokens;
	uint32_t pos;
} json_t;

typedef struct arena_t arena_t;

json_id_t json_parse(json_t* json, arena_t* arena, const char* src, const uint32_t len);

json_id_t json_next_key(json_t* json, char** key, uint32_t* key_len);

// NOTE: USE STRNCMP OTHERWISE UR TURBO FUCKED.
json_id_t json_next_value(json_t* json, json_t* value); // for both objects AND arrays

// NOTE: uses a scratch backing for a string, so memcpy it if you need it elsewhere
uint8_t json_c(void* dest, const json_t* json);



/* NOTE
 * These might come to bite me in the ass, but for now I like them.
 * The real test is whether or not I can _encode_ rpc responses easily
 * In which case, I see the value of either fprintf IMMEADIATELY &
 * introducing (object/array)(open/close) macros
 */

// NOTE: https://xkcd.com/297/
/* #define JS_OBJECT(BODY, ...) "{" BODY "}" __VA_ARGS__ */
/* #define JS_ARRAY(BODY, ...) "[" BODY "]" __VA_ARGS__ */
/* #define JS_MEMBER(KEY, VALUE, ...) #KEY ":" VALUE __VA_ARGS__ */
/* #define JS_VALUE(VALUE, ...) #VALUE __VA_ARGS__ */


#define COMMA arena_dynamic_push(JS_ARENA, ",", 1);

#define JS_OBJECT(BODY, ...)					\
	arena_dynamic_push(JS_ARENA, "{", 1);		\
	BODY										\
	arena_dynamic_push(JS_ARENA, "}", 1);		\
	__VA_ARGS__

#define JS_ARRAY(BODY, ...)						\
	arena_dynamic_push(JS_ARENA, "[", 1);		\
	BODY										\
	arena_dynamic_push(JS_ARENA, "]", 1);		\
	__VA_ARGS__

#define JS_VAL(VALUE, ...)								\
	arena_dynamic_push(JS_ARENA, #VALUE, strlen(#VALUE));	\
	__VA_ARGS__

#define JS_VAR(VAR, ...)						\
	arena_dynamic_push(JS_ARENA, VAR, strlen(VAR)); \
	__VA_ARGS__

#define JS_MEMBER(KEY, VALUE, ...)											\
	arena_dynamic_push(JS_ARENA, "\"" #KEY "\":", strlen(#KEY) + 3);	\
	arena_dynamic_push(JS_ARENA, #VALUE, strlen(#VALUE));	\
	__VA_ARGS__

#define JS_MEMBER_MACRO(KEY, MACRO, ...)								\
	arena_dynamic_push(JS_ARENA, "\"" #KEY "\":", strlen(#KEY) + 3);	\
	MACRO																\
	__VA_ARGS__

#define JS_MEMBER_VAR(KEY, VAR, ...)										\
	arena_dynamic_push(JS_ARENA, "\"" #KEY "\":", strlen(#KEY) + 3);	\
	arena_dynamic_push(JS_ARENA, VAR, strlen(VAR)); \
	__VA_ARGS__

#define JS_TERMINATE (arena_dynamic_push(JS_ARENA, "\0", 1), JS_ARENA->allocation)


#endif /* End JSON_H */
