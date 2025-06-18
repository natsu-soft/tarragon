#include "gdscript_tokenizer.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "gdscript_parser.h"
#include "token.h"
#include "arena.h"
#include "scratch.h"
#include "mem_sys.h"

#define RESIZE_OVERHEAD  4096
#define RESIZE_SCALE  1.618

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

static void preprocess_tokens(token_t* tokens, gdscript_id_t* ids, uint32_t* num_tokens);
static uint32_t delimiting_token(const char* str, const unsigned int n);
static uint32_t string_starter(const char* str, const unsigned int n, uint8_t* str_bitmask);
static uint32_t string_ender(const char* str, const unsigned int n, const uint8_t str_bitmask);
static const gdscript_id_t id_token(const token_t* token);
static void new_token(arena_t* token_arena, const char* src, const int pos, const int len);
static const gdscript_id_t verify_number(const char* src, const uint32_t n);
static void cull_line_continuations(token_t* tokens, gdscript_id_t* ids, uint32_t* num_tokens);
static void lws_to_scopes(arena_t* token_arena, arena_t* id_arena, uint32_t* num_tokens);

static void do_nothing_with_token(const token_t* t, const char* message)
{
	return;
}

// TODO: early errors/warnings/notes
static void (*note_token_func)(const token_t*, const char* message) = do_nothing_with_token;
static void (*warn_token_func)(const token_t*, const char* message) = do_nothing_with_token;
static void (*error_token_func)(const token_t*, const char* message) = do_nothing_with_token;

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

static const gdscript_id_t id_token(const token_t* token)
{
	const char* str = token->string;
	char* temp;
	const uint32_t token_len = token->len;
	unsigned int len;
#define X(STR, ID)								\
	temp = STR;									\
	len = strlen(temp);							\
	len = len > token_len ? token->len : len;	\
	for (unsigned int i = 0; i < len; ++i)		\
	{											\
		if (str[i] != temp[i])					\
		{										\
			goto STOP_##ID;						\
		}										\
	}											\
	return GDSCRIPT_##ID;						\
	STOP_##ID:
	GDSCRIPT_BUILTIN_TOKENS
	if (('0' <= str[0] && str[0] < '9') || (str[0] == '.' && token_len > 1))
	{
		return GDSCRIPT_NUMBER_CANDIDATE;
	}
	else
	{
		return GDSCRIPT_NONE;
	}
#undef X
}



static void new_token(arena_t* token_arena, const char* src, const int pos, const int len)
{
	token_t* token = arena_dynamic_alloc(token_arena, sizeof *token);
	token->string = src + pos;
	token->pos = pos;
	token->len = len;
}

static const gdscript_id_t verify_number(const char* src, const uint32_t n)
{
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

void cull_pointless_lines(token_t* tokens, gdscript_id_t* ids, uint32_t* num_tokens)
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
		token_t swap_token;
		for (int i = swap + 1; i < n; ++i)
		{
			if (ids[swap] != GDSCRIPT_LINE_END || ids[i] != GDSCRIPT_LINE_END)
			{
				swap++;
				ms_buffer_swap(ids + swap, ids + i, &swap_id, sizeof swap_id);
				ms_buffer_swap(tokens + swap, tokens + i, &swap_token, sizeof swap_token);
				/* memcpy(ids + swap, ids + i, sizeof *ids); */
				/* memcpy(tokens + swap, tokens + i, sizeof *tokens); */
			}
		}
		*num_tokens = swap + 1;
	}
	else
	{
		*num_tokens = 0;
	}
}

