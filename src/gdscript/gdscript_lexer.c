#include "gdscript_lexer.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>
#include "tokens.h"
#include "token_ids.h"
#include "../allocators/arena.h"
#include "../allocators/scratch.h"
#include "../allocators/workpad.h"
#include "../macro_stack.h"

#define STRING_ENDERS							\
	STR_X("'''",    3, 0x01, TRIPLE_SINGLE_QOUTE)	\
	STR_X("\"\"\"", 3, 0x02, TRIPLE_DOUBLE_QUOTE)	\
	STR_X("'",      1, 0x04, SINGLE_QUOTE)			\
	STR_X("\"",     1, 0x08, DOUBLE_QUOTE)				\

#define STRING_DELIMITERS								\
	STR_X("r'''",    3, 0x10, RAW_TRIPLE_SINGLE_QUOTE)	\
	STR_X("r\"\"\"", 4, 0x20, RAW_TRIPLE_DOUBLE_QUOTE)	\
	STR_X("r'",      2, 0x40, RAW_SINGLE_QUOTE)		    \
	STR_X("r\"",     2, 0x80, RAW_DOUBLE_QUOTE)		    \
	STR_X("&\"",     2, 0x81, STRING_NAME)				\
	STR_X("^\"",     2, 0x82, NODE_PATH)				    \
	STRING_ENDERS										\

/* // X(ENCOUNTER, EXPECTED, ...) // use __VA_ARGS__ JUST for enum (which im fine with)  */
/* #define EXPECTED_TOKEN_LIST \ */
/* 	X(PAREN_OPEN, PAREN_CLOSE) \ */
/* 	X(BRACE_OPEN, BRACE_CLOSE) \ */
/* 	X(BRACKET_OPEN, BRACKET_CLOSE) \ */
/* 	X(IF, COLON) \ */
/* 	X(ELIF, COLON) \ */
/* 	X(ELSE, COLON) \ */
/* 	X(FOR, COLON) \ */
/* 	X(WHILE, COLON) \ */
/* 	X(MATCH, COLON) */
/* // NOTE: just do these ones manually since they're SPECIAL */
/* 	/\* X(FUNC, PAREN_OPEN, PAREN_CLOSE, COLON)	\ *\/ */
/* 	/\* X(ENUM, BRACE_OPEN, BRACE_CLOSE) // OR: just handle enum manually (not a bad idea tbqh) *\/ */


// NOTE: THE FUNNY METHOD IS FASTER!
/* static struct */
/* { */
/* 	char** tokens; */
/* 	uint8_t* lens; */
/* 	uint8_t size; */
/* } delimiters = {0}; */

static struct {
	char** tokens;
	token_id_t* ids;
	uint8_t* lens;
} builtin = {0};

 // NOTE: THIS IS BAD STOP USING IT
static int8_t num_unary_preceders = 0;
static token_id_t* unary_preceders = NULL;

/* __attribute__((constructor)) */
void init_lexer(void)
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
		.addr = builtin.ids + GDSCRIPT_NUM_BUILTIN,
		.cap = l
	};
	int32_t i = 0;
	char* str;
#define X(ENUM, STR, LEN)												\
	builtin.tokens[i] = char_arena.addr + char_arena.size;	\
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
		/* X(LEADING_WHITESPACE) */				\
		X(COMMA)							\
		X(PAREN_OPEN)						\
		X(BRACKET_OPEN)						\
		X(SCOPE_OPEN)						\
		X(SCOPE_CLOSE)
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


static void classify_gdscript_tokens(const char* src, token_id_t* ids, const int32_t* tokens, const int16_t* lens, const uint32_t n);
static void preprocess_tokens(const char* src, arena_t* offset_arena, arena_t* len_arena, arena_t* id_arena, int32_t* num_tokens);
static const int32_t delimiting_token(const char* str, const unsigned int n);
static const int32_t string_starter(const char* str, const unsigned int n, uint8_t* str_bitmask);
static const int32_t string_ender(const char* str, const unsigned int n, const uint8_t str_bitmask);
static const token_id_t id_token(const char* token, const int16_t len);
static inline void new_token(arena_t* offset_arena, arena_t* len_arena, const int32_t pos, const int16_t len);
static const token_id_t verify_number(const char* src, const int32_t n);
static void cull_line_continuations(int32_t* offsets, int16_t* lens, token_id_t* ids, int32_t* num_tokens);
static void lws_to_scopes(arena_t* offset_arena, arena_t* len_arena, arena_t* id_arena, int32_t* num_tokens);
static void trim_token(int32_t* offsets, int16_t* lens, token_id_t* ids, int32_t* num_tokens, const token_id_t trim_token);

