#include "token_ids.h"
#include <stdlib.h>
#include <stdio.h>
#include "../allocators/arena.h"
#include "token_id_map.h"

#define BUILTIN_MAPPED_OPERATORS \
	X(OP_IS,		  "is",				2) \
	X(OP_IN,		  "in",				2) \
	X(OP_AS,		  "as",				2) \
	X(OP_NOT,		  "not",				3) \
	X(OP_OR,		  "or",				2) \
	X(OP_AND,		  "and",				3) \
	X(OP_XOR,		  "xor",				3) \
	X(OP_POWER,					    "**",			2) \
	X(OP_PRODUCT,				    "*",			1) \
	X(OP_SUM,					    "+",			1) \
	X(OP_DIFFERENCE,			    "-",			1) \
	X(OP_DIVISION,				    "/",			1) \
	X(OP_MODULO,				    "%",			1) \
	X(OP_BOOL_AND,				    "&&",			2) \
	X(OP_BITWISE_AND,			    "&",			1) \
	X(OP_BOOL_OR,				    "||",			2) \
	X(OP_BITWISE_OR,				"|",			1) \
	X(OP_BITSHIFT_LEFT,				"<<",			2) \
	X(OP_BITSHIFT_RIGHT,			">>",			2) \
	X(OP_BITWISE_XOR,				"^",			1) \
	X(OP_EQUALS,					"==",			2) \
	X(OP_INEQUALS,					"!=",			2) \
	X(OP_LESS_THAN_EQUALS,			"<=",			2) \
	X(OP_LESS_THAN,					"<",			1) \
	X(OP_GREATER_THAN_EQUALS,		">=",			2) \
	X(OP_GREATER_THAN,				">",			1) \
	X(OP_BITWISE_NOT,	 "~",	1)	\
	X(OP_BOOL_NOT,		 "!",	1)

static token_id_map_t builtin_ops = {0};

/* __attribute__((constructor)) */
void token_ids_init(void)
{
		int32_t str_size = 0;
		#define X(ID, STR, LEN) str_size += LEN;
		BUILTIN_MAPPED_OPERATORS
		#undef X
		uint8_t* buffer = malloc(128 * 10 + str_size);
		token_id_map_init(&builtin_ops, buffer, 128);
		arena_t str_arena = { .allocation = buffer + 128 * 10, .cap = str_size};
		#define X(ID, STR, LEN) token_id_map_cache(&builtin_ops, arena_push(&str_arena, STR, LEN), LEN, GDSCRIPT_##ID);
		BUILTIN_MAPPED_OPERATORS
		#undef X
		/* gid_map_print(&builtin_ops); */
}

token_id_t builtin_operators(const char* key, const int8_t len)
{
	return token_id_map_fetch(&builtin_ops, key, len);
}
