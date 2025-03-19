#include "gdscript_parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "file_sys.h"
#include "token.h"
#include "arena.h"

#define TOKENIZE_RESIZE_OVERHEAD  2048

#define STRING_ENDERS							\
	STR_X("'''",    0x01, TRIPLE_SINGLE_QOUTE)	\
	STR_X("\"\"\"", 0x02, TRIPLE_DOUBLE_QUOTE)	\
	STR_X("'",      0x04, SINGLE_QUOTE)			\
	STR_X("\"",     0x08, DOUBLE_QUOTE)			\

#define STRING_DELIMITERS								\
	STR_X("r'''",    0x10, RAW_TRIPLE_SINGLE_QUOTE)		\
	STR_X("r\"\"\"", 0x20, RAW_TRIPLE_DOUBLE_QUOTE)     \
	STR_X("r'",      0x40, RAW_SINGLE_QUOTE)		    \
	STR_X("r\"",     0x80, RAW_DOUBLE_QUOTE)		    \
	STR_X("&\"",     0x81, STRING_NAME)		            \
	STR_X("^\"",     0x82, NODE_PATH)				    \
	STRING_ENDERS										\

#define ITERATE_OVER_SOURCE												\
	state = LEADING_WHITESPACE;											\
	str_bitmask = STR_BITMASK_NONE;										\
	line = 1;															\
	column = 0;															\
	for (uint32_t i = 0; i < n; ++i)									\
	{																	\
		const char c = str[i];											\
		switch (state)													\
		{																\
		case LEADING_WHITESPACE:										\
			switch(c)													\
			{															\
			case '#': pos = i; state = COMMENT; break;					\
			case '\n': break;											\
			case ' ': __attribute__((fallthrough));						\
			case '\t':													\
				do { HANDLE_LEADING_WHITESPACE } while(0);				\
				break;													\
			default:													\
				if ((offset = string_starter(str + i, n - i, &str_bitmask)) > 0) \
				{														\
					pos = i;											\
					coords.line = line;									\
					coords.col = column;								\
					i += offset - 1;									\
					state = STRING;										\
				}														\
				else if ((offset = delimiting_token(str + i, n - i)) > 0) \
				{														\
					do { HANDLE_DELIMITING_TOKEN }while(0);				\
					i += offset - 1;									\
					state = WAITING_CHAR;								\
				}														\
				else													\
				{														\
					pos = i;											\
					coords.line = line;									\
					coords.col = column;								\
					state = WSB_TOKEN;									\
				}														\
			}															\
			break;														\
		case WSB_TOKEN:													\
			if ((offset = delimiting_token(str + i, n - i)) > 0)		\
			{															\
				do { HANDLE_TOKEN } while(0);							\
				do { HANDLE_DELIMITING_TOKEN } while(0);				\
				state = c == '\n' ? LEADING_WHITESPACE : WAITING_CHAR;	\
				i += offset - 1;										\
			}															\
			else if (isspace(c))										\
			{															\
				do { HANDLE_TOKEN } while(0);							\
				state = WAITING_CHAR;									\
			}															\
			break;														\
		case WAITING_CHAR:												\
			if ((offset = string_starter(str + i, n - i, &str_bitmask)) > 0) \
			{															\
				pos = i;												\
				coords.line = line;										\
				coords.col = column;									\
				i += offset - 1;										\
				state = STRING;											\
			}															\
			else if ((offset = delimiting_token(str + i, n - i)) > 0)	\
			{															\
				do { HANDLE_DELIMITING_TOKEN } while(0);				\
				i += offset - 1;										\
				switch (c)												\
				{														\
				case '\n':												\
					state = LEADING_WHITESPACE;							\
					break;												\
				case '#':												\
					pos = i;											\
					coords.line = line;									\
					coords.col = column;								\
					state = COMMENT;									\
					break;												\
				default:												\
					state = WAITING_CHAR;								\
				}														\
			}															\
			else if (!isspace(c))										\
			{															\
				pos = i;												\
				coords.line = line;										\
				coords.col = column;									\
				state = WSB_TOKEN;										\
			}															\
			break;														\
		case COMMENT:													\
			switch (c)													\
			{															\
			case '\n':													\
				do { HANDLE_COMMENT } while(0);							\
				state = LEADING_WHITESPACE;								\
				break;													\
			}															\
			break;														\
		case STRING:													\
			if (str[i - 1] != '\\' && (offset = string_ender(str + i, n - i, str_bitmask)) > 0) \
			{															\
				do { HANDLE_STRING } while(0);							\
				i += offset - 1;										\
				str_bitmask = STR_BITMASK_NONE;							\
				state = WAITING_CHAR;									\
			}															\
			break;														\
		}																\
		if (c == '\n')													\
		{																\
			line++;														\
			column = 0;													\
		}																\
		else															\
		{																\
			column++;													\
		}																\
	}																	\
	if (state & (WSB_TOKEN | STRING | COMMENT))							\
	{																	\
		do { HANDLE_TRAILING } while(0);								\
	}

