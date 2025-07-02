#include "gdscript_lexer.h"
#include "gdscript_parser.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>
#include "arena.h"
#include "scratch.h"

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


// NOTE: THE FUNNY METHOD IS FASTER!
/* static struct */
/* { */
/* 	char** tokens; */
/* 	uint8_t* lens; */
/* 	uint8_t size; */
/* } delimiters = {0}; */

static struct {
	char** tokens;
	gdscript_id_t* ids;
	uint8_t* lens;
} builtin = {0};

 // NOTE: THIS IS BAD STOP USING IT
#define NUM_UNARY_PRECEDERS 47
static gdscript_id_t* unary_preceders = NULL;

__attribute__((constructor)) static void init_lexer(void)
{
	uint32_t l = 0;
#define X(STR, _) l += strlen(STR);
	GDSCRIPT_BUILTIN_TOKENS
#undef X
	const size_t size0 = sizeof *builtin.tokens + sizeof *builtin.lens + sizeof *builtin.ids;
	uint8_t* allocation = malloc(GDSCRIPT_NUM_BUILTIN * size0 + l);
	builtin.tokens = allocation;
	builtin.lens = builtin.tokens + GDSCRIPT_NUM_BUILTIN;
	builtin.ids = builtin.lens + GDSCRIPT_NUM_BUILTIN;
	arena_t char_arena =
	{
		.allocation = builtin.ids + GDSCRIPT_NUM_BUILTIN,
		.cap = l
	};
	uint32_t i = 0;
	char* str;
#define X(STR, ENUM)												\
	builtin.tokens[i] = char_arena.allocation + char_arena.size;	\
	str = STR; \
	l = strlen(str);												\
	arena_push(&char_arena, str, l);								\
	builtin.lens[i] = l;											\
	builtin.ids[i] = GDSCRIPT_##ENUM;								\
	++i;
	GDSCRIPT_BUILTIN_TOKENS
#undef X
	i = 0;
	unary_preceders = malloc(NUM_UNARY_PRECEDERS * sizeof *unary_preceders);
#define X(_, ENUM) unary_preceders[i++] = GDSCRIPT_##ENUM;
	GDSCRIPT_OPERATORS
#undef X
#define UNARY_STRUCT						\
		X(NONE)								\
		X(SEMICOLON)						\
		X(LINE_END)							\
		X(LEADING_WHITESPACE)				\
		X(COMMA)							\
		X(PAREN_OPEN)						\
		X(BRACKET_OPEN)						\
		X(SCOPE_START)						\
		X(SCOPE_END)
#define X(ENUM) unary_preceders[i++] = GDSCRIPT_##ENUM;
		UNARY_STRUCT
#undef X
#undef UNARY_STRUCT
}


static void preprocess_tokens(char** tokens, uint16_t* lens, gdscript_id_t* ids, uint32_t* num_tokens);
static uint32_t delimiting_token(const char* str, const unsigned int n);
static uint32_t string_starter(const char* str, const unsigned int n, uint8_t* str_bitmask);
static uint32_t string_ender(const char* str, const unsigned int n, const uint8_t str_bitmask);
static const gdscript_id_t id_token(const char* token, const uint16_t len);
static void new_token(arena_t* token_arena, arena_t* len_arena, const char* src, const uint16_t pos, const uint16_t len);
static const gdscript_id_t verify_number(const char* src, const uint32_t n);
static void cull_line_continuations(char** tokens, uint16_t* lens, gdscript_id_t* ids, uint32_t* num_tokens);
static void lws_to_scopes(arena_t* token_arena, arena_t* len_arena, arena_t* id_arena, uint32_t* num_tokens);
static void trim_token (char** tokens, uint16_t* lens, gdscript_id_t* ids, uint32_t* num_tokens, const gdscript_id_t trim_token);

/* static void do_nothing_with_token(const char** t, const char* message) */
/* { */
/* 	return; */
/* } */

// TODO: early errors/warnings/notes
/* static void (*note_token_func)(const char**, const char* message) = do_nothing_with_token; */
/* static void (*warn_token_func)(const char**, const char* message) = do_nothing_with_token; */
/* static void (*error_token_func)(const char**, const char* message) = do_nothing_with_token; */

