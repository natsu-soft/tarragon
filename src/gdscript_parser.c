#include "gdscript_parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "benchmark.h"
#include "file_sys.h"
#include "mem_sys.h"
#include "token.h"
#include "arena.h"

#define TOKENIZE_RESIZE_OVERHEAD  4096

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
			else														\
			{															\
				do { HANDLE_WHITESPACE } while(0);						\
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
static void new_token(arena_t* token_arena, arena_t* char_arena, const char* str, const int n, const uint16_t line, const uint32_t col);
static gdscript_id_t verify_number(const char* str, const uint32_t n);

/* static uint32_t move_unclassified_to_front(token_t* tokens, gdscript_id_t* ids, const int n); */
/* static void original_positions(token_t* tokens, gdscript_id_t* ids, const int n); */

static void cull_line_continuations(token_t* tokens, gdscript_id_t* ids, uint32_t* num_tokens);

static void do_nothing_with_token(const token_t* t, const char* message)
{
	return;
}

static void (*note_token_func)(const token_t*, const char* message) = do_nothing_with_token;
static void (*warn_token_func)(const token_t*, const char* message) = do_nothing_with_token;
static void (*error_token_func)(const token_t*, const char* message) = do_nothing_with_token;

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
	if (isdigit(str[0]) || (str[0] == '.' && strlen(str) > 1))
	{
		return GDSCRIPT_NUMBER_CANDIDATE;
	}
	else
	{
		return GDSCRIPT_NONE;
	}
	#undef GDSCRIPT_X
}



static void new_token(arena_t* token_arena, arena_t* char_arena, const char* str, const int n, const uint16_t line, const uint32_t col)
{
	token_t* t = arena_alloc(token_arena, sizeof *t);
	t->line = line;
	/* t->col = col - n; */ // notices underflow--OwO whats this?
	t->col = col;
	char* token_str = arena_alloc(char_arena, n + 1);
	memcpy(token_str, str, n);
	token_str[n] = '\0';
	t->string = token_str;
}

static gdscript_id_t verify_number(const char* str, const uint32_t n)
{
	enum {
		NC_NONE = 0x00,
		NC_HEX_FLAG = 0x01,
		NC_BIN_FLAG = 0x02,
		NC_FLOAT_FLAG = 0x04,
		NC_SCI_FLAG = 0x08,
		NC_NEGATIVE_FLAG = 0x10,
		NC_POSITIVE_FLAG = 0x20
	} flags = NC_NONE;
	for (int i = 0; i < n; ++i)
	{
		char c = str[i];
		/* printf("checking: %c\n", c); */
		switch (c)
		{
		case ' ': return GDSCRIPT_NONE; // it's somewhat common tbqh
		case '_':
			switch(i)
			{
			case 0: return GDSCRIPT_NONE;
			default: continue;
			}
			break;
		case '-':
			if (flags & (NC_POSITIVE_FLAG | NC_NEGATIVE_FLAG))
			{
				return GDSCRIPT_NONE;
			}
			else
			{
				switch (i)
				{
				case 0:
					flags |= NC_NEGATIVE_FLAG;
					continue;
				default:
					if (str[i - 1] == 'e' || str[i  - 1] == 'E')
					{
						flags |= NC_NEGATIVE_FLAG;
						continue;
					}
					return GDSCRIPT_NONE;
				}
			}
			break;
		case '+':
			if (flags & (NC_POSITIVE_FLAG | NC_NEGATIVE_FLAG))
			{
				return GDSCRIPT_NONE;
			}
			else
			{
				switch (i)
				{
				case 0:
					flags |= NC_POSITIVE_FLAG;
					continue;
				default:
					if (str[i - 1] == 'e' || str[i  - 1] == 'E')
					{
						flags |= NC_POSITIVE_FLAG;
						continue;
					}
					return GDSCRIPT_NONE;
				}
			}
			break;
		case 'x':
			if (flags ^ NC_HEX_FLAG && i == 1 && str[0] == '0')
			{
				flags |= NC_HEX_FLAG;
				continue;
			}
			else
			{
				return GDSCRIPT_NONE;
			}
			break;
		case 'b':
			if (flags ^ NC_BIN_FLAG && i == 1 && str[0] == '0')
			{
				flags |= NC_BIN_FLAG;
				continue;
			}
			else
			{
				return GDSCRIPT_NONE;
			}
			break;
		case 'e': __attribute__((fallthrough));
		case 'E':
			if (flags & (NC_HEX_FLAG | NC_BIN_FLAG | NC_SCI_FLAG))
			{
				return GDSCRIPT_NONE;
			}
			else if (i == 0)
			{
				return GDSCRIPT_NONE;
			}
			else
			{
				flags |= NC_SCI_FLAG;
				flags &= ~(NC_NEGATIVE_FLAG | NC_POSITIVE_FLAG);
				continue;
			}
			break;
		case '.':
			if (flags & (NC_FLOAT_FLAG | NC_HEX_FLAG | NC_BIN_FLAG | NC_SCI_FLAG))
			{
				return GDSCRIPT_NONE;
			}
			else
			{
				flags |= NC_FLOAT_FLAG;
				continue;
			}
			break;
		default:
			if (flags | NC_HEX_FLAG)
			{
				uint8_t valid = '0' <= c && c <= '9';
				valid |= 'a' <= c && c <= 'f';
				valid |= 'A' <= c && c <= 'F';
				if (valid)
				{
					continue;
				}
				else
				{
					return GDSCRIPT_NONE;
				}
			}
			else if (flags | NC_BIN_FLAG)
			{
				uint8_t valid = c == '0' || c == '1';
				if (valid)
				{
					continue;
				}
				else
				{
					return GDSCRIPT_NONE;
				}
			}
			else
			{
				uint8_t valid = '0' <= c && c <= '9';
				if (valid)
				{
					continue;
				}
				else
				{
					return GDSCRIPT_NONE;
				}
			}
		}
	}
	return GDSCRIPT_NUMBER;
}

