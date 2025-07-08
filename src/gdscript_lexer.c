#include "gdscript_lexer.h"
#include "gdscript_parser.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>
#include "arena.h"
#include "deck.h"
#include "stack.h"
#include "scratch.h"

#define true 1
#define false 0
typedef int8_t bool_t;

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
static int8_t num_unary_preceders = 0;
static gdscript_id_t* unary_preceders = NULL;

__attribute__((constructor))
static void init_lexer(void)
{
	int32_t l = 0;
#define X(_, __, LEN) l += LEN;
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
	int32_t i = 0;
	char* str;
#define X(ENUM, STR, LEN)												\
	builtin.tokens[i] = char_arena.allocation + char_arena.size;	\
	str = STR; \
	arena_push(&char_arena, str, LEN);								\
	builtin.lens[i] = LEN;											\
	builtin.ids[i] = GDSCRIPT_##ENUM;								\
	++i;
	GDSCRIPT_BUILTIN_TOKENS
#undef X
/* #define X(_, ENUM) unary_preceders[i++] = GDSCRIPT_##ENUM; */
#define X(...) ++num_unary_preceders;
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
/* #define X(ENUM) unary_preceders[i++] = GDSCRIPT_##ENUM; */
#define X(...) ++num_unary_preceders;
		UNARY_STRUCT
#undef X
	unary_preceders = calloc(num_unary_preceders, sizeof *unary_preceders);
	i = 0;
#define X(ENUM,...) unary_preceders[i++] = GDSCRIPT_##ENUM;
	GDSCRIPT_OPERATORS
#undef X
#define X(ENUM) unary_preceders[i++] = GDSCRIPT_##ENUM;
		UNARY_STRUCT
#undef X
#undef UNARY_STRUCT
}


static void classify_gdscript_tokens(const char* src, gdscript_id_t* ids, const int16_t* tokens, const int16_t* lens, const uint32_t n);
static void preprocess_tokens(const char* src, int16_t* offsets, int16_t* lens, gdscript_id_t* ids, int32_t* num_tokens);
static int32_t delimiting_token(const char* str, const unsigned int n);
static int32_t string_starter(const char* str, const unsigned int n, uint8_t* str_bitmask);
static int32_t string_ender(const char* str, const unsigned int n, const uint8_t str_bitmask);
static const gdscript_id_t id_token(const char* token, const int16_t len);
static inline void new_token(arena_t* offset_arena, arena_t* len_arena, const int16_t pos, const int16_t len);
static const gdscript_id_t verify_number(const char* src, const int32_t n);
static void cull_line_continuations(int16_t* offsets, int16_t* lens, gdscript_id_t* ids, int32_t* num_tokens);
static void lws_to_scopes(arena_t* offset_arena, arena_t* len_arena, arena_t* id_arena, int32_t* num_tokens);
static void trim_token(int16_t* offsets, int16_t* lens, gdscript_id_t* ids, int32_t* num_tokens, const gdscript_id_t trim_token);

/* static void do_nothing_with_token(const char** t, const char* message) */
/* { */
/* 	return; */
/* } */

// TODO: early errors/warnings/notes
/* static void (*note_token_func)(const char**, const char* message) = do_nothing_with_token; */
/* static void (*warn_token_func)(const char**, const char* message) = do_nothing_with_token; */
/* static void (*error_token_func)(const char**, const char* message) = do_nothing_with_token; */

// MR. GOD IS ENTERTAINED BY HUMOR
static int32_t delimiting_token(const char* str, const unsigned int n)
{
	char* temp;
#define X(ID, STR, LEN)								\
	temp = STR;									\
	for (uint32_t i = 0; i < LEN; ++i)			\
	{											\
		if (i < n)								\
		{										\
			if (str[i] != temp[i])				\
			{									\
				goto STOP_##ID;					\
			}									\
		}										\
		else									\
		{										\
			goto STOP_##ID;						\
		}										\
	}											\
	return LEN;									\
	STOP_##ID:
	GDSCRIPT_DELIMITING_TOKENS
	return 0;
	#undef X
}

static int32_t string_starter(const char* str, const unsigned int n, uint8_t* str_bitmask)
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

static int32_t string_ender(const char* str, const unsigned int n, const uint8_t str_bitmask)
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

static const gdscript_id_t id_token(const char* token, const int16_t len)
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