// MR. GOD IS ENTERTAINED BY HUMOR
static uint32_t delimiting_token(const char* str, const unsigned int n)
{
	char* temp;
	unsigned int len;
	#define X(STR, ID)					\
	temp = STR;									\
	len = strlen(temp);							\
	for (uint32_t i = 0; i < len; ++i)			\
	{											\
		if (i < n)								\
		{										\
			if (str[i] != temp[i])				\
			{									\
				goto stop_##ID;					\
			}									\
		}										\
		else									\
		{										\
			goto stop_##ID;						\
		}										\
	}											\
	return len;									\
	stop_##ID:
	GDSCRIPT_DELIMITING_TOKENS
	return 0;
	#undef X
}

static uint32_t string_starter(const char* str, const unsigned int n, uint8_t* str_bitmask)
{
	char* temp;
	unsigned int len;
	#define STR_X(STR, BITFLAG, ID) \
	temp = STR; \
	len = strlen(temp); \
	for (unsigned int i = 0; i < strlen(temp); ++i) \
	{ \
		if (i < n) \
		{ \
			if (str[i] != temp[i]) \
			{ \
				goto STOP_##ID; \
			} \
		} \
		else \
		{ \
			goto STOP_##ID; \
		} \
	} \
	*str_bitmask = BITFLAG > 0x08 ? BITFLAG >> 4 : BITFLAG; \
	return len; \
	STOP_##ID:
	STRING_DELIMITERS
	return 0;
	#undef STR_X
}

static uint32_t string_ender(const char* str, const unsigned int n, const uint8_t str_bitmask)
{
	char* temp;
	unsigned int len;
	#define STR_X(STR, BITFLAG, _) \
	case BITFLAG: \
		temp = STR; \
		len = strlen(temp); \
		for (unsigned int i = 0; i < strlen(temp); ++i)	\
		{ \
			if (i < n) \
			{ \
				if (str[i] != temp[i]) \
				{ \
					goto STOP_##BITFLAG; \
				} \
			} \
			else \
			{ \
				goto STOP_##BITFLAG; \
			} \
		} \
		return len; \
	STOP_##BITFLAG: \
		break;
	switch (str_bitmask)
	{
		STRING_ENDERS
	}
	return 0;
	#undef STR_X
}

/* static const gdscript_id_t id_token(const char** token) */
/* { */
/* 	const char* str = token->string; */
/* 	char* temp; */
/* 	const uint32_t token_len = token->len; */
/* 	unsigned int len; */
/* #define X(STR, ID)								\ */
/* 	temp = STR;									\ */
/* 	len = strlen(temp);							\ */
/* 	len = len > token_len ? token->len : len;	\ */
/* 	for (unsigned int i = 0; i < len; ++i)		\ */
/* 	{											\ */
/* 		if (str[i] != temp[i])					\ */
/* 		{										\ */
/* 			goto STOP_##ID;						\ */
/* 		}										\ */
/* 	}											\ */
/* 	return GDSCRIPT_##ID;						\ */
/* 	STOP_##ID: */
/* 	GDSCRIPT_BUILTIN_TOKENS */
/* 	if (('0' <= str[0] && str[0] < '9') || (str[0] == '.' && token_len > 1)) */
/* 	{ */
/* 		return GDSCRIPT_NUMBER_CANDIDATE; */
/* 	} */
/* 	else */
/* 	{ */
/* 		return GDSCRIPT_NONE; */
/* 	} */
/* #undef X */
/* } */

static const gdscript_id_t id_token(const char* token, const uint16_t len)
{
	char** tokens = builtin.tokens;
	uint8_t* lens = builtin.lens;
	for (uint8_t i = 0; i < GDSCRIPT_NUM_BUILTIN; ++i)
	{
		if (lens[i] == len && strncmp(tokens[i], token, len) == 0)
		{
			return builtin.ids[i];
		}
	}
	if (('0' <= token[0] && token[0] < '9') || (token[0] == '.' && len > 1))
	{
		return GDSCRIPT_NUMBER_CANDIDATE;
	}
	else
	{
		return GDSCRIPT_NONE;
	}
}