static void cull_line_continuations(token_t* tokens, gdscript_id_t* ids, uint32_t* num_tokens)
{
	/* error_token_func(tokens + i); */
	uint32_t n = *num_tokens;
	for (int i = 0; i < n; ++i) {
		if (ids[i] == GDSCRIPT_LINE_CONTINUE && ids[i + 1] == GDSCRIPT_LINE_END)
		{
			size_t k = n - (i + 2);
			memmove(tokens + i, tokens + 2, k * sizeof *tokens);
			memmove(ids + i, ids + 2, k * sizeof *ids);
			n -= 2;
		}
	}
	*num_tokens = n;
}

void cull_redundant_line_ends(token_t* tokens, gdscript_id_t* ids, uint32_t* num_tokens)
{
	uint32_t n = *num_tokens;
	for (int i = 0; i < n - 1; ++i)
	{
		if (ids[i] == GDSCRIPT_LINE_END && ids[i + 1] == GDSCRIPT_LINE_END)
		{
			for (int j = i + 2; j < n; ++j)
			{
				if (ids[j] != GDSCRIPT_LINE_END)
				{
					int k = n - j;
					memmove(tokens + i + 1, tokens + j, k * sizeof *tokens);
					memmove(ids + i + 1, ids + j, k * sizeof *ids);
					n -= j - i;
					break;
				}
			}
		}
	}
	*num_tokens = n;
}