static uint32_t delimiting_token(const char* str, const unsigned int n);
static uint32_t string_starter(const char* str, const unsigned int n, uint8_t* str_bitmask);
static uint32_t string_ender(const char* str, const unsigned int n, const uint8_t str_bitmask);
static gdscript_id_t id_token(const char* str);
static void new_token(arena_t* token_arena, arena_t* char_arena, const char* str, const int n, const uint32_t line, const uint32_t col);

static uint32_t delimiting_token(const char* str, const unsigned int n)
{
	char* temp;
	unsigned int len;
	#define GDSCRIPT_X(STR, ID)					\
	temp = STR;									\
	len = strlen(temp);							\
	for (uint32_t i = 0; i < len && i < n; ++i) \
	{											\
		if (str[i] != temp[i])					\
		{										\
			goto stop_##ID;						\
		}										\
	}											\
	return len;									\
	stop_##ID:

	// GDSCRIPT_DELIMITING_TOKENS

	GDSCRIPT_DELIMITING_TOKENS


		return 0;
	#undef GDSCRIPT_X
}

static uint32_t string_starter(const char* str, const unsigned int n, uint8_t* str_bitmask)
{
	char* temp;
	unsigned int len;
	#define STR_X(STR, BITFLAG, ID)								\
	temp = STR;													\
	len = strlen(temp);											\
	for (unsigned int i = 0; i < strlen(temp) && i < n; ++i)	\
	{															\
		if (str[i] != temp[i])									\
		{														\
			goto stop_##ID;								        \
		}														\
	}															\
	*str_bitmask = BITFLAG > 0x08 ? BITFLAG >> 4 : BITFLAG;		\
	return len;													\
	stop_##ID:
	STRING_DELIMITERS
	return 0;
	#undef STR_X
}

static uint32_t string_ender(const char* str, const unsigned int n, const uint8_t str_bitmask)
{
	char* temp;
	unsigned int len;
	#define STR_X(STR, BITFLAG, _)									\
	case BITFLAG:													\
		temp = STR;													\
		len = strlen(temp);											\
		for (unsigned int i = 0; i < strlen(temp) && i < n; ++i)	\
		{															\
			if (str[i] != temp[i])									\
			{														\
				goto stop_##BITFLAG;								\
			}														\
		}															\
		return len;													\
	stop_##BITFLAG:													\
		break;
	switch (str_bitmask)
	{
		STRING_ENDERS
	}
	return 0;
	#undef STR_X
}

static gdscript_id_t id_token(const char* str)
{
	char* temp;
	unsigned int len;
	#define GDSCRIPT_X(STR, ID)					\
	temp = STR;									\
	len = strlen(temp);							\
	for (unsigned int i = 0; i < len; ++i)		\
	{											\
		if (str[i] != temp[i])					\
		{										\
			goto stop_##ID;						\
		}										\
	}											\
	return ID;									\
	stop_##ID:
	GDSCRIPT_BUILTIN_TOKENS
	return GDSCRIPT_SYMBOL;
	#undef GDSCRIPT_X
}


static void new_token(arena_t* token_arena, arena_t* char_arena, const char* str, const int n, const uint32_t line, const uint32_t col)
{
	token_t* t = arena_alloc(token_arena, sizeof *t);
	t->line = line;
	/* t->col = col - n; */
	t->col = col; // notices underflow OwO whats this
	char* token_str = arena_alloc(char_arena, n + 1);
	memcpy(token_str, str, n);
	token_str[n] = '\0';
	t->string = token_str;
}


