#ifndef PERFECT_JSON_H
#define PERFECT_JSON_H

#include <stdint.h>

#ifdef _WIN32
typedef struct _iobuf FILE;
#else
typedef struct _IO_FILE FILE;
#endif

#define JSON_STRUCTURE_LIST	\
	X(COMMA,		',')	\
	X(COLON,		':')	\
	X(OBJECT_START, '{')	\
	X(OBJECT_END,	'}')	\
	X(ARRAY_START,	'[')	\
	X(ARRAY_END,	']')

#define JSON_IDS \
	JSON_STRUCTURE_LIST \
	X(INTEGER)  /* 0x07 */ \
	X(FLOAT) /* 0x08 */ \
	X(STRING) /* 0x09 */ \
	X(NULL) /* 0x0a */ \
	X(TRUE)  /* 0x0b */ \
	X(FALSE) /* 0x0c */


typedef enum __attribute__((packed)) json_id_t
{
	JSON_ERROR = 0x00,
	#define X(NAME, ...) JSON_##NAME,
	JSON_IDS
	#undef X
	JSON_NUMBER = 0x10,
	JSON_BOOLEAN = 0x20,
	JSON_VALUE = 0x40,
	JSON_KEY = 0x80,
} json_id_t;

// some useful shorthands that definitely wont confuse me later
#define JSON_OBJECT JSON_OBJECT_START
#define JSON_ARRAY JSON_ARRAY_START

typedef struct id_stack id_stack;

typedef struct json_t
{
	char* src;
	int32_t* offsets;
	uint8_t* bitmasks;
	int32_t num_tokens;
	int32_t index;
} json_t;

typedef struct arena_t arena_t;

void pj_allman(const json_t* json);

json_id_t pj_parse(json_t* json, arena_t* arena, const char* src, const int32_t len);

uint8_t pj_next_key(json_t* json, char** key, int32_t* key_len);

uint8_t pj_next_value(json_t* json, json_t* value); // for both objects AND arrays

json_id_t pj_str(char** str, int32_t* len, const json_t* json);

json_id_t pj_int(int* dest, const json_t* json);

json_id_t pj_float(float* dest, const json_t* json);

#endif /* End PERFECT_JSON_H */