uint32_t gdscript_tokenize(arena_t* arena, token_t** tokens, gdscript_id_t** ids, const char* str, const uint32_t n)
{
	uint32_t num_chars = 0;
	uint32_t num_tokens = 0;
	struct
	{
		uint16_t line;
		uint16_t col;
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
	#define HANDLE_WHITESPACE \
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
	uint16_t line;
	uint16_t column;


	ITERATE_OVER_SOURCE

	#undef HANDLE_LEADING_WHITESPACE
	#undef HANDLE_WHITESPACE
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
	/* num_tokens += 12; */
	/* num_chars += 24; */
	num_tokens += 1;
	num_chars += 2;
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
		*id = GDSCRIPT_LEADING_WHITESPACE; \
		new_token(&token_arena, &char_arena, str + i, 1, line, column);
	#define HANDLE_WHITESPACE \
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
	#undef HANDLE_WHITESPACE
	#undef HANDLE_DELIMITING_TOKEN
	#undef HANDLE_TOKEN
	#undef HANDLE_COMMENT
	#undef HANDLE_STRING
	#undef HANDLE_TRAILING
	token_t* t = arena_alloc(&token_arena, sizeof *t);
	t->line = line;
	t->col = column;
	char* string = arena_alloc(&char_arena, 2);
	memcpy(string, "\n", 2);
	t->string = string;
	/* token_t* t = arena_alloc(&token_arena, sizeof *t); */
	*tokens = token_arena.memory;
	*ids = id_arena.memory;
	return num_tokens;
}


static void classify_tokens(token_t* tokens, gdscript_id_t* ids, const uint32_t n)
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

static void coalesce_numbers(token_t* tokens, gdscript_id_t* ids, uint32_t* num_tokens)
{
	uint32_t n = *num_tokens;
	for (int i = 0; i < n; ++i)
	{
		gdscript_id_t id = ids[i];
		/* uint8_t attempt_flag = id == GDSCRIPT_NUMBER_CANDIDATE; */
		/* attempt_flag |= id == GDSCRIPT_OP_SUM; */
		/* attempt_flag |= id == GDSCRIPT_OP_DIFFERENCE; */
		/* if (attempt_flag) */
		if (id == GDSCRIPT_NUMBER_CANDIDATE)
		{
			/* continue; */
			int index = -1;
			int len = 0;
			for (int j = i; j < n; ++j)
			{
				len += strlen(tokens[j].string);
				char temp[len];
				int cum  = 0;
				for (int k = i; k <= j; ++k)
				{
					char* str = tokens[k].string;
					int str_len = strlen(str);
					memmove(temp + cum, str, str_len);
					cum += str_len;
				}
				if (verify_number(temp, len))
				{
					index = j;
				}
				else
				{
					break;
				}
			}
			if (index >= i)
			{
				ids[i] = GDSCRIPT_NUMBER;
				for (int j = i + 1; j <= index; ++j)
				{
					int token_len = strlen(tokens[i].string);
					memmove(tokens[i].string + token_len, tokens[j].string, strlen(tokens[j].string) + 1);
				}
				if (i < index && index < n - 1)
				{
					size_t k = n - index;
					memmove(tokens + i + 1, tokens + index + 1, k * sizeof *tokens);
					memmove(ids + i + 1, ids + index + 1, k * sizeof *ids);
				}
				n -= index - i;
			}
		}
	}
	*num_tokens = n;
}

static void leading_whitespace_to_scope(token_t* tokens, gdscript_id_t* ids, uint32_t* num_tokens)
{
	enum {
		LWS_ACTIVE = 0x01,
		LWS_WAITING = 0x02,
	} state = LWS_ACTIVE;
	uint32_t n = *num_tokens;
	// if someone overflows this they desserve the parser to break
	uint8_t depth = 0;
	uint8_t code_depth = 0;
	for (uint32_t i = 0; i < n; ++i)
	{
		switch (ids[i])
		{
		case GDSCRIPT_COMMENT: continue;
		case GDSCRIPT_LEADING_WHITESPACE:
			if (state & LWS_ACTIVE)
			{
				depth++;
			}
			break;
		case GDSCRIPT_LINE_END:
			if(state & LWS_WAITING)
			{
				depth = 0;
				state = LWS_ACTIVE;
			}
			break;
		default:
			if (code_depth != depth)
			{
				int tail = n - i;
				memmove(tokens + i + 1, tokens + i, tail * sizeof *tokens);
				memmove(ids + i + 1, ids + i, tail * sizeof *ids);
				ids[i] = depth > code_depth ? GDSCRIPT_SCOPE_START : GDSCRIPT_SCOPE_END;
				n += 1;
			}
			code_depth = depth;
			state = LWS_WAITING;
		}
	}
	// NOTE: this CAN fuck you up
	for (uint8_t i = 0; i < code_depth; ++i)
	{
		tokens[n + i] = tokens[n];
		ids[n + i] = GDSCRIPT_SCOPE_END;
	}
	*num_tokens = n + code_depth;
}

// TODO: backwards iterate
static void trim_token (token_t* tokens, gdscript_id_t* ids, uint32_t* num_tokens, const gdscript_id_t trim_token)
{
	uint32_t swap = 0;
	const uint32_t n = *num_tokens;
	gdscript_id_t swap_id;
	token_t swap_token = {0};
	for (int i = swap; i < n; ++i)
	{
		gdscript_id_t id = ids[i];
		if (id != trim_token)
		{
			ms_buffer_swap(ids + i, ids + swap, &swap_id, sizeof swap_id);
			ms_buffer_swap(tokens + i, tokens + swap, &swap_token, sizeof swap_token);
			swap++;
		}
	}
	*num_tokens = swap;
}

void gdscript_preprocess(token_t* tokens, gdscript_id_t* ids, uint32_t* num_tokens)
{
	classify_tokens(tokens, ids, *num_tokens);
	coalesce_numbers(tokens, ids, num_tokens);
	printf("coalesce_numbers: %d\n", *num_tokens);
	trim_token(tokens, ids, num_tokens, GDSCRIPT_WHITESPACE);
	printf("trim_whitespace: %d\n", *num_tokens);
	cull_line_continuations(tokens, ids, num_tokens);
	printf("cull_line_continuations: %d\n", *num_tokens);
	leading_whitespace_to_scope(tokens, ids, num_tokens);
	printf("leading_whitespace_to_scope: %d\n", *num_tokens);
	trim_token(tokens, ids, num_tokens, GDSCRIPT_LEADING_WHITESPACE);
	printf("trim_leading_whitespace: %d\n", *num_tokens);
	trim_token(tokens, ids, num_tokens, GDSCRIPT_COMMENT);
	printf("trim_comments: %d\n", *num_tokens);
	cull_redundant_line_ends(tokens, ids, num_tokens);
	printf("cull_redundant_line_ends: %d\n", *num_tokens);
	/* NANOTIME(classify_tokens(tokens, ids, *num_tokens);) */
	/* NANOTIME(coalesce_numbers(tokens, ids, num_tokens);) */
	/* NANOTIME(trim_token(tokens, ids, num_tokens, GDSCRIPT_WHITESPACE);) */
	/* NANOTIME(cull_line_continuations(tokens, ids, num_tokens);) */
	/* NANOTIME(leading_whitespace_to_scope(tokens, ids, num_tokens);) */
	/* NANOTIME(trim_token(tokens, ids, num_tokens, GDSCRIPT_LEADING_WHITESPACE);) */
	/* NANOTIME(trim_token(tokens, ids, num_tokens, GDSCRIPT_COMMENT);) */
	/* NANOTIME(cull_redundant_line_ends(tokens, ids, num_tokens);) */
}

static void pv(const char* str, const int n)
{
	struct
	{
		uint16_t line;
		uint16_t col;
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
	#define HANDLE_WHITESPACE \
		printf("%d:%d: ws\n", line, column);
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
	uint16_t line;
	uint16_t column;

	ITERATE_OVER_SOURCE

	#undef HANDLE_LEADING_WHITESPACE
	#undef HANDLE_DELIMITING_TOKEN
	#undef HANDLE_TOKEN
	#undef HANDLE_COMMENT
	#undef HANDLE_STRING
	#undef HANDLE_TRAILING
}

void gdscript_note_token_func(void (*func_ptr)(const token_t*, const char* message))
{
	note_token_func = func_ptr;
}

void gdscript_warn_token_func(void (*func_ptr)(const token_t*, const char* message))
{
	warn_token_func = func_ptr;
}

void gdscript_error_token_func(void (*func_ptr)(const token_t*, const char* message))
{
	error_token_func = func_ptr;
}


int main(int argc, char** argv)
{
	arena_t arena = {0};
	token_t* tokens;
	gdscript_id_t* ids;
	int src_len = 0;
	char* src = fs_read("RewindAbility.gd", &src_len);
	uint32_t num_tokens = gdscript_tokenize(&arena, &tokens, &ids, src, src_len);
	const uint32_t n0 = num_tokens;
	printf("num_tokens0: %d\n", num_tokens);
	gdscript_preprocess(tokens, ids, &num_tokens);
	printf("num_tokens1: %d\n", num_tokens);
	for (uint32_t i = 0; i < num_tokens; ++i)
	{
		token_t t = tokens[i];
		gdscript_id_t id = ids[i];
		// printf("%d:%d:(%#06x) %s\n", t.line, t.col, id, t.string);
	}
	printf("SIZE: %zu\n", arena.cap);
	printf("GDSCRIPT_SCOPE_START: %#06x\n", GDSCRIPT_SCOPE_START);
	printf("GDSCRIPT_SCOPE_END: %#06x\n", GDSCRIPT_SCOPE_END);
	printf("unused bytes between tokens & ids: %zu\n", (n0 - num_tokens) * sizeof *tokens);
	return 0;
}