/* static void do_nothing_with_token(const char** t, const char* message) */
/* { */
/* 	return; */
/* } */

// TODO: early errors/warnings/notes
/* static void (*note_token_func)(const char**, const char* message) = do_nothing_with_token; */
/* static void (*warn_token_func)(const char**, const char* message) = do_nothing_with_token; */
/* static void (*error_token_func)(const char**, const char* message) = do_nothing_with_token; */

// MR. GOD IS ENTERTAINED BY HUMOR
static const int32_t delimiting_token(const char* str, const unsigned int n)
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

static const int32_t string_starter(const char* str, const unsigned int n, uint8_t* str_bitmask)
{
	char* temp;
	unsigned int len;
	#define STR_X(STR, LEN, BITFLAG, ID)				\
	temp = STR; \
	for (unsigned int i = 0; i < LEN; ++i) \
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
	return LEN; \
	STOP_##ID:
	STRING_DELIMITERS
	return 0;
	#undef STR_X
}

static const int32_t string_ender(const char* str, const unsigned int n, const uint8_t str_bitmask)
{
	char* temp;
	unsigned int len;
	#define STR_X(STR, LEN, BITFLAG, ...)					\
	case BITFLAG: \
		temp = STR; \
		for (unsigned int i = 0; i < LEN; ++i)	\
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
		return LEN; \
	STOP_##BITFLAG: \
		break;
	switch (str_bitmask)
	{
		STRING_ENDERS
	}
	return 0;
	#undef STR_X
}

static const token_id_t id_token(const char* token, const int16_t len)
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
		/* return GDSCRIPT_NONE; */
		return GDSCRIPT_IDENTIFIER;
	}
}

/* static void push_expected_tokens(stack_t* stack, const token_id_t id) */
/* { */
/* 	switch(id) */
/* 	{ */
/* 	#define X(A,B) case GDSCRIPT_##A: stack_push8(stack, GDSCRIPT_##B); return; */
/* 		EXPECTED_TOKEN_LIST */
/* 	#undef X */
/* 	case GDSCRIPT_ENUM: */
/* 		stack_push8(stack, GDSCRIPT_BRACE_OPEN); */
/* 		stack_push8(stack, GDSCRIPT_BRACE_CLOSE); */
/* 		return; */
/* 	case GDSCRIPT_FUNC: */
/* 		stack_push8(stack, GDSCRIPT_PAREN_OPEN); */
/* 		stack_push8(stack, GDSCRIPT_PAREN_CLOSE); */
/* 		stack_push8(stack, GDSCRIPT_COLON); */
/* 		return; */
/* 	} */
/* } */

/* static const token_id_t make_missing(const token_id_t id) */
/* { */
/* 	#define MISSING_LIST \ */
/* 		X(COLON) \ */
/* 		X(PAREN_OPEN) \ */
/* 		X(PAREN_CLOSE) \ */
/* 		X(BRACE_OPEN) \ */
/* 		X(BRACE_CLOSE) \ */
/* 		X(BRACKET_CLOSE) */
/* 	switch(id) */
/* 	{ */
/* 	#define X(ID) case GDSCRIPT_##ID: return GDSCRIPT_MISSING_##ID; */
/* 		MISSING_LIST */
/* 	#undef X */
/* 	} */
/* 	return GDSCRIPT_NONE; */
/* 	#undef MISSING_LIST */
/* } */


static inline void new_token(arena_t* offset_arena, arena_t* len_arena, const int32_t pos, const int16_t len)
{
	/* arena_dynamic_push(offset_arena, &pos, sizeof pos); */
	/* arena_dynamic_push(len_arena, &len, sizeof len); */
	arena_push(offset_arena, &pos, 4);
	arena_push(len_arena, &len, 2);
}