static void new_token(arena_t* token_arena, arena_t* len_arena, const char* src, const uint16_t pos, const uint16_t len)
{
	char** token = arena_dynamic_alloc(token_arena, sizeof *token);
	*token = src + pos;
	arena_dynamic_push(len_arena, &len, sizeof len);
}

static const gdscript_id_t verify_number(const char* src, const uint32_t n)
{
	/* printf("verifying: "); */
	/* for (uint32_t i = 0; i < n; ++i) */
	/* { */
	/* 	printf("%c", src[i]); */
	/* } */
	/* printf("\n"); */
	enum __attribute__((packed)) {
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
		char c = src[i];
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
					if (src[i - 1] == 'e' || src[i  - 1] == 'E')
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
					if (src[i - 1] == 'e' || src[i  - 1] == 'E')
					{
						flags |= NC_POSITIVE_FLAG;
						continue;
					}
					return GDSCRIPT_NONE;
				}
			}
			break;
		case 'x':
			if (flags ^ NC_HEX_FLAG && i == 1 && src[0] == '0')
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
			if (flags ^ NC_BIN_FLAG && i == 1 && src[0] == '0')
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

static void cull_line_continuations(char** tokens, uint16_t* lens, gdscript_id_t* ids, uint32_t* num_tokens)
{
	uint32_t swap = 0;
	const uint32_t n = *num_tokens;
	for (uint32_t i = swap; i < n; ++i)
	{
		const gdscript_id_t id = ids[i];
		if (id != GDSCRIPT_LINE_CONTINUE)
		{
			tokens[swap] = tokens[i];
			lens[swap] = lens[i];
			ids[swap] = id;
			++swap;
		}
		else if (i < n - 1 && ids[i + 1] == GDSCRIPT_LINE_END)
		{
			#define KEEP_TRUCKING \
				X(LEADING_WHITESPACE) ||\
				X(LINE_END) || \
				X(LINE_CONTINUE)
			#define X(A) ids[i] == GDSCRIPT_##A
			for (i += 2; i < n && (KEEP_TRUCKING); ++i);
			#undef X
			#undef KEEP_TRUCKING
			tokens[swap] = tokens[i];
			lens[swap] = lens[i];
			ids[swap] = ids[i];
			++swap;
		}
		else // NOTE: for EOF it marks the 
		{
			for (++i; i < n && ids[i] != GDSCRIPT_LINE_END; ++i)
			{
				tokens[swap] = tokens[i];
				lens[swap] = lens[i];
				ids[swap] = GDSCRIPT_ERROR;
				++swap;
			}
		}
	}
	*num_tokens = swap;
}

static void cull_pointless_lines(char** tokens, uint16_t* lens, gdscript_id_t* ids, uint32_t* num_tokens)
{
	uint32_t n = *num_tokens;
	int line_start = 0;
	uint8_t cull_flag = 1;
	for (int i = 0; i < n - 1; ++i)
	{
		switch(ids[i])
		{
		case GDSCRIPT_LINE_END:
			if (cull_flag && i > line_start)
			{
				/* printf("culling %d\n", tokens[i].line); */
				int k = n - i;
				memmove(tokens + line_start, tokens + i, k * sizeof *tokens);
				memmove(lens + line_start, lens + i, k * sizeof *lens);
				memmove(ids + line_start, ids + i, k * sizeof *ids);
				n -= i - line_start;
			}
			else
			{
				line_start = i + 1;
				cull_flag = 1;
			}
			break;
		case GDSCRIPT_WHITESPACE:
		case GDSCRIPT_LEADING_WHITESPACE:
			break;
		default:
			cull_flag = 0;
		}
	}
	if (n > 0)
	{
		uint32_t swap = 0;
		gdscript_id_t swap_id;
		uint16_t swap_len;
		char* swap_token;
		for (int i = swap + 1; i < n; ++i)
		{
			if (ids[swap] != GDSCRIPT_LINE_END || ids[i] != GDSCRIPT_LINE_END)
			{
				swap++;
				tokens[swap] = tokens[i];
				lens[swap] = lens[i];
				ids[swap] = ids[i];
			}
		}
		*num_tokens = swap + 1;
	}
	else
	{
		*num_tokens = 0;
	}
}