uint32_t gdscript_tokenize(arena_t* arena, token_t** tokens, gdscript_id_t** ids, const char* str, const uint32_t n)
{
	uint32_t num_chars = 0;
	uint32_t num_tokens = 0;
	struct
	{
		uint32_t line;
		uint32_t col;
	} coords = {0};
	enum __attribute__((packed))
	{
		LEADING_WHITESPACE = 0x00,
		WSB_TOKEN,
		WAITING_CHAR,
		STRING,
		COMMENT,
	} state = LEADING_WHITESPACE;
	enum __attribute__((packed))
	{
		STR_BITMASK_NONE = 0x00,
		#define STR_X(STR, BITS, NAME) NAME = BITS,
		STRING_DELIMITERS
		#undef STR_X
	} str_bitmask = STR_BITMASK_NONE;
	#define HANDLE_LEADING_WHITESPACE \
		num_tokens++;\
		num_chars += 2;
	#define HANDLE_DELIMITING_TOKEN \
		num_tokens++;\
		num_chars += offset + 1;
	#define HANDLE_TOKEN \
		num_tokens++; \
		num_chars += i - pos + 1;
	#define HANDLE_COMMENT \
		num_tokens += 2; \
		num_chars += i - pos + 3;
	#define HANDLE_STRING \
		num_tokens++; \
		num_chars += i - pos + offset + 2;
	#define HANDLE_TRAILING \
		num_tokens++; \
		num_chars += n - pos + 1;
	uint32_t offset;
	uint32_t pos;
	uint32_t line;
	uint32_t column;

	ITERATE_OVER_SOURCE

	#undef HANDLE_LEADING_WHITESPACE
	#undef HANDLE_DELIMITING_TOKEN
	#undef HANDLE_TOKEN
	#undef HANDLE_COMMENT
	#undef HANDLE_STRING
	#undef HANDLE_TRAILING
	if (num_tokens == 0)
	{
		*tokens = NULL;
		*ids = NULL;
		return 0;
	}
	// memory memery
	size_t req_size = num_tokens * (sizeof **tokens + sizeof **ids) + num_chars;
	if (arena->cap < req_size)
	{
		if (arena->memory != NULL)
		{
			free(arena->memory);
		}
		size_t arena_size = req_size + TOKENIZE_RESIZE_OVERHEAD;
		arena->memory = malloc(arena_size);
		arena->cap = arena_size;
	}
	arena->size = 0;
	arena_t token_arena =
	{
		.memory = arena_alloc(arena, num_tokens * sizeof **tokens),
		.cap = num_tokens * sizeof **tokens
	};
	arena_t id_arena =
	{
		.memory = arena_alloc(arena, num_tokens * sizeof **ids),
		.cap = num_tokens * sizeof **ids
	};
	arena_t char_arena =
	{
		.memory = arena_alloc(arena, num_chars),
		.cap = num_chars,
	};

	#define HANDLE_LEADING_WHITESPACE \
		gdscript_id_t* id = arena_alloc(&id_arena, sizeof *id); \
		*id = GDSCRIPT_WHITESPACE; \
		new_token(&token_arena, &char_arena, str + i, 1, line, column);
	#define HANDLE_DELIMITING_TOKEN \
		gdscript_id_t* id = arena_alloc(&id_arena, sizeof *id); \
		*id = GDSCRIPT_NONE; \
		new_token(&token_arena, &char_arena, str + i, offset, line, column);
	#define HANDLE_COMMENT \
		gdscript_id_t* id = arena_alloc(&id_arena, sizeof *id); \
		*id = GDSCRIPT_COMMENT; \
		new_token(&token_arena, &char_arena, str + pos, i - pos, coords.line, coords.col); \
		id = arena_alloc(&id_arena, sizeof *id); \
		*id = GDSCRIPT_LINE_END; \
		new_token(&token_arena, &char_arena, str + i, 1, line, column);
	#define HANDLE_TOKEN \
		gdscript_id_t* id = arena_alloc(&id_arena, sizeof *id); \
		*id = GDSCRIPT_NONE; \
		new_token(&token_arena, &char_arena, str + pos, i - pos, coords.line, coords.col);
	#define HANDLE_STRING \
		gdscript_id_t* id = arena_alloc(&id_arena, sizeof *id); \
		*id = GDSCRIPT_STRING; \
		new_token(&token_arena, &char_arena, str + pos, i - pos + offset + 1, coords.line, coords.col);
	#define HANDLE_TRAILING \
		gdscript_id_t* id = arena_alloc(&id_arena, sizeof *id); \
		*id = GDSCRIPT_STRING; \
		new_token(&token_arena, &char_arena, str + pos, n - pos, coords.line, coords.col);

	ITERATE_OVER_SOURCE

	#undef HANDLE_LEADING_WHITESPACE
	#undef HANDLE_DELIMITING_TOKEN
	#undef HANDLE_TOKEN
	#undef HANDLE_COMMENT
	#undef HANDLE_STRING
	#undef HANDLE_TRAILING
	*tokens = token_arena.memory;
	*ids = id_arena.memory;
	return num_tokens;
}