static const token_id_t verify_number(const char* src, const int32_t n)
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
	for (int32_t i = 0; i < n; ++i)
	{
		const char c = src[i];
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
		// TODO: let the shunter + checker handle unary negative.
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

static void cull_line_continuations(int32_t* offsets, int16_t* lens, token_id_t* ids, int32_t* num_tokens)
{
	uint32_t swap = 0;
	const uint32_t n = *num_tokens;
	for (uint32_t i = swap; i < n; ++i)
	{
		const token_id_t id = ids[i];
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
				ids[swap] = GDSCRIPT_TOKENS_AFTER_CONTINUATION;
				++swap;
			}
		}
	}
	*num_tokens = swap;
}


static void cull_pointless_lines(int32_t* offsets, int16_t* lens, token_id_t* ids, int32_t* num_tokens)
{
	int32_t swap = 0;
	int32_t endl = 0;
	const int32_t n = *num_tokens;
	for (int32_t i = swap; i < n; ++i)
	{
		const token_id_t id = ids[i];
		if (id == GDSCRIPT_LINE_END)
		{
			endl = i;
			for (int32_t j = i + 1; j < n; ++j)
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
		#define STR_X(STR, LEN, BITS, NAME) NAME = BITS,
		STRING_DELIMITERS
		#undef STR_X
	} str_bitmask = STR_BITMASK_NONE;
	int32_t offset = 0;
	int32_t pos = 0;
	arena->size = 0;
	arena_t* len_arena = alloca(2 * sizeof *len_arena);
	arena_t* id_arena = len_arena + 1;
	int16_t* lens = scratch_alloc(n * (sizeof *table->lens + sizeof(token_id_t)));
	*len_arena = (arena_t)
	{
		.addr = lens,
		.cap = n * sizeof *table->lens
	};
	*id_arena = (arena_t)
	{
		.addr = lens + n,
		.cap = n * sizeof(token_id_t)
	};
	/* arena_t* len_arena = SCRATCH_ARENA(n * sizeof *table->lens, 0); */
	/* WORKPAD* id_arena = workpad_checkout(n * sizeof(token_id_t)); */
	/* arena_t* id_arena = SCRATCH_ARENA(n * sizeof(token_id_t), 1); */
	/* arena_t* expected_arena = scratch_checkout_size(n * sizeof(token_id_t)); */
	// TODO: rip-off Carbon.  Just insert missing things appropriately
	/* stack_t* expected_stack = alloca(sizeof *expected_stack); */
	/* *expected_stack = (stack_t) */
	/* { */
	/* 	.allocation = expected_arena->allocation, */
	/* 	.item_size = sizeof(token_id_t), */
	/* 	.cap = n */
	/* }; */
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
					token_id_t* const id = arena_alloc(id_arena, sizeof *id);
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
					new_token(arena, len_arena, i, offset);
					/* const token_id_t id = id_token(src + i, offset); */
					/* arena_push(id_arena, &id, sizeof id); */
					token_id_t* const id = arena_alloc(id_arena, 1);
					*id = id_token(src + i, offset);
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
				token_id_t* const id = arena_alloc(id_arena, 2);
				id[0] = id_token(src + pos, i - pos);
				id[1] = id_token(src + i, offset);
				new_token(arena, len_arena, pos, i - pos);
				new_token(arena, len_arena, i, offset);
				/* push_expected_tokens(expected_stack, id[0]); */
				/* push_expected_tokens(expected_stack, id[1]); */
				state = c == '\n' ? LEADING_WHITESPACE : WAITING_CHAR;
				i += offset - 1;
				num_tokens += 2;
			}
			else if (c == ' ' || c == '\t')
			{
				/* const token_id_t id = arena_alloc(id_arena, sizeof *id); */
				/* *id = id_token(src + pos, i - pos); */
				/* const token_id_t id = id_token(src + pos, i - pos); */
				/* arena_push(id_arena, &id, sizeof id); */
				token_id_t* const id = arena_alloc(id_arena, 1);
				*id = id_token(src + pos, i - pos);
				/* push_expected_tokens(expected_stack, id); */
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
				/* token_id_t *id = arena_alloc(id_arena, sizeof *id); */
				/* *id = id_token(src + i, offset); */
				/* const token_id_t id = id_token(src + i, offset); */
				/* arena_push(id_arena, &id, sizeof id); */
				token_id_t* const id = arena_alloc(id_arena, 1);
				*id = id_token(src + i, offset);
				/* push_expected_tokens(expected_stack, id); */
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
				/* token_id_t *id = arena_alloc(id_arena, sizeof *id); */
				/* *id = GDSCRIPT_WHITESPACE; */
				token_id_t* const id = arena_alloc(id_arena, 1);
				*id = GDSCRIPT_WHITESPACE;
				new_token(arena, len_arena,  i, 1);
				num_tokens++;
			}
			break;
		case COMMENT:
			if(c == '\n')
			{
				token_id_t *id = arena_alloc(id_arena, 2 * sizeof *id);
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
				/* token_id_t *id = arena_alloc(id_arena, sizeof *id); */
				token_id_t* const id = arena_alloc(id_arena, 1);
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
		/* token_id_t *id = arena_alloc(id_arena, sizeof *id); */
		token_id_t* const id = arena_alloc(id_arena, 1);
		*id = GDSCRIPT_STRING;
		new_token(arena, len_arena, pos, n - pos);
		++num_tokens;
	}
	// TODO: dump missing
	// DONE: check missing arena back in
	/* scratch_checkin(expected_arena); */
	{
		int32_t* eof_offset = arena_alloc(arena, sizeof *eof_offset);
		*eof_offset = n;
		uint16_t* eof_len = arena_alloc(len_arena, sizeof *eof_len);
		*eof_len = 0;
		token_id_t* const eof_id = arena_alloc(id_arena, sizeof *eof_id);
		*eof_id = GDSCRIPT_EOF;
		++num_tokens;
	}
	// the long requested "touch" of processing
	/* int16_t* offsets = arena->allocation; */
	/* int16_t* lens = len_arena->allocation; */
	/* token_id_t* ids = id_arena->allocation; */
	preprocess_tokens(src, arena, len_arena, id_arena, &num_tokens);
	// and FINALLY, replace semicolons with newlines
	/* uint8_t* temp = realloc(arena, num_tokens * (sizeof *tokens + sizeof *ids)); */
	const size_t req_size = num_tokens * (sizeof *table->offsets + sizeof *table->ids + sizeof *table->lens);
	if (arena->cap < req_size)
	{
		arena_grow(arena, req_size);
	}
	table->src = src;
	table->offsets = arena->addr;
	table->ids = memcpy(table->offsets + num_tokens, id_arena->addr, num_tokens * sizeof *table->ids);
	table->lens = memcpy(table->ids + num_tokens, len_arena->addr, num_tokens * sizeof *table->lens);
	table->num_tokens = num_tokens;
	return num_tokens;
}