static void cull_pointless_lines2(char** tokens, uint16_t* lens, gdscript_id_t* ids, uint32_t* num_tokens)
{
	uint32_t swap = 0;
	uint32_t endl = 0;
	const uint32_t n = *num_tokens;
	for (uint32_t i = swap; i < n; ++i)
	{
		const gdscript_id_t id = ids[i];
		if (id == GDSCRIPT_LINE_END)
		{
			endl = i;
			for (uint32_t j = i + 1; j < n; ++j)
			{
				if (ids[j] == GDSCRIPT_LINE_END)
				{
					endl = j;
				}
				else if (ids[j] != GDSCRIPT_LEADING_WHITESPACE)
				{
					break;
				}
			}
			i = endl;
		}
		tokens[swap] = tokens[i];
		lens[swap] = lens[i];
		ids[swap] = id;
		++swap;
	}
	*num_tokens = swap;
}

uint32_t gdscript_lex(arena_t* arena, token_table_t* table, const char* src, const uint32_t n)
{
	uint32_t num_tokens = 0;
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
	uint32_t offset = 0;
	uint32_t pos = 0;
	arena->size = 0;
	SCRATCH* id_arena = scratch_checkout_size(n);
	SCRATCH* len_arena = scratch_checkout_size(n);
	for (uint32_t i = 0; i < n; ++i)
	{
		const char c = src[i];
		switch (state)
		{
		case LEADING_WHITESPACE:
			switch (c)
			{
			case '#':
				pos = i;
				state = COMMENT;
				break;
			case ' ':
				__attribute__((fallthrough));
			case '\t':
				{
					gdscript_id_t *id = arena_dynamic_alloc(id_arena, sizeof *id);
					*id = GDSCRIPT_LEADING_WHITESPACE;
					new_token(arena, len_arena, src, i, 1);
					++num_tokens;
				}
				break;
			default:
				if ((offset = string_starter(src + i, n - i, &str_bitmask)) > 0)
				{
					pos = i;
					i += offset - 1;
					state = STRING;
				}
				else if ((offset = delimiting_token(src + i, n - i)) > 0)
				{
					gdscript_id_t *id = arena_dynamic_alloc(id_arena, sizeof *id);
					*id = GDSCRIPT_NONE;
					new_token(arena, len_arena, src, i, offset);
					i += offset - 1;
					state = WAITING_CHAR;
					++num_tokens;
				}
				else
				{
					pos = i;
					state = WSB_TOKEN;
				}
			}
			break;
		case WSB_TOKEN:
			if ((offset = delimiting_token(src + i, n - i)) > 0)
			{
				gdscript_id_t* id = arena_dynamic_alloc(id_arena, 2 * sizeof *id);
				id[0] = GDSCRIPT_NONE;
				id[1] = GDSCRIPT_NONE;
				new_token(arena, len_arena, src, pos, i - pos);
				new_token(arena, len_arena, src, i, offset);
				state = c == '\n' ? LEADING_WHITESPACE : WAITING_CHAR;
				i += offset - 1;
				num_tokens += 2;
			} else if (isspace(c)) {
				gdscript_id_t *id = arena_dynamic_alloc(id_arena, sizeof *id);
				*id = GDSCRIPT_NONE;
				new_token(arena, len_arena, src, pos, i - pos);
				state = WAITING_CHAR;
				++num_tokens;
			}
			break;
		case WAITING_CHAR:
			if ((offset = string_starter(src + i, n - i, &str_bitmask)) > 0)
			{
				pos = i;
				i += offset - 1;
				state = STRING;
			}
			else if ((offset = delimiting_token(src + i, n - i)) > 0)
			{
				gdscript_id_t *id = arena_dynamic_alloc(id_arena, sizeof *id);
				*id = GDSCRIPT_NONE;
				new_token(arena, len_arena, src, i, offset);
				num_tokens++;
				i += offset - 1;
				switch (c)
				{
				case '\n':
					state = LEADING_WHITESPACE;
					break;
				case '#':
					pos = i;
					state = COMMENT;
					break;
				default:
					state = WAITING_CHAR;
				}
			}
			else if (!isspace(c))
			{
				pos = i;
				state = WSB_TOKEN;
			}
			else
			{
				gdscript_id_t *id = arena_dynamic_alloc(id_arena, sizeof *id);
				*id = GDSCRIPT_WHITESPACE;
				new_token(arena, len_arena, src, i, 1);
				num_tokens++;
			}
			break;
		case COMMENT:
			if(c == '\n')
			{
				gdscript_id_t *id = arena_dynamic_alloc(id_arena, 2 * sizeof *id);
				id[0] = GDSCRIPT_COMMENT;
				id[1] = GDSCRIPT_LINE_END;
				new_token(arena, len_arena, src, pos, i - pos);
				new_token(arena, len_arena, src, i, 1);
				state = LEADING_WHITESPACE;
				num_tokens += 2;
			}
			break;
		case STRING:
			if (src[i - 1] != '\\' && (offset = string_ender(src + i, n - i, str_bitmask)) > 0)
			{
				gdscript_id_t *id = arena_dynamic_alloc(id_arena, sizeof *id);
				*id = GDSCRIPT_STRING;
				new_token(arena, len_arena, src, pos, i - pos + offset);
				i += offset - 1;
				str_bitmask = STR_BITMASK_NONE;
				state = WAITING_CHAR;
				++num_tokens;
			}
			break;
		}
	}
	if (state & (WSB_TOKEN | STRING | COMMENT))
	{
		gdscript_id_t *id = arena_dynamic_alloc(id_arena, sizeof *id);
		*id = GDSCRIPT_STRING;
		new_token(arena, len_arena, src, pos, n - pos);
		++num_tokens;
	}
	// NOTE: this would be better after a TOUCH of postprocessing
	/* lws_to_scopes(arena, id_arena, &num_tokens); */
	/* char** t = arena_dynamic_alloc(arena, sizeof *t); */
	/* t->string = src + n - 1; */
	/* t->pos = n - 1; */
	/* t->len = 0; */
	char** t = arena_dynamic_alloc(arena, sizeof *t);
	*t = src + n - 1;
	uint16_t* len = arena_dynamic_alloc(len_arena, sizeof *len);
	*len = 0;
	gdscript_id_t* id = arena_dynamic_alloc(id_arena, sizeof *id);
	*id = GDSCRIPT_EOF;
	++num_tokens;
	preprocess_tokens(arena->allocation, len_arena->allocation, id_arena->allocation, &num_tokens);
	lws_to_scopes(arena, len_arena, id_arena, &num_tokens);
	/* uint8_t* temp = realloc(arena, num_tokens * (sizeof *tokens + sizeof *ids)); */
	const size_t req_size = num_tokens * (sizeof *table->tokens + sizeof *table->ids + sizeof *table->lens);
	if (arena->cap < req_size)
	{
		arena_grow(arena, req_size);
	}
	table->tokens = arena->allocation;
	table->ids = memcpy(table->tokens + num_tokens, id_arena->allocation, num_tokens * sizeof *table->ids);
	table->lens = memcpy(table->ids + num_tokens, len_arena->allocation, num_tokens * sizeof *table->lens);
	table->num_tokens = num_tokens;
	return num_tokens;
}