uint32_t gdscript_classify(token_t* tokens, gdscript_id_t* ids, const uint32_t n)
{
	for (int i = 0; i < n; ++i)
	{
		token_t token = tokens[i];
		if (ids[i] == GDSCRIPT_NONE)
		{
			ids[i] = id_token(token.string);
		}
	}
}

static void pv(const char* str, const int n)
{
	struct
	{
		uint32_t line;
		uint32_t col;
	} coords = {0};
	enum __attribute__((packed))
	{
		LEADING_WHITESPACE = 0x00,
		WSB_TOKEN,
		WAITING_CHAR,
		STRING,
		COMMENT,
	} state = LEADING_WHITESPACE;
	enum __attribute__((packed))
	{
		STR_BITMASK_NONE = 0x00,
		#define STR_X(STR, BITS, NAME) NAME = BITS,
		STRING_DELIMITERS
		#undef STR_X
	} str_bitmask = STR_BITMASK_NONE;
	#define HANDLE_LEADING_WHITESPACE \
		printf("%d:%d: leading ws\n", line, column);
	#define HANDLE_DELIMITING_TOKEN \
		char temp[offset + 1]; \
		memcpy(temp, str + i, offset); \
		temp[offset] = '\0'; \
		printf("%d:%d: delimiter: %s\n", line, column, temp);
	#define HANDLE_TOKEN \
		char temp[i - pos + 1]; \
		memcpy(temp, str + pos, i - pos); \
		temp[i - pos] = '\0'; \
		printf("%d:%d: token: %s\n", coords.line, coords.col, temp);
	#define HANDLE_COMMENT \
		char temp[i - pos + 1]; \
		memcpy(temp, str + pos, i - pos); \
		temp[i - pos] = '\0'; \
		printf("%d:%d: %s\n\n", line, column, temp);
	#define HANDLE_STRING \
		char temp[i - pos + offset + 2]; \
		memcpy(temp, str + pos, i - pos + offset + 1); \
		temp[i - pos + offset + 1] = '\0'; \
		printf("%d:%d: string: %s\n", coords.line, coords.col, temp);
	#define HANDLE_TRAILING \
		char temp[n - pos + 1]; \
		for (int i = pos; i < n; ++i) \
		{ \
			printf("%c", str[i]); \
		} \
		memcpy(temp, str + pos, n - pos); \
		temp[n - pos] = '\0'; \
		printf("%d:%d: trailing: %s\n", coords.line, coords.col, temp);
	uint32_t offset;
	uint32_t pos;
	uint32_t line;
	uint32_t column;

	ITERATE_OVER_SOURCE

	#undef HANDLE_LEADING_WHITESPACE
	#undef HANDLE_DELIMITING_TOKEN
	#undef HANDLE_TOKEN
	#undef HANDLE_COMMENT
	#undef HANDLE_STRING
	#undef HANDLE_TRAILING
}

int main(int argc, char** argv)
{
	arena_t arena = {0};
	token_t* tokens;
	gdscript_id_t* ids;
	int src_len = 0;
	char* src = fs_read("RewindAbility.gd", &src_len);
	uint32_t num_tokens = gdscript_tokenize(&arena, &tokens, &ids, src, src_len);
	gdscript_classify(tokens, ids, num_tokens);
	for (uint32_t i = 0; i < num_tokens; ++i)
	{
		token_t t = tokens[i];
		gdscript_id_t id = ids[i];
		printf("%d:%d:(%#06x) %s\n", t.line, t.col, id, t.string);
	}
	return 0;
}