static void lws_to_scopes(arena_t* offset_arena, arena_t* len_arena, arena_t* id_arena, int32_t* num_tokens)
{
	#define OFFSETS ((int32_t*)offset_arena->addr)
	#define LENS ((int16_t*)len_arena->addr)
	#define IDS ((token_id_t*)id_arena->addr)
	int32_t lws0 = 0;
	int32_t n = *num_tokens;
	arena_t* depth_arena = scratch_arena(128 * sizeof lws0); // if this isn't enough the user deserves to segfault
	const token_id_t scope_start = GDSCRIPT_SCOPE_OPEN;
	const token_id_t scope_end = GDSCRIPT_SCOPE_CLOSE;
	int32_t* d;
	const int16_t zero_len = 0;
	for (int32_t i = 0; i < n; ++i)
	{
		const int32_t i0 = i;
		const int32_t offset = OFFSETS[i0];
		for (; (IDS)[i] == GDSCRIPT_LEADING_WHITESPACE && i < n; ++i); // find the first non lws token
		int32_t lws1 = i - i0;
		/* printf("i0: %d, i: %d, lws0: %d, lws1: %d,\n", i0, i, lws0, lws1); */
		if (lws1 > lws0)
		{
			/* printf("INSERT SCOPE START!\n"); */
			// insert a scope start at i0
			arena_insert(offset_arena, i0 * 4, &offset, 4);
			arena_insert(len_arena, i0 * 2, &zero_len, 2);
			arena_insert(id_arena, i0 , &scope_start, 1);
			lws0 = lws1;
			++n;
			assert(depth_arena->size < 128 * sizeof lws0 && "REALLY, YOU HAVE MORE THAN 128 DEPTHS?  YOU DESERVE THIS ASSERT.");
			arena_push(depth_arena, &lws0, sizeof lws0);
		}
		else if (lws1 < lws0)
		{
			/* printf("INSERT SCOPE END!\n"); */
			lws0 = lws1;
			while(depth_arena->size > 0 && *(d = arena_peek(depth_arena, 4)) > lws0)
			{
				depth_arena->size -= 4;
				arena_insert(offset_arena, i0 * 4, &offset, 4);
				arena_insert(len_arena, i0 * 2, &zero_len, 2);
				arena_insert(id_arena, i0, &scope_end, 1);
				++n;
			}
		}
		for (; (IDS)[i] != GDSCRIPT_LINE_END && i < n; ++i); // find the first non lws token
	}
	const int32_t offset = (OFFSETS)[n - 1];
	while (depth_arena->size > 0)
	{
		depth_arena->size -= 4;
		arena_insert(offset_arena, (n - 1) * 4, &offset, 4);
		arena_insert(len_arena, (n - 1) * 2, &zero_len, 2);
		arena_insert(id_arena, (n - 1), &scope_end, 1);
		++n;
	}
	*num_tokens = n;
	#undef OFFSETS
	#undef LENS
	#undef IDS
}