static void lws_to_scopes(arena_t* token_arena, arena_t* len_arena, arena_t* id_arena, uint32_t* num_tokens)
{
	char** tokens = token_arena->allocation;
	uint16_t* lens = len_arena->allocation;
	gdscript_id_t* ids = id_arena->allocation;
	uint32_t lws0 = 0;
	uint32_t n = *num_tokens;
	SCRATCH* depth_arena = scratch_checkout_size(128 * sizeof lws0); // if this isn't enough the user deserves to segfault
	const size_t token_size = sizeof *tokens;
	const size_t id_size = sizeof *ids;
	const size_t len_size = sizeof(uint16_t);
	const gdscript_id_t scope_start = GDSCRIPT_SCOPE_START;
	const gdscript_id_t scope_end = GDSCRIPT_SCOPE_END;
	uint32_t* d;
	const size_t d_size = sizeof *d;
	for (uint32_t i = 0; i < n; ++i)
	{
		const uint32_t i0 = i;
		const char* token = tokens[i0];
		const size_t len = lens[i0];
		for (; ids[i] == GDSCRIPT_LEADING_WHITESPACE && i < n; ++i); // find the first non lws token
		uint32_t lws1 = i - i0;
		/* printf("i0: %d, i: %d, lws0: %d, lws1: %d,\n", i0, i, lws0, lws1); */
		if (lws1 > lws0)
		{
			/* printf("INSERT SCOPE START!\n"); */
			// insert a scope start at i0
			arena_insert(token_arena, i0 * token_size, &token, token_size);
			arena_insert(id_arena, i0 * id_size, &scope_start, id_size);
			arena_insert(len_arena, i0 * len_size, &len, len_size);
			tokens = token_arena->allocation;
			ids = id_arena->allocation;
			lws0 = lws1;
			++n;
			assert(depth_arena->size < 128 * sizeof lws0 && "REALLY, YOU HAVE MORE THAN 128 DEPTHS?  YOU DESERVE THIS ASSERT.");
			arena_push(depth_arena, &lws0, sizeof lws0);
		}
		else if (lws1 < lws0)
		{
			/* printf("INSERT SCOPE END!\n"); */
			lws0 = lws1;
			while(depth_arena->size > 0 && *(d = arena_peek(depth_arena, d_size)) > lws0)
			{
				depth_arena->size -= d_size;
				arena_insert(token_arena, i0 * token_size, &token, token_size);
				arena_insert(len_arena, i0 * len_size, &len, len_size);
				arena_insert(id_arena, i0 * id_size, &scope_end, id_size);
				++n;
			}
			tokens = token_arena->allocation;
			ids = id_arena->allocation;
		}
		for (; ids[i] != GDSCRIPT_LINE_END && i < n; ++i); // find the first non lws token
	}
	const char* token = tokens[n - 1];
	const uint16_t len = lens[n - 1];
	while (depth_arena->size > 0)
	{
		depth_arena->size -= d_size;
		arena_insert(token_arena, (n - 1) * token_size, &token, token_size);
		arena_insert(len_arena, (n - 1) * len_size, &len, len_size);
		arena_insert(id_arena, (n - 1) * id_size, &scope_end, id_size);
		++n;
	}
	tokens = token_arena->allocation;
	lens = len_arena->allocation;
	ids = id_arena->allocation;
	// trim the lws tokens
	trim_token(tokens, lens, ids, &n, GDSCRIPT_LEADING_WHITESPACE);
	*num_tokens = n;
}