static inline void new_token(arena_t* offset_arena, arena_t* len_arena, const int16_t pos, const int16_t len)
{
	arena_dynamic_push(offset_arena, &pos, sizeof pos);
	arena_dynamic_push(len_arena, &len, sizeof len);
}

static const gdscript_id_t verify_number(const char* src, const int32_t n)
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

static void cull_line_continuations(int16_t* offsets, int16_t* lens, gdscript_id_t* ids, int32_t* num_tokens)
{
	uint32_t swap = 0;
	const uint32_t n = *num_tokens;
	for (uint32_t i = swap; i < n; ++i)
	{
		const gdscript_id_t id = ids[i];
		if (id != GDSCRIPT_LINE_CONTINUE)
		{
			offsets[swap] = offsets[i];
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
			offsets[swap] = offsets[i];
			lens[swap] = lens[i];
			ids[swap] = ids[i];
			++swap;
		}
		else // NOTE: for EOF it marks the 
		{
			for (++i; i < n && ids[i] != GDSCRIPT_LINE_END; ++i)
			{
				offsets[swap] = offsets[i];
				lens[swap] = lens[i];
				ids[swap] = GDSCRIPT_ERROR;
				++swap;
			}
		}
	}
	*num_tokens = swap;
}


static void cull_pointless_lines(int16_t* offsets, int16_t* lens, gdscript_id_t* ids, int32_t* num_tokens)
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
		offsets[swap] = offsets[i];
		lens[swap] = lens[i];
		ids[swap] = id;
		++swap;
	}
	*num_tokens = swap;
}

int32_t gdscript_lex(arena_t* arena, token_table_t* table, const char* src, const int32_t n)
{
	int32_t num_tokens = 0;
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
	int32_t offset = 0;
	int32_t pos = 0;
	arena->size = 0;
	SCRATCH* len_arena = scratch_checkout_size(n * sizeof *table->lens);
	SCRATCH* id_arena = scratch_checkout_size(n * sizeof *table->ids);
	SCRATCH* missing_arena = scratch_checkout_size(n * sizeof *table->ids);
	// TODO: rip-off Carbon.  Just insert missing things appropriately
	stack_t* missing_stack = alloca(sizeof *missing_stack);
	*missing_stack = (stack_t)
	{
		.allocation = missing_arena->allocation,
		.item_size = sizeof *table->ids,
		.cap = n
	};
	for (int32_t i = 0; i < n; ++i)
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
					new_token(arena, len_arena, i, 1);
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
					new_token(arena, len_arena, i, offset);
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
				new_token(arena, len_arena, pos, i - pos);
				new_token(arena, len_arena, i, offset);
				state = c == '\n' ? LEADING_WHITESPACE : WAITING_CHAR;
				i += offset - 1;
				num_tokens += 2;
			} else if (isspace(c)) {
				gdscript_id_t *id = arena_dynamic_alloc(id_arena, sizeof *id);
				*id = GDSCRIPT_NONE;
				new_token(arena, len_arena, pos, i - pos);
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
				new_token(arena, len_arena, i, offset);
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
				new_token(arena, len_arena,  i, 1);
				num_tokens++;
			}
			break;
		case COMMENT:
			if(c == '\n')
			{
				gdscript_id_t *id = arena_dynamic_alloc(id_arena, 2 * sizeof *id);
				id[0] = GDSCRIPT_COMMENT;
				id[1] = GDSCRIPT_LINE_END;
				new_token(arena, len_arena, pos, i - pos);
				new_token(arena, len_arena, i, 1);
				state = LEADING_WHITESPACE;
				num_tokens += 2;
			}
			break;
		case STRING:
			if (src[i - 1] != '\\' && (offset = string_ender(src + i, n - i, str_bitmask)) > 0)
			{
				gdscript_id_t *id = arena_dynamic_alloc(id_arena, sizeof *id);
				*id = GDSCRIPT_STRING;
				new_token(arena, len_arena, pos, i - pos + offset);
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
		new_token(arena, len_arena, pos, n - pos);
		++num_tokens;
	}
	int16_t* eof_offset = arena_dynamic_alloc(arena, sizeof *eof_offset);
	*eof_offset = n;
	uint16_t* eof_len = arena_dynamic_alloc(len_arena, sizeof *eof_len);
	*eof_len = 0;
	gdscript_id_t* eof_id = arena_dynamic_alloc(id_arena, sizeof *eof_id);
	*eof_id = GDSCRIPT_EOF;
	++num_tokens;
	// the long requested "touch" of processing
	/* int16_t* offsets = arena->allocation; */
	/* int16_t* lens = len_arena->allocation; */
	/* gdscript_id_t* ids = id_arena->allocation; */
	preprocess_tokens(src, arena->allocation, len_arena->allocation, id_arena->allocation, &num_tokens);
	lws_to_scopes(arena, len_arena, id_arena, &num_tokens);
	/* uint8_t* temp = realloc(arena, num_tokens * (sizeof *tokens + sizeof *ids)); */
	const size_t req_size = num_tokens * (sizeof *table->offsets + sizeof *table->ids + sizeof *table->lens);
	if (arena->cap < req_size)
	{
		arena_grow(arena, req_size);
	}
	table->src = src;
	table->offsets = arena->allocation;
	table->ids = memcpy(table->offsets + num_tokens, id_arena->allocation, num_tokens * sizeof *table->ids);
	table->lens = memcpy(table->ids + num_tokens, len_arena->allocation, num_tokens * sizeof *table->lens);
	table->num_tokens = num_tokens;
	return num_tokens;
}