static void classify_gdscript_tokens(const char* src, token_id_t* ids, const int32_t* offsets, const int16_t* lens, const uint32_t n)
{
	token_id_t prev_id = GDSCRIPT_NONE;
	for (int i = 0; i < n; ++i)
	{
		token_id_t id = ids[i];
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

static void coalesce_numbers(const char* const src, int32_t* offsets, int16_t* lens, token_id_t* ids, int32_t* num_tokens)
{
	int32_t n = *num_tokens;
	for (int32_t i = 0; i < n; ++i)
	{
		token_id_t id = ids[i];
		if (id == GDSCRIPT_NUMBER_CANDIDATE)
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
				if (index == i)
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

static void trim_token(int32_t* offsets, int16_t* lens, token_id_t* ids, int32_t* num_tokens, const token_id_t trim_token)
{
	int32_t swap = 0;
	const int32_t n = *num_tokens;
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

static void id_unary_operators(token_id_t* ids, const int32_t num_tokens)
{
	token_id_t prev_id = GDSCRIPT_NONE;
	const int unary_delta = GDSCRIPT_OP_IDENTITY - GDSCRIPT_OP_SUM; // NOTE: THIS ONLY WORKS BECAUSE THE ORDERING ALLOWS IT
	for (int32_t i = 0; i < num_tokens - 1; ++i) // last token is EOF
	{
		const token_id_t id = ids[i];
		if (id < GDSCRIPT_OP_SUM || GDSCRIPT_OP_DIFFERENCE < id)
		{
			prev_id = id;
		}
		else
		{
			for (int8_t j = 0; j < num_unary_preceders; ++j)
			{
				if (prev_id == unary_preceders[j])
				{
					ids[i] += unary_delta;
					prev_id = id + unary_delta;
					break;
				}
			}
		}
	}
}

/* NOTE: do this during syntax analysis */
/* static void one_assignment_per_line(int32_t* const offsets, int16_t* const lens, token_id_t* const ids, int32_t* const num_tokens) */
/* { */
/* 	int32_t swap = 0; */
/* 	const int32_t n = *num_tokens; */
/* 	int8_t has_assignment = 0x00; */
/* 	for (int32_t i = 0; i < n; ++i) */
/* 	{ */
/* 		const token_id_t id = ids[i]; */
/* 		switch(id) */
/* 		{ */
/* 		case GDSCRIPT_OP_ASSIGNMENT_POWER: __attribute__((fallthrough)); */
/* 		case GDSCRIPT_OP_ASSIGNMENT_PRODUCT: __attribute__((fallthrough)); */
/* 		case GDSCRIPT_OP_ASSIGNMENT_SUM: __attribute__((fallthrough)); */
/* 		case GDSCRIPT_OP_ASSIGNMENT_DIFFERENCE: __attribute__((fallthrough)); */
/* 		case GDSCRIPT_OP_ASSIGNMENT_DIVISION: __attribute__((fallthrough)); */
/* 		case GDSCRIPT_OP_ASSIGNMENT_MODULO: __attribute__((fallthrough)); */
/* 		case GDSCRIPT_OP_ASSIGNMENT_BITWISE_AND: __attribute__((fallthrough)); */
/* 		case GDSCRIPT_OP_ASSIGNMENT_BITWISE_OR: __attribute__((fallthrough)); */
/* 		case GDSCRIPT_OP_ASSIGNMENT_BITSHIFT_LEFT: __attribute__((fallthrough)); */
/* 		case GDSCRIPT_OP_ASSIGNMENT_BITSHIFT_RIGHT: __attribute__((fallthrough)); */
/* 		case GDSCRIPT_OP_ASSIGNMENT_XOR: __attribute__((fallthrough)); */
/* 		case GDSCRIPT_OP_ASSIGNMENT_INFERRED: __attribute__((fallthrough)); */
/* 		case GDSCRIPT_OP_ASSIGNMENT: */
/* 			if (!has_assignment) */
/* 			{ */
/* 				has_assignment = 0x01; */
/* 			} */
/* 			else */
/* 			{ */
/* 				int32_t j = i + 1; */
/* 				for (; j < n - 1; ++j) */
/* 				{ */
/* 					if (ids[j] == GDSCRIPT_SEMICOLON || ids[j] == GDSCRIPT_LINE_END) */
/* 					{ */
/* 						break; */
/* 					} */
/* 				} */
/* 				ids[swap] = GDSCRIPT_MULTIPLE_ASSIGNMENTS; */
/* 				lens[swap] = offsets[j - 1] - offsets[i] + lens[j - 1]; */
/* 				++swap; */
/* 				i = j; */
/* 			} */
/* 			break; */
/* 		case GDSCRIPT_SEMICOLON: __attribute__((fallthrough)); */
/* 		case GDSCRIPT_LINE_END: */
/* 			has_assignment = 0x00; */
/* 		} */
/* 		offsets[swap] = offsets[i]; */
/* 		lens[swap] = lens[i]; */
/* 		ids[swap] = ids[i]; */
/* 		++swap; */
/* 	} */
/* 	*num_tokens = swap; */
/* } */

STACK(id, token_id_t)
static void lint_pairs(arena_t* offset_arena, arena_t* len_arena, arena_t* id_arena, int32_t* num_tokens)
{
	#define OFFSETS ((int32_t*)offset_arena->addr)
	#define LENS ((int16_t*)len_arena->addr)
	#define IDS ((token_id_t*)id_arena->addr)
	struct id_stack* stack = alloca(sizeof *stack);
	*stack = (struct id_stack)
	{
		.buffer = scratch_alloc(4096),
		.cap = 1024
	};
	int32_t n = *num_tokens;
	const int16_t zero_len = 0;
	const int32_t extra_delta = GDSCRIPT_EXTRA_PAREN_CLOSE - GDSCRIPT_PAREN_CLOSE;
	const int32_t missing_delta = GDSCRIPT_MISSING_PAREN_CLOSE - GDSCRIPT_PAREN_CLOSE;
	for (int32_t i = 0; i < n; ++i)
	{
		const token_id_t id = IDS[i];
		switch(id)
		{
		case GDSCRIPT_PAREN_OPEN:
			id_stack_push(stack, GDSCRIPT_PAREN_CLOSE);
			break;
		case GDSCRIPT_BRACE_OPEN:
			id_stack_push(stack, GDSCRIPT_BRACE_CLOSE);
			break;
		case GDSCRIPT_BRACKET_OPEN:
			id_stack_push(stack, GDSCRIPT_BRACKET_CLOSE);
			break;
		/* case GDSCRIPT_PAREN_CLOSE ... GDSCRIPT_BRACKET_CLOSE: */
		/* 	if (id_stack_peek(stack) == id) */
		/* 	{ */
		/* 		id_stack_pop(stack); */
		/* 	} */
		/* 	else */
		/* 	{ */
		/* 		/\* const int32_t index = id_stack_has(stack, id); *\/ */
		/* 		#warning THIS CODE NO LONGER WORKS! */
		/* 		const int32_t index = 0; */
		/* 		const int32_t offset = OFFSETS[i]; */
		/* 		if (index > -1) */
		/* 		{ */
		/* 			const token_id_t* stack_ids = stack->buffer; */
		/* 			for (int32_t j = stack->size - 1; j > index; --j) */
		/* 			{ */
		/* 				const token_id_t missing_id = stack_ids[j] + missing_delta; */
		/* 				arena_insert(offset_arena, i * 4, &offset, 4); */
		/* 				arena_insert(len_arena, i * 2, &zero_len, 2); */
		/* 				arena_insert(id_arena, i, &missing_id, 1); */
		/* 				++n; */
		/* 			} */
		/* 			stack->size = index; */
		/* 		} */
		/* 		else */
		/* 		{ */
		/* 			const token_id_t extra_id = id + extra_delta; */
		/* 			arena_insert(offset_arena, i * 4, &offset, 4); */
		/* 			arena_insert(len_arena, i * 2, &zero_len, 2); */
		/* 			arena_insert(id_arena, i, &extra_id, 1); */
		/* 			++n; */
		/* 		} */
		/* 	} */
		/* 	break; */
		case GDSCRIPT_EOF: __attribute__((fallthrough));
		case GDSCRIPT_SEMICOLON: __attribute__((fallthrough));
		case GDSCRIPT_LINE_END:
			// pop all as missing
			{
				const int32_t offset = OFFSETS[i];
				const token_id_t* stack_ids = stack->buffer;
				for (int32_t j = stack->size - 1; j >= 0; --j)
				{
					const token_id_t missing_id = stack_ids[j] + missing_delta;
					arena_insert(offset_arena, i * 4, &offset, 4);
					arena_insert(offset_arena, i * 2, &zero_len, 2);
					arena_insert(offset_arena, i, &missing_id, 1);
				}
			}
			break;
		}
	}
	*num_tokens = n;
	#undef OFFSETS
	#undef LENS
	#undef IDS
}

// NOTE: HAS to be after lint_pairs
static void lint_commas(token_id_t* const ids, const int32_t num_tokens)
{
	token_id_t prev_id = GDSCRIPT_NONE;
	const int16_t comma_delta = GDSCRIPT_EXTRA_COMMA - GDSCRIPT_COMMA;
	int16_t depth = 0;
	for (int32_t i = 0; i < num_tokens; ++i)
	{
		const token_id_t id = ids[i];
		ids[i] += (id == GDSCRIPT_COMMA) * (depth == 0 || prev_id == GDSCRIPT_COMMA) * comma_delta;
		switch(id)
		{
		case GDSCRIPT_PAREN_OPEN ... GDSCRIPT_BRACKET_OPEN:
			++depth;
			break;
		/* case GDSCRIPT_PAREN_CLOSE ... GDSCRIPT_BRACKET_CLOSE: */
		/* 	--depth; */
		/* 	break; */
		case GDSCRIPT_MISSING_PAREN_CLOSE ... GDSCRIPT_MISSING_BRACKET_CLOSE:
			--depth;
			break;
		}
		prev_id = id;
	}
}


static void preprocess_tokens(const char* src, arena_t* offset_arena, arena_t* len_arena, arena_t* id_arena, int32_t* num_tokens)
{
	/* classify_gdscript_tokens(src, ids, offsets, lens, *num_tokens - 1); // don't gotta classify the last token -- it's EOF */
	int32_t* const offsets = offset_arena->addr;
	int16_t* const lens = len_arena->addr;
	token_id_t* ids = id_arena->addr;
	coalesce_numbers(src, offsets, lens, ids, num_tokens);
	trim_token(offsets, lens, ids, num_tokens, GDSCRIPT_WHITESPACE);
	trim_token(offsets, lens, ids, num_tokens, GDSCRIPT_COMMENT);
	cull_line_continuations(offsets, lens, ids, num_tokens);
	cull_pointless_lines(offsets, lens, ids, num_tokens);
	/* one_assignment_per_line(offsets, lens, ids, num_tokens); */
	lws_to_scopes(offset_arena, len_arena, id_arena, num_tokens);
	trim_token(offset_arena->addr, len_arena->addr, id_arena->addr, num_tokens, GDSCRIPT_LEADING_WHITESPACE);
	lint_pairs(offset_arena, len_arena, id_arena, num_tokens);
	ids = id_arena->addr;
	const int32_t n = *num_tokens;
	lint_commas(ids, n);
	id_unary_operators(ids, n);
	// convert unclassified to identifiers
	/* for (int i = 0; i < n; ++i) */
	/* { */
	/* 	const token_id_t id = ids[i]; */
	/* 	ids[i] += (id == GDSCRIPT_NONE) * GDSCRIPT_IDENTIFIER; */
	/* } */
	// convert semicolons to newlines
	// CHECK THE ENUMS--THIS IS BASED
	for (int32_t i = 0; i < n; ++i)
	{
		ids[i] += ids[i] == GDSCRIPT_SEMICOLON;
	}
}