uint32_t gdscript_tokenize(arena_t* arena, token_t** tokens, gdscript_id_t** ids, const char* src, const uint32_t n)
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
	SCRATCH* id_arena = scratch_checkout();
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
					new_token(arena, src, i, 1);
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
					new_token(arena, src, i, offset);
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
				new_token(arena, src, pos, i - pos);
				new_token(arena, src, i, offset);
				state = c == '\n' ? LEADING_WHITESPACE : WAITING_CHAR;
				i += offset - 1;
				num_tokens += 2;
			} else if (isspace(c)) {
				gdscript_id_t *id = arena_dynamic_alloc(id_arena, sizeof *id);
				*id = GDSCRIPT_NONE;
				new_token(arena, src, pos, i - pos);
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
				new_token(arena, src, i, offset);
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
				new_token(arena, src, i, 1);
				num_tokens++;
			}
			break;
		case COMMENT:
			if(c == '\n')
			{
				gdscript_id_t *id = arena_dynamic_alloc(id_arena, 2 * sizeof *id);
				id[0] = GDSCRIPT_COMMENT;
				id[1] = GDSCRIPT_LINE_END;
				new_token(arena, src, pos, i - pos);
				new_token(arena, src, i, 1);
				state = LEADING_WHITESPACE;
				num_tokens += 2;
			}
			break;
		case STRING:
			if (src[i - 1] != '\\' && (offset = string_ender(src + i, n - i, str_bitmask)) > 0)
			{
				gdscript_id_t *id = arena_dynamic_alloc(id_arena, sizeof *id);
				*id = GDSCRIPT_STRING;
				new_token(arena, src, pos, i - pos + offset);
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
		new_token(arena, src, pos, n - pos);
		++num_tokens;
	}
	// NOTE: this would be better after a TOUCH of postprocessing
	/* lws_to_scopes(arena, id_arena, &num_tokens); */
	token_t* t = arena_dynamic_alloc(arena, sizeof *t);
	t->string = src + n - 1;
	t->pos = n - 1;
	t->len = 0;
	gdscript_id_t* id = arena_dynamic_alloc(id_arena, sizeof *id);
	*id = GDSCRIPT_EOF;
	++num_tokens;
	preprocess_tokens(arena->allocation, id_arena->allocation, &num_tokens);
	lws_to_scopes(arena, id_arena, &num_tokens);
	/* uint8_t* temp = realloc(arena, num_tokens * (sizeof *tokens + sizeof *ids)); */
	const size_t req_size = num_tokens * (sizeof **tokens + sizeof **ids);
	if (arena->cap < req_size)
	{
		arena_grow(arena, req_size);
	}
	*tokens = arena->allocation;
	*ids = memcpy(*tokens + num_tokens, id_arena->allocation, num_tokens * sizeof **ids);
	return num_tokens;
}


static void lws_to_scopes(arena_t* token_arena, arena_t* id_arena, uint32_t* num_tokens)
{
	token_t* tokens = token_arena->allocation;
	gdscript_id_t* ids = token_arena->allocation;
	uint32_t lws0 = 0;
	uint32_t lws1 = 0;
	for (uint32_t i = 0; i < *num_tokens; ++i)
	{
		switch(ids[i])
		{
		case GDSCRIPT_LINE_END:
			lws1 = 0;
			break;
		case GDSCRIPT_LEADING_WHITESPACE:
			++lws1;
			break;
		default:
		}
	}
}


static void classify_gdscript_tokens(token_t* tokens, gdscript_id_t* ids, const uint32_t n)
{
	for (int i = 0; i < n; ++i)
	{
		if (ids[i] == GDSCRIPT_NONE)
		{
			ids[i] = id_token(tokens + i);
		}
	}
}

// NOTE: not working :(
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
			ms_buffer_swap(ids + swap, ids + i, &swap_id, sizeof swap_id);
			ms_buffer_swap(tokens + swap, tokens + i, &swap_token, sizeof swap_token);
			swap++;
		}
	}
	*num_tokens = swap;
}

static void preprocess_tokens(token_t* tokens, gdscript_id_t* ids, uint32_t* num_tokens)
{
	classify_gdscript_tokens(tokens, ids, *num_tokens - 1); // don't gotta classify the last token -- it's EOF
	/* classify_gdscript_tokens(tokens, ids, *num_tokens); */
	coalesce_numbers(tokens, ids, num_tokens);
	/* printf("coalesce_numbers: %d\n", *num_tokens); */
	trim_token(tokens, ids, num_tokens, GDSCRIPT_WHITESPACE);
	/* printf("trim_whitespace: %d\n", *num_tokens); */
	cull_line_continuations(tokens, ids, num_tokens);
	/* printf("cull_line_continuations: %d\n", *num_tokens); */
	trim_token(tokens, ids, num_tokens, GDSCRIPT_COMMENT);
	/* printf("trim_comments: %d\n", *num_tokens); */
	cull_pointless_lines(tokens, ids, num_tokens);
	/* printf("cull_pointless_lines: %d\n", *num_tokens); */
	const uint32_t n = *num_tokens;
	// convert remaining periods to dot operators
	const int period_delta = GDSCRIPT_OP_DOT - GDSCRIPT_PERIOD;
	for (int i = 0; i < n; ++i)
	{
		const gdscript_id_t id = ids[i];
		ids[i] += (id == GDSCRIPT_PERIOD) * period_delta;
	}
	// convert remaining to identifiers
	for (int i = 0; i < n; ++i)
	{
		const gdscript_id_t id = ids[i];
		ids[i] += (id == GDSCRIPT_NONE) * GDSCRIPT_IDENTIFIER;
	}
}