static void lws_to_scopes(arena_t* offset_arena, arena_t* len_arena, arena_t* id_arena, int32_t* num_tokens)
{
	int16_t* offsets = offset_arena->allocation;
	int16_t* lens = len_arena->allocation;
	gdscript_id_t* ids = id_arena->allocation;
	int32_t lws0 = 0;
	int32_t n = *num_tokens;
	SCRATCH* depth_arena = scratch_checkout_size(128 * sizeof lws0); // if this isn't enough the user deserves to segfault
	const size_t offset_size = sizeof *offsets;
	const size_t id_size = sizeof *ids;
	const size_t len_size = sizeof *lens;
	const gdscript_id_t scope_start = GDSCRIPT_SCOPE_START;
	const gdscript_id_t scope_end = GDSCRIPT_SCOPE_END;
	uint32_t* d;
	const size_t d_size = sizeof *d;
	for (int32_t i = 0; i < n; ++i)
	{
		const int32_t i0 = i;
		const int16_t offset = offsets[i0];
		const size_t len = lens[i0];
		for (; ids[i] == GDSCRIPT_LEADING_WHITESPACE && i < n; ++i); // find the first non lws token
		int32_t lws1 = i - i0;
		/* printf("i0: %d, i: %d, lws0: %d, lws1: %d,\n", i0, i, lws0, lws1); */
		if (lws1 > lws0)
		{
			/* printf("INSERT SCOPE START!\n"); */
			// insert a scope start at i0
			arena_insert(offset_arena, i0 * offset_size, &offset, offset_size);
			arena_insert(id_arena, i0 * id_size, &scope_start, id_size);
			arena_insert(len_arena, i0 * len_size, &len, len_size);
			offsets = offset_arena->allocation;
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
				arena_insert(offset_arena, i0 * offset_size, &offset, offset_size);
				arena_insert(len_arena, i0 * len_size, &len, len_size);
				arena_insert(id_arena, i0 * id_size, &scope_end, id_size);
				++n;
			}
			offsets = offset_arena->allocation;
			ids = id_arena->allocation;
		}
		for (; ids[i] != GDSCRIPT_LINE_END && i < n; ++i); // find the first non lws token
	}
	const int16_t offset = offsets[n - 1];
	const int16_t len = lens[n - 1];
	while (depth_arena->size > 0)
	{
		depth_arena->size -= d_size;
		arena_insert(offset_arena, (n - 1) * offset_size, &offset, offset_size);
		arena_insert(len_arena, (n - 1) * len_size, &len, len_size);
		arena_insert(id_arena, (n - 1) * id_size, &scope_end, id_size);
		++n;
	}
	offsets = offset_arena->allocation;
	lens = len_arena->allocation;
	ids = id_arena->allocation;
	// trim the lws tokens
	trim_token(offsets, lens, ids, &n, GDSCRIPT_LEADING_WHITESPACE);
	*num_tokens = n;
}