static void classify_gdscript_tokens(gdscript_id_t* ids, const char** tokens, const uint16_t* lens, const uint32_t n)
{
	for (int i = 0; i < n; ++i)
	{
		if (ids[i] == GDSCRIPT_NONE)
		{
			const gdscript_id_t id = id_token(tokens[i], lens[i]);
			ids[i] = id;
			if (id == GDSCRIPT_OP_SUM || id == GDSCRIPT_OP_DIFFERENCE)
			{
				const int delta = ids[i] = GDSCRIPT_OP_SUM ?
					GDSCRIPT_OP_IDENTITY - GDSCRIPT_OP_SUM :
					GDSCRIPT_OP_NEGATION - GDSCRIPT_OP_DIFFERENCE;
				const gdscript_id_t prev = i > 0 ? ids[i - 1] : GDSCRIPT_NONE;
				for (uint8_t j = 0; j < NUM_UNARY_PRECEDERS; ++j)
				{
					if (prev == unary_preceders[j])
					{
						ids[i] += delta;
						break;
					}
				}
			}
		}
	}
}

static void coalesce_numbers(char** tokens, uint16_t* lens, gdscript_id_t* ids, uint32_t* num_tokens)
{
	uint32_t n = *num_tokens;
	for (uint32_t i = 0; i < n; ++i)
	{
		gdscript_id_t id = ids[i];
		if (id == GDSCRIPT_NUMBER_CANDIDATE || id == GDSCRIPT_OP_DOT || id == GDSCRIPT_PERIOD)
		{
			int index = -1;
			int len = 0;
			char* str = tokens[i]; // remember, token string is NO LONGER NULL TERMINATED :D
			for (int j = i; j < n; ++j)
			{
				len += lens[j];
				if (verify_number(str, len))
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
				if (index == i && ids[i] == GDSCRIPT_PERIOD || ids[i] == GDSCRIPT_OP_DOT)
				{
					continue;
				}
				ids[i] = GDSCRIPT_NUMBER;
				uint16_t new_len = lens[i];
				for (int j = i + 1; j <= index; ++j)
				{
					new_len += lens[j];
				}
				lens[i] = new_len;
				/* printf("coalesced: "); */
				/* for (uint32_t j = 0; j < token->len; ++j) */
				/* { */
				/* 	printf("%c", token->string[j]); */
				/* } */
				/* printf("\n"); */
				if (i < index && index < n - 1)
				{
					size_t k = n - index;
					memmove(tokens + i + 1, tokens + index + 1, k * sizeof *tokens);
					memmove(ids + i + 1, ids + index + 1, k * sizeof *ids);
					memmove(lens + i + 1, lens + index + 1, k * sizeof *lens);
				}
				n -= index - i;
			}
		}
	}
	*num_tokens = n;
}

