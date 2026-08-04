#ifndef PERFECT_JSON_H
#define PERFECT_JSON_H

#include <stdint.h>

#define JSON_STRUCTURE_LIST	\
	X(COMMA,		',')	\
	X(COLON,		':')	\
	X(OBJECT_START, '{')	\
	X(OBJECT_END,	'}')	\
	X(ARRAY_START,	'[')	\
	X(ARRAY_END,	']')

#define JSON_IDS \
	JSON_STRUCTURE_LIST \
	X(INTEGER = 0x08) \
	X(FLOAT) /* 0x09 */ \
	X(STRING) /* 0x0a */ \
	X(NULL) /* 0x0b */ \
	X(TRUE)  /* 0x0c */ \
	X(FALSE) /* 0x0d */


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

/* typedef struct id_stack id_stack; */

typedef struct json_t
{
	char* src;
	int32_t* offsets;
	uint8_t* bitmasks; int32_t num_tokens;
	int32_t index;
} json_t;

typedef struct json_data_t
{
	union
	{
		struct
		{
			char* str;
			int32_t len;
		};
		json_t* json;
		int8_t boolean;
		int32_t integer;
		float floater;
	};
	const json_id_t id;
} json_data_t;

typedef void (*json_data_func)(const json_data_t* data);
typedef void (*json_func)(json_t* json);


typedef struct arena_t arena_t;
typedef struct string string;


// jfc u idiot child create a hashmap.
// but y tho?

void pj_allman(const json_t* json);

json_id_t pj_parse(json_t* json, arena_t* arena, const char* src, const int32_t len);

uint8_t pj_next_key(string* key, json_t* json);

uint8_t pj_next_value(json_t* value, json_t* json); // for both objects AND arrays

json_id_t pj_str(string* str, const json_t* json);

json_id_t pj_int(int* dest, const json_t* json);

json_id_t pj_float(float* dest, const json_t* json);

// GOLD!
void pj_mapc_array(json_func func, json_t* json);

// SOLID!
int32_t pj_array_size(json_t* json);

json_t pj_find_key(const string* query, json_t* json);

// SILVER!
uint8_t pj_next_pair(string* key, json_t* value, json_t* json);

#define LIT(A) #A

#define PJ_OBJ(...) \
	putc('{', JSON_DEST); __VA_ARGS__; putc('}', JSON_DEST)

#define PJ_MEMBER(KEY, VALUE) \
	fprintf(JSON_DEST, #KEY ": "); VALUE; putc(',', JSON_DEST);

#define PJ_LIT(VALUE) fprintf(JSON_DEST, #VALUE)
#define PJ_CHAR(VALUE) putc(VALUE, JSON_DEST)
#define PJ_STR(VALUE) fprintf(JSON_DEST, VALUE)
#define PJ_INT(VALUE) fprintf(JSON_DEST,"%d", VALUE)
#define PJ_FLOAT(VALUE) fprintf(JSON_DEST,"%f", VALUE)
#define PJ_TRUE fprintf(JSON_DEST, "true")
#define PJ_FALSE fprintf(JSON_DEST, "false")
#define PJ_NULL fprintf(JSON_DEST, "null")

#define PJ_LAST_MEMBER(KEY, VALUE)							 \
	fprintf(JSON_DEST, #KEY ": "); VALUE;

#define PJ_ARRAY(...) \
	putc('[', JSON_DEST); __VA_ARGS__; putc(']', JSON_DEST)

#define PJ_ITEM(A) A; putc(',', JSON_DEST)


#endif /* End PERFECT_JSON_H */