static void classify_gdscript_tokens(const char* src, gdscript_id_t* ids, const int16_t* offsets, const int16_t* lens, const uint32_t n)
{
	gdscript_id_t prev_id = GDSCRIPT_NONE;
	for (int i = 0; i < n; ++i)
	{
		gdscript_id_t id = ids[i];
		if (id == GDSCRIPT_NONE)
		{
			id = id_token(src + offsets[i], lens[i]);
			ids[i] = id;
			if (id == GDSCRIPT_OP_SUM || id == GDSCRIPT_OP_DIFFERENCE)
			{
				const int delta = ids[i] = GDSCRIPT_OP_SUM ?
					GDSCRIPT_OP_IDENTITY - GDSCRIPT_OP_SUM :
					GDSCRIPT_OP_NEGATION - GDSCRIPT_OP_DIFFERENCE;
				for (uint8_t j = 0; j < num_unary_preceders; ++j)
				{
					if (prev_id == unary_preceders[j])
					{
						ids[i] += delta;
						break;
					}
				}
			}
		}
		prev_id = ids[i];
	}
}

static void coalesce_numbers(const char* const src, int16_t* offsets, int16_t* lens, gdscript_id_t* ids, int32_t* num_tokens)
{
	uint32_t n = *num_tokens;
	for (uint32_t i = 0; i < n; ++i)
	{
		gdscript_id_t id = ids[i];
		if (id == GDSCRIPT_NUMBER_CANDIDATE || id == GDSCRIPT_PERIOD)
		{
			int index = -1;
			int len = 0;
			char* str = src + offsets[i]; // remember, token string is NO LONGER NULL TERMINATED :D
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
				if (index == i && ids[i] == GDSCRIPT_PERIOD)
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
					memmove(offsets + i + 1, offsets + index + 1, k * sizeof *offsets);
					memmove(ids + i + 1, ids + index + 1, k * sizeof *ids);
					memmove(lens + i + 1, lens + index + 1, k * sizeof *lens);
				}
				n -= index - i;
			}
		}
	}
	*num_tokens = n;
}

static void trim_token(int16_t* offsets, int16_t* lens, gdscript_id_t* ids, int32_t* num_tokens, const gdscript_id_t trim_token)
{
	uint32_t swap = 0;
	const uint32_t n = *num_tokens;
	for (int i = swap; i < n; ++i)
	{
		if (ids[i] != trim_token)
		{
			offsets[swap] = offsets[i];
			lens[swap] = lens[i];
			ids[swap] = ids[i];
			swap++;
		}
	}
	*num_tokens = swap;
}

static void preprocess_tokens(const char* src, int16_t* offsets, int16_t* lens, gdscript_id_t* ids, int32_t* num_tokens)
{
	classify_gdscript_tokens(src, ids, offsets, lens, *num_tokens - 1); // don't gotta classify the last token -- it's EOF
	coalesce_numbers(src, offsets, lens, ids, num_tokens);
	trim_token(offsets, lens, ids, num_tokens, GDSCRIPT_WHITESPACE);
	trim_token(offsets, lens, ids, num_tokens, GDSCRIPT_COMMENT);
	cull_line_continuations(offsets, lens, ids, num_tokens);
	cull_pointless_lines(offsets, lens, ids, num_tokens);
	const uint32_t n = *num_tokens;
	// convert remaining to identifiers
	for (int i = 0; i < n; ++i)
	{
		const gdscript_id_t id = ids[i];
		ids[i] += (id == GDSCRIPT_NONE) * GDSCRIPT_IDENTIFIER;
	}
}

#define PAIRS \
	X(PAREN) \
	X(BRACE) \
	X(BRACKET)

#define X
bool_t paired_paren(const gdscript_id_t* ids, int32_t* index, const int32_t n)
{
	int16_t depth = 0;
	for (int32_t i = *index; i < n; ++i)
	{
		const gdscript_id_t id = ids[i];
		if (id == GDSCRIPT_PAREN_CLOSE)
		{
			if (depth == 0)
			{
				*index = i;
				return true;
			}
		}
	}
	return false;
}

void gdscript_pair_linting(token_table_t* table)
{
	const int32_t n = table->num_tokens;
	for (int32_t i = 0; i < n; ++i)
	{
	}
}

void gdscript_polska(token_table_t* dest_table, const token_table_t *src_table)
{
	/* TODO */
}