static void trim_token (char** tokens, uint16_t* lens, gdscript_id_t* ids, uint32_t* num_tokens, const gdscript_id_t trim_token)
{
	uint32_t swap = 0;
	const uint32_t n = *num_tokens;
	/* uint16_t swap_len; */
	/* gdscript_id_t swap_id; */
	/* char* swap_token; */
	for (int i = swap; i < n; ++i)
	{
		if (ids[i] != trim_token)
		{
			/* ms_buffer_swap(ids + swap, ids + i, &swap_id, sizeof swap_id); */
			/* ms_buffer_swap(lens + swap, lens + i, &swap_len, sizeof swap_len); */
			/* ms_buffer_swap(tokens + swap, tokens + i, &swap_token, sizeof swap_token); */
			tokens[swap] = tokens[i];
			lens[swap] = lens[i];
			ids[swap] = ids[i];
			swap++;
		}
	}
	*num_tokens = swap;
}

static void preprocess_tokens(char** tokens, uint16_t* lens, gdscript_id_t* ids, uint32_t* num_tokens)
{
	classify_gdscript_tokens(ids, tokens, lens, *num_tokens - 1); // don't gotta classify the last token -- it's EOF
	/* classify_gdscript_tokens(tokens, ids, *num_tokens); */
	coalesce_numbers(tokens, lens, ids, num_tokens);
	/* printf("coalesce_numbers: %d\n", *num_tokens); */
	trim_token(tokens, lens, ids, num_tokens, GDSCRIPT_WHITESPACE);
	/* printf("trim_whitespace: %d\n", *num_tokens); */
	/* printf("cull_line_continuations: %d\n", *num_tokens); */
	trim_token(tokens, lens, ids, num_tokens, GDSCRIPT_COMMENT);
	/* printf("trim_comments: %d\n", *num_tokens); */
	cull_line_continuations(tokens, lens, ids, num_tokens);
	cull_pointless_lines2(tokens, lens, ids, num_tokens);
	/* printf("cull_pointless_lines: %d\n", *num_tokens); */
	const uint32_t n = *num_tokens;
	// convert remaining periods to dot operators
	const int period_delta = GDSCRIPT_OP_DOT - GDSCRIPT_PERIOD;
	for (int i = 0; i < n; ++i)
	{
		const gdscript_id_t id = ids[i];
		ids[i] += (id == GDSCRIPT_PERIOD) * period_delta;
	}
	// convert semicolons to line ends
	/* const int semicolon_delta = GDSCRIPT_LINE_END - GDSCRIPT_SEMICOLON; */
	/* for (int i = 0; i < n; ++i) */
	/* { */
	/* 	const gdscript_id_t id = ids[i]; */
	/* 	ids[i] += (id == GDSCRIPT_SEMICOLON) * semicolon_delta; */
	/* } */
	// convert remaining to identifiers
	for (int i = 0; i < n; ++i)
	{
		const gdscript_id_t id = ids[i];
		ids[i] += (id == GDSCRIPT_NONE) * GDSCRIPT_IDENTIFIER;
	}
}