/* static void pv(const char* str, const int n) */
/* { */
/* 	struct */
/* 	{ */
/* 		uint16_t line; */
/* 		uint16_t col; */
/* 	} coords = {0}; */
/* 	enum __attribute__((packed)) */
/* 	{ */
/* 		LEADING_WHITESPACE = 0x00, */
/* 		WSB_TOKEN, */
/* 		WAITING_CHAR, */
/* 		STRING, */
/* 		COMMENT, */
/* 	} state = LEADING_WHITESPACE; */
/* 	enum __attribute__((packed)) */
/* 	{ */
/* 		STR_BITMASK_NONE = 0x00, */
/* 		#define STR_X(STR, BITS, NAME) NAME = BITS, */
/* 		STRING_DELIMITERS */
/* 		#undef STR_X */
/* 	} str_bitmask = STR_BITMASK_NONE; */
/* 	#define HANDLE_LEADING_WHITESPACE \ */
/* 		printf("%d:%d: leading ws\n", line, column); */
/* 	#define HANDLE_WHITESPACE \ */
/* 		printf("%d:%d: ws\n", line, column); */
/* 	#define HANDLE_DELIMITING_TOKEN \ */
/* 		char temp[offset + 1]; \ */
/* 		memcpy(temp, str + i, offset); \ */
/* 		temp[offset] = '\0'; \ */
/* 		printf("%d:%d: delimiter: %s\n", line, column, temp); */
/* 	#define HANDLE_TOKEN \ */
/* 		char temp[i - pos + 1]; \ */
/* 		memcpy(temp, str + pos, i - pos); \ */
/* 		temp[i - pos] = '\0'; \ */
/* 		printf("%d:%d: token: %s\n", coords.line, coords.col, temp); */
/* 	#define HANDLE_COMMENT \ */
/* 		char temp[i - pos + 1]; \ */
/* 		memcpy(temp, str + pos, i - pos); \ */
/* 		temp[i - pos] = '\0'; \ */
/* 		printf("%d:%d: %s\n\n", line, column, temp); */
/* 	#define HANDLE_STRING \ */
/* 		char temp[i - pos + offset + 2]; \ */
/* 		memcpy(temp, str + pos, i - pos + offset); \ */
/* 		temp[i - pos + offset + 1] = '\0'; \ */
/* 		printf("%d:%d: string: %s\n", coords.line, coords.col, temp); */
/* 	#define HANDLE_TRAILING \ */
/* 		char temp[n - pos + 1]; \ */
/* 		for (int i = pos; i < n; ++i) \ */
/* 		{ \ */
/* 			printf("%c", str[i]); \ */
/* 		} \ */
/* 		memcpy(temp, str + pos, n - pos); \ */
/* 		temp[n - pos] = '\0'; \ */
/* 		printf("%d:%d: trailing: %s\n", coords.line, coords.col, temp); */
/* 	uint32_t offset; */
/* 	uint32_t pos; */
/* 	uint16_t line; */
/* 	uint16_t column; */

/* 	ITERATE_OVER_SOURCE */

/* 	#undef HANDLE_LEADING_WHITESPACE */
/* 	#undef HANDLE_DELIMITING_TOKEN */
/* 	#undef HANDLE_TOKEN */
/* 	#undef HANDLE_COMMENT */
/* 	#undef HANDLE_STRING */
/* 	#undef HANDLE_TRAILING */
/* } */

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

