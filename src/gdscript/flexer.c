#include "flexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "token.h"
#include "gdscript_ids.h"
#include "../allocators/arena.h"
#include "../allocators/scratch.h"
#include "../macro_stack.h"
/* STACK(i32, int32_t) */
STACK(i16, int32_t)
STACK(id, gdscript_id_t)

#define true 1
#define false 0
typedef int8_t bool_t;


/* typedef enum __attribute__((packed)) lex_state_t */
/* { */
/* 	LEX_NONE = 0x00, */
/* 	LEX_LWS, */
/* 	LEX_FUNC_SIG, */
/* 	LEX_PARAMS, */
/* 	LEX_ARGS, */
/* 	LEX_VARIABLE, */
/* 	LEX_CONTROL, */
/* 	LEX_BODY, */
/* 	LEX_GROUP, */
/* 	LEX_DICT, */
/* 	LEX_ARRAY, */
/* 	LEX_SUBCLASS, */
/* 	LEX_EXPRESSION */
/* } lex_state_t; */
/* STACK(state, lex_state_t) */


static struct {
	char* delim_chars;
	char** tokens;
	int16_t* lens;
	gdscript_id_t* ids;
	int8_t num_delim_chars;
	int8_t num_builtin;
} flex_table = {0};

/* #define BESPOKE_DELIM_CHARS \ */
/* 	X('@') */

__attribute__((constructor))
static void init_flexer(void)
{
	int32_t i = 0;
#define X(_, __, LEN) i += LEN;
	GDSCRIPT_BUILTIN_TOKENS
#undef X
	const int32_t i0 = i;
#define X(_, __, LEN) i += LEN == 1;
	GDSCRIPT_STRUCTURE
	GDSCRIPT_OPERATORS
#undef X
	i += 2; // two more for space & tab
	const int32_t size0 = sizeof(char*) + sizeof(int16_t) + sizeof(gdscript_id_t);
	uint8_t* allocation = malloc(GDSCRIPT_NUM_BUILTIN * size0 + i);
	flex_table.tokens = allocation;
	flex_table.lens = flex_table.tokens + GDSCRIPT_NUM_BUILTIN;
	flex_table.ids = flex_table.lens + GDSCRIPT_NUM_BUILTIN;
	arena_t token_arena =
	{
		.allocation = flex_table.ids + GDSCRIPT_NUM_BUILTIN,
		.cap = i0
	};
	flex_table.delim_chars = token_arena.allocation + i0;
	flex_table.num_delim_chars = i - i0;
	char* str;
	i = 0;
	char* foo = malloc(4);
	foo = "foo";
#define X(ID, STR, LEN) \
	flex_table.tokens[i] = token_arena.allocation + token_arena.size; \
	arena_push(&token_arema, STR, LEN); /* does thiss work tho? */ \
	flex_table.tokens[i] = token_arena.allocation + token_arena.size; \
	flex_table.lens[i] = LEN; \
	flex_table.ids[i] = GDSCRIPT_##ID; \
	++i;
#undef X
	flex_table.delim_chars[0] = ' ';
	flex_table.delim_chars[1] = '\t';
	i = 1;
#define X(_, STR, LEN) if (LEN == 1) { flex_table.delim_chars[i++] = STR[0]; }
	GDSCRIPT_STRUCTURE
	GDSCRIPT_OPERATORS
#undef X
		/* printf("DELIMITERS:\n"); */
		/* for (int8_t i = 0; i < flex_table.num_delim_chars; ++i) */
		/* { */
		/* 	printf("> %c\n", flex_table.delim_chars[i]); */
		/* } */
}

static inline void new_token(arena_t* offset_arena, arena_t* len_arena, const int32_t pos, const int16_t len)
{
	/* arena_dynamic_push(offset_arena, &pos, sizeof pos); */
	/* arena_dynamic_push(len_arena, &len, sizeof len); */
	arena_push(offset_arena, &pos, 4);
	arena_push(len_arena, &len, 2);
}

static const gdscript_id_t missing(const gdscript_id_t id)
{
	#define MISSING \
		X(PAREN_CLOSE) \
		X(BRACE_CLOSE) \
		X(BRACKET_CLOSE) \
		X(PAREN_OPEN) \
		X(COLON) \
		X(IDENTIFIER)
	switch(id)
	{
#define X(ID) case GDSCRIPT_##ID: return GDSCRIPT_MISSING_##ID;
	MISSING
#undef X
	default: return id;
	}
#undef MISSING
}


static const gdscript_id_t tokenize(const char* src, const int32_t i0, const int32_t n, int16_t* const out_len)
{
	// TODO: try matching operators directly first

	/* printf("i0: %d, n: %d\n", i0, n); */
	const char* delims = flex_table.delim_chars;
	const int32_t num_delims = flex_table.num_delim_chars;
	int32_t i = i0 + 1;
	bool_t break_flag = false;
	for (; i < n; ++i)
	{
		const char c = src[i];
		/* printf("%c", c); */
		for (int32_t j = 0; j < num_delims; ++j)
		{
			if (c == delims[j])
			{
				/* printf(" - %d: %c -\n", i, c); */
				/* printf("len: %d\n", i - i0); */
				break_flag = true;
				break;
			}
		}
		if (break_flag)
		{
			break;
		}
	}
	const char* token = src + i0;
	const int16_t len = i - i0;
	*out_len = len;
	/* printf("token len: %d\n", len); */
	/* printf("token len: %d\n"); */
	char** tokens = flex_table.tokens;
	const int16_t* lens = flex_table.lens;
	for (int32_t j = 0; j < GDSCRIPT_NUM_BUILTIN; ++j)
	{
		if (lens[j] == len && strncmp(tokens[j], token, len) == 0)
		{
			return flex_table.ids[j];
		}
	}
	return GDSCRIPT_IDENTIFIER;
}



static int32_t delimiter(const char* src, const int32_t i0, const int32_t n)
{
	const char* delims = flex_table.delim_chars;
	const int32_t num_delims = flex_table.num_delim_chars;
	for (int32_t i = i0; i < n; ++i)
	{
		const char c = src[i];
		for (int8_t j = 0; j < num_delims; ++j)
		{
			if (c == delims[j])
			{
				return i;
			}
		}
	}
	return n;
}

static int32_t num_delimiter(const char* src, const int32_t i0, const int32_t n)
{
	for (int32_t i = i0; i < n; ++i)
	{
		const char c = src[i];
		const bool_t num_flag = '0' <= c && c <= '9';
		const bool_t hex_flag = ('a' <= c && c <= 'f') || ('A' <= c && c <= 'F');
		const bool_t etc_flag = c == '.' || c == '_' || c == '-' || c == 'x';
		if (num_flag && hex_flag && etc_flag)
		{
			continue;
		}
		else
		{
			return i;
		}
	}
	return n;
}

static int32_t str32_delimiter(const char* src, const int32_t i0, const int32_t n)
{
	char c0 = i0 > 0 ? src[i0 - 1] : 0;
	for (int32_t i = i0; i < n - 3; ++ i)
	{
		const char c = src[i];
		if (c0 != '\\' && c == '"' && src[i + 1] == '"' && src[i + 2] == '"')
		{
			return i;
		}
		c0 = c;
	}
	return n;
}

static int32_t str31_delimiter(const char* src, const int32_t i0, const int32_t n)
{
	for (int32_t i = i0; i < n - 3; ++ i)
	{
		if (src[i] == '\'' && src[i + 1] == '\'' && src[i + 2] == '\'')
		{
			return i;
		}
	}
	return n;
}

static int32_t str12_delimiter(const char* src, const int32_t i0, const int32_t n)
{
	char c0 = i0 > 0 ? src[i0 - 1] : 0;
	for (int32_t i = i0; i < n; ++ i)
	{
		const char c = src[i];
		if (c0 != '\\' && c == '"')
		{
			return i;
		}
		c0 = c;
	}
	return n;
}

static int32_t str11_delimiter(const char* src, const int32_t i0, const int32_t n)
{
	for (int32_t i = i0; i < n; ++ i)
	{
		if (src[i] == '\'')
		{
			return i;
		}
	}
	return n;
}

/* static const int16_t token_len(const char* src, const gdscript_id_t id, const int32_t offset, const int32_t n) */
/* { */
/* 	#define LEN_MAP \ */
/* 		X(IDENTIFIER, delimiter) \ */
/* 		X(NUMBER, num_delimiter)				\ */
/* 		X(TRIPLE_DOUBLE_QUOTE_STRING, str32_delimiter) \ */
/* 		X(TRIPLE_QUOTE_STRING, str31_delimiter) \ */
/* 		X(DOUBLE_QUOTE_STRING, str12_delimiter) \ */
/* 		X(QUOTE_STRING, str11_delimiter) */
/* 	switch (id) */
/* 	{ */
/* 	#define X(ID, __, LEN) case GDSCRIPT_##ID: return LEN; */
/* 		GDSCRIPT_BUILTIN_TOKENS */
/* 	#undef X */
/* 	#define X(ID, FUNC) case GDSCRIPT_##ID: return FUNC(src, offset, n); */
/* 			LEN_MAP */
/* 	#undef X */
/* 	case GDSCRIPT_MIXED_LWS: */
/* 		return n; */
/* 	default: */
/* 		return 0; */
/* 	} */
/* 	#undef LEN_MAP */
/* } */


static gdscript_id_t verify_identifier(const char* src, const int32_t offset, const int32_t len)
{
	const char* str = src + offset;
	for (int32_t i = 0; i < len; ++i)
	{
		const char c = str[i];
		const bool_t lowercase = 'a' <= c && 'z' <= c;
		const bool_t uppercase = 'A' <= c && 'Z' <= c;
		const bool_t digits = '0' <= c && '9' <= c;
		if (lowercase || uppercase || digits || c == '_')
		{
			continue;
		}
		else
		{
			return GDSCRIPT_PERVERTED_IDENTIFIER;
		}
	}
	return GDSCRIPT_IDENTIFIER;
}

/* THE ACTUAL NUMBER RULES
 * floats may start with .
 * must use lower case e for scientific notation
 * _ cannot follow itself nor the e for scientific notation
 * the exponent of scientific numbers must be an integer
 * binary & hexadecimal numbers must start with 0
 * must use lower case b to denote binary
 * must use lower case x to denote hexadecimals
 * can use either lower or upper case a through f for actual hexadigits */
static gdscript_id_t verify_number(const char* src, const int32_t offset, const int16_t len)
{
	const char* str = src + offset;
	char prev_c = str[0];
	enum __attribute__((packed))
	{
		BIN_FLAG = 0x01,
		HEX_FLAG = 0x02,
		FLOAT_FLAG = 0x04,
		SCI_FLAG = 0x08,
		/* NEG_FLAG = 0x10, */
	};
	int8_t flags = (prev_c == '.') * FLOAT_FLAG;
	// NOTE: first char is guaranteed to appropriate for a number
	for (int32_t i = 1; i < len; ++i)
	{
		const char c  = str[i];
		switch (c)
		{
		case '0'...'1': continue;
		case '2'...'9':
			if (flags & BIN_FLAG)
			{
				return GDSCRIPT_PERVERTED_NUMBER;
			}
			break;
		case 'b':
			if (flags & HEX_FLAG)
			{
				continue;
			}
			else if (i == 1 && prev_c == '0')
			{
				flags |= BIN_FLAG;
			}
			else
			{
				return GDSCRIPT_PERVERTED_NUMBER;
			}
			break;
		case 'e':
			if (flags & HEX_FLAG)
			{
				continue;
			}
			else if (flags & SCI_FLAG)
			{
				return GDSCRIPT_PERVERTED_NUMBER;
			}
			else
			{
				flags |= SCI_FLAG;
			}
			break;
		case 'a': __attribute__((fallthrough));
		case 'f': __attribute__((fallthrough));
		case 'c'...'d': __attribute__((fallthrough));
		case 'A'...'F':
			if (flags ^ HEX_FLAG)
			{
				return GDSCRIPT_PERVERTED_NUMBER;
			}
			break;
		case 'x':
			if (i == 1 && prev_c == '0')
			{
				flags |= HEX_FLAG;
			}
			else
			{
				return GDSCRIPT_PERVERTED_NUMBER;
			}
			break;
		case '_':
			if (prev_c == '_' || flags & SCI_FLAG && prev_c == 'e')
			{
				return GDSCRIPT_PERVERTED_NUMBER;
			}
			break;
		case '-':
			if (flags & SCI_FLAG && str[i - 1] == 'e') // TODO: check capitalization for sci-notation
			{
				continue;
			}
			else
			{
				return GDSCRIPT_PERVERTED_NUMBER;
			}
		case '.':
			if (flags > 0x00)
			{
				return GDSCRIPT_PERVERTED_NUMBER;
			}
			else
			{
				flags |= FLOAT_FLAG;
			}
		default:
			return GDSCRIPT_PERVERTED_NUMBER;
		}
		prev_c = c;
	}
	return GDSCRIPT_NUMBER;
}

/* const inline bool_t unary_trigger(const gdscript_id_t id) */
/* { */
/* 	return (GDSCRIPT_OP_ASSIGNMENT_POWER <= id && id <= GDSCRIPT_OP_BOOL_NOT) || */
/* 		GDSCRIPT_PAREN_OPEN == id || id == GDSCRIPT_BRACE_OPEN || id == GDSCRIPT_BRACKET_OPEN || */
/* 		id == GDSCRIPT_LINE_END || id == GDSCRIPT_SEMICOLON || GDSCRIPT_SCOPE_START || */
/* 		id == GDSCRIPT_NONE || */
/* 		id == GDSCRIPT_NOT || id == GDSCRIPT_OR || id == GDSCRIPT_AND; */
/* } */

typedef struct context_t
{
	int32_t* offsets;
	int16_t* lens;
	gdscript_id_t* ids;
	int32_t num;
	gdscript_id_t id;
	const uint8_t prev_mask;
} context_t;

enum __attribute__((packed))
{
	CONST_OR_VAR_2PREV = 0x01,
	COMMA_PREV		  = 0x02,
	COLON_PREV		  = 0x04,
};

void fuck_it_we_ball(context_t* const ctx, id_stack* const expected)
{
	int32_t num = ctx->num;
	int32_t* const offsets = ctx->offsets;
	int16_t* const lens = ctx->lens;
	gdscript_id_t* const ids = ctx->ids;
	const gdscript_id_t peek = expected->size > 0 ? id_stack_peek(expected) : GDSCRIPT_NONE;
	const bool_t promote_colon =
		peek == GDSCRIPT_PAREN_CLOSE ||
		// NOTE: dict colons stay as colons
		/* peek == GDSCRIPT_BRACE_CLOSE || */
		peek == GDSCRIPT_BRACKET_CLOSE ||
		(ctx->prev_mask & CONST_OR_VAR_2PREV);
	gdscript_id_t id0 = ctx->id == GDSCRIPT_COLON && promote_colon ? GDSCRIPT_OP_TYPE : ctx->id;
	// unarize as needed
	if (peek == id0)
	{
		--expected->size;
		return;
	}
	else if (peek == GDSCRIPT_IDENTIFIER)
	{
		offsets[num] = offsets[num - 1];
		lens[num] = lens[num - 1];
		ids[num] = GDSCRIPT_MISSING_IDENTIFIER;
		++num;
	}
	else if (ctx->prev_mask & COLON_PREV)
	{
		for (int32_t i = num - 1; i >= 0; --i)
		{
			const gdscript_id_t id = ids[i];
			if (id == GDSCRIPT_LINE_END)
			{
				break;
			}
			else if (id == GDSCRIPT_FUNC || id == GDSCRIPT_IF || id == GDSCRIPT_ELIF || id == GDSCRIPT_ELSE)
			{
				offsets[num] = offsets[num - 1];
				offsets[num + 1] = offsets[num - 1];
				lens[num] = 0;
				lens[num + 1] = 0;
				ids[num] =  GDSCRIPT_LINE_END;
				ids[num + 1] =  GDSCRIPT_SCOPE_START;
				num += 2;
				id_stack_push(expected, GDSCRIPT_SCOPE_END); // NOTE: scope end goes AFTER endl
				break;
			}
		}
	}
	#define PAIRS \
		X(PAREN) \
		X(BRACE) \
		X(BRACKET)
	switch (id0)
	{
	case GDSCRIPT_COMMA:
		if (ctx->prev_mask & COMMA_PREV || peek == GDSCRIPT_COLON || peek == GDSCRIPT_IDENTIFIER || peek == GDSCRIPT_NONE)
		{
			id0 = GDSCRIPT_EXTRA_COMMA;
			goto set_num;
		}
	case GDSCRIPT_CONST: __attribute__((fallthrough));
	case GDSCRIPT_VAR:
		id_stack_push(expected, GDSCRIPT_IDENTIFIER);
		break;
	case GDSCRIPT_FUNC:
		id_stack_push(expected, GDSCRIPT_COLON);
		id_stack_push(expected, GDSCRIPT_PAREN_CLOSE);
		id_stack_push(expected, GDSCRIPT_PAREN_OPEN);
		id_stack_push(expected, GDSCRIPT_IDENTIFIER);
		break;
#define X(ID) case GDSCRIPT_##ID##_OPEN: id_stack_push(expected, GDSCRIPT_##ID##_CLOSE); break;
		PAIRS
#undef X
// NOTE: you know FOR A FACT that peek != id0, so skip control flow!
#define X(ID) \
case GDSCRIPT_##ID##_CLOSE:	  \
{ \
	int32_t i = expected->size - 2; \
	for (; i > 0; --i) \
	{ \
		const gdscript_id_t stack_id = expected->allocation[i]; \
		if(stack_id == GDSCRIPT_##ID##_CLOSE || stack_id == GDSCRIPT_SCOPE_END) \
		{ \
			break; \
		} \
	} \
	if (i > 0) \
	{ \
		const int32_t offset0 = offsets[num - 1]; \
		for (int32_t j = expected->size; j > i; --j) \
		{ \
			offsets[num] = offset0; \
			lens[num] = 0; \
			ids[num] = missing(id_stack_pop(expected));	\
			++num;										\
		} \
	} \
	else										\
	{ \
		id0 = GDSCRIPT_EXTRA_##ID##_CLOSE; \
	} \
} \
break;
		PAIRS
#undef X
#define X(ID,...) case GDSCRIPT_##ID: __attribute__((fallthrough));
		GDSCRIPT_ASSIGNMENT_OPERATORS
#undef X
	case GDSCRIPT_COLON:
		for (int32_t i = expected->size; i > 0; ++i)
		{
			if (id_stack_peek(expected) != GDSCRIPT_SCOPE_END)
			{
				offsets[num] = offsets[num - 1];
				lens[num] = lens[num - 1];
				ids[num] = missing(id_stack_pop(expected));
				++num;
			}
			else
			{
				break;
			}
		}
	}
	ctx->id = id0;
set_num:
	ctx->num = num;
}

// arena_t has atleast 8 * n + 4096 bytes. Extras for inserting scope tokens & missing closing pairs.
// NOTE: 2 hidden scratch checkouts
// NOTE: semicolon must be followed by an EXPRESSION.  A REAL expression!
// FUCK IT WE BALL
int32_t gdscript_flex(arena_t* arena, token_table_t* table, const char* src, const int32_t n)
{
	table->src = src;
	int32_t num = 0;
	int32_t* offsets = arena->allocation;
	int16_t* lens = offsets + n + 512;
	gdscript_id_t* ids = lens + n + 512;
	/* SCRATCH* shunt_scratch = scratch_checkout_size(4 * n); */
	/* i32_stack shunt_stack = */
	/* { */
	/* 	.allocation = shunt_scratch->allocation, */
	/* 	.cap = n */
	/* }; */
	SCRATCH* expected_arena = scratch_checkout_size(n);
	id_stack expected =
	{
		.allocation = expected_arena->allocation,
		.cap = n
	};
	SCRATCH* depth_arena = scratch_checkout_size(2 * n);
	i16_stack depths =
	{
		.allocation = depth_arena->allocation,
		.cap = n
	};
	i16_stack_push(&depths, 0);
	enum __attribute__((packed))
	{
		LWS_NONE     = 0x00,
		LWS_SPACES	= 0x01,
		LWS_TABS     = 0x02,
		LWS_BOTH     = 0x03,
		LWS_INACTIVE = 0x04,
		LWS_MASK     = 0x0f,
	};
	enum __attribute__((packed))
	{
		STR_QUOTE  = 0x10,
		STR_DOUBLE = 0x20,
		STR_RAW    = 0x40,
		STR_TRIPLE = 0x80,
		STR_MASK   = 0xf0,
	};
	uint8_t lex_mask = 0x00;
	int16_t lws = 0;
	const int16_t zero_len = 0;
	/* char prev_c = 0; */
	for (int32_t i = 0; i < n; ++i)
	{
		const char c = src[i];
		if (lex_mask & LWS_INACTIVE)
		{
			if (c == ' ' || c == '\t' || c == '\r')
			{
				continue;
			}
			else if (c == '\\')
			{
				const int32_t i0 = i + 1;
				for (i = i0; i < n && src[i] != '\n'; ++i);
				if (i > i0)
				{
					offsets[num] = i;
					lens[num] = i0 - i;
					ids[num] = GDSCRIPT_TOKENS_AFTER_CONTINUATION;
					++num;
				}
				continue;
			}
			else if (c == '\n')
			{
				for (int32_t j = expected.size; j > 0; --j)
				{
					if (id_stack_peek(&expected) != GDSCRIPT_SCOPE_END)
					{
						offsets[num] = i;
						lens[num] = lens[num - 1];
						ids[num] = missing(id_stack_pop(&expected));
						++num;
					}
					else
					{
						break;
					}
				}
				offsets[num] = i;
				lens[num] = 1;
				ids[num] = GDSCRIPT_LINE_END;
				++num;
				const gdscript_id_t peek = expected.size > 0? id_stack_peek(&expected) : GDSCRIPT_NONE;
				if (peek == GDSCRIPT_SCOPE_END)
				{
					offsets[num] = i;
					lens[num] = 0;
					ids[num] = peek;
					++num;
				}
				lex_mask &= ~LWS_INACTIVE;
				lws = 0;
			}
			else if (c == ';')
			{
				// TODO: semicolon
			}
			else
			{
				const char next_char = i < n - 1 ? src[i + 1]: 0;
				const bool_t num_flag = ('0' <= c && c <= '9') ||
					(c == '.' && '0' < next_char && next_char <= '9');
				lex_mask |= (c == '\'') << 4;
				lex_mask |= (c == '"') << 5;
				lex_mask |= (c == 'r' && (next_char == '\'' || next_char == '"')) << 6;
				if (num_flag)
				{
					const int32_t i0 = i;
					i = num_delimiter(src, i0, n);
					offsets[num] = i0;
					lens[num] = i - i0;
					ids[num] = GDSCRIPT_NUMBER;
				}
				else if (lex_mask & STR_MASK)
				{
					const bool_t raw_flag = (lex_mask & STR_RAW) >> 6;
					lex_mask |= raw_flag ? STR_QUOTE << next_char == '"' : 0x00;
					const int32_t i0 = i + raw_flag;
					lex_mask |= i0 + 3 < n ? STR_TRIPLE : 0x00;
					for (i = i0 + 1; i < n && i < i0 + 3; ++i)
					{
						lex_mask &= (lex_mask & STR_QUOTE ? src[i] == '\'' : src[i] == '"') << 7;
					}
					lex_mask &= ~STR_RAW;
					if ((lex_mask & STR_MASK) == STR_DOUBLE)
					{
						i = str12_delimiter(src, i0, n);
					}
					else if ((lex_mask & STR_MASK) == (STR_TRIPLE | STR_DOUBLE))
					{
						i = str32_delimiter(src, i0, n);
					}
					else if ((lex_mask & STR_MASK) == STR_QUOTE)
					{
						i = str11_delimiter(src, i0, n);
					}
					else if ((lex_mask & STR_MASK) == (STR_TRIPLE | STR_QUOTE))
					{
						i = str31_delimiter(src, i0, n);
					}
					offsets[num] = i0;
					lens[num] = i - i0;
					ids[num] = GDSCRIPT_STRING;
					++num;
					lex_mask &= ~STR_MASK;
					i += lex_mask & STR_TRIPLE ? 2 : 0;
				}
				else
				{
					int16_t len;
					const gdscript_id_t id = tokenize(src, i, n, &len); // NOTE: DO NOT ADD 1 TO i
					int16_t id_mask = id;
					if (id == GDSCRIPT_IDENTIFIER)
					{
						if (id_stack_peek(&expected) == GDSCRIPT_IDENTIFIER)
						{
							--expected.size;
						}
						offsets[num] = i;
						lens[num] = len;
						ids[num] = id;
						++num;
					}
					else
					{
						const gdscript_id_t id2 = num - 2 > 0 ? ids[num - 2] : GDSCRIPT_NONE;
						const gdscript_id_t id1 = num - 1 > 0 ? ids[num - 1] : GDSCRIPT_NONE;
						const uint8_t prev_mask =
							id2 == GDSCRIPT_VAR || id2 == GDSCRIPT_CONST ||
							(id1 == GDSCRIPT_COMMA) << 1 ||
							(id1 == GDSCRIPT_COLON || id1 == GDSCRIPT_MISSING_COLON) << 2;
						context_t ctx =
						{
							.offsets = offsets,
							.lens = lens,
							.ids = ids,
							.num = num,
							.id = id,
							.prev_mask = prev_mask
						};
						fuck_it_we_ball(&ctx, &expected);
						num = ctx.num;
						offsets[num] = i;
						lens[num] = len;
						ids[num] = ctx.id;
						++num;
					}
					i += len - 1;
				}
			}
		}
		// LWS handling
		else if (c == '\t' || c == ' ')
		{
			++lws;
			lex_mask |= c == '\t' ? LWS_TABS : LWS_SPACES;
			if ((lex_mask & LWS_MASK) == LWS_BOTH)
			{
				lens = offsets + 1;
				ids = lens + 1;
				table->offsets = offsets;
				table->lens = lens;
				table->ids = ids;
				*offsets = 0;
				*lens = n;
				*ids = GDSCRIPT_MIXED_LWS;
				return 1;
			}
		}
		else if (c == '\n')
		{
			lws = 0;
		}
		else if (c == '#')
		{
			for (; i < n && src[i] != '\n'; ++i);
			lws = 0;
		}
		// TODO: semicolon and \ handling
		else
		{
			// NOTE: scope insertion goes AFTER endl
			// TODO: depth updates
			lex_mask |= LWS_INACTIVE;
			--i;
		}
	}
	// insert closing tokens as needed.  ending scopes never err.
	const int32_t n_1 = n - 1;
	for (int32_t i = expected.size; i > 0; --i)
	{
		offsets[num] = n_1;
		lens[num] = 0;
		ids[num] = missing(id_stack_pop(&expected));
		++num;
	}
	for (int32_t i = depths.size - 1; i > 0; --i) // subtract 1 because the first depth is 0
	{
		offsets[num] = n_1;
		lens[num] = 0;
		ids[num] = GDSCRIPT_SCOPE_END;
		++num;
	}
	// use memmove since memory might overlap
	lens = memmove(offsets + num + 1, lens, 2 * num);
	ids = memmove(lens + num + 1, ids, num);
	// TODO: run this whilestve shunting
	/* for (int32_t i = 0; i < num; ++i) */
	/* { */
	/* 	/\* const int16_t offset = offsets[i]; *\/ */
	/* 	const gdscript_id_t id = ids[i]; */
	/* 	/\* const int16_t len = token_len(src, id, offset, n); *\/ */
	/* 	if (id == GDSCRIPT_IDENTIFIER) */
	/* 	{ */
	/* 		ids[i] = verify_identifier(src, offset, len); */
	/* 	} */
	/* 	else if (id == GDSCRIPT_NUMBER) */
	/* 	{ */
	/* 		ids[i] = verify_number(src, offset, len); */
	/* 	} */
	/* 	lens[i] = len; */
	/* } */
	// FINALLY, insert EOF
	offsets[num] = n;
	lens[num] = 0;
	ids[num] = GDSCRIPT_EOF;
	table->offsets = offsets;
	table->lens = lens;
	table->ids = ids;
	table->num_tokens = ++num;
	return num;
}

int32_t gdscript_shunt_etc(token_table_t* const dest, const token_table_t* src)
{
	// shunt
	// also check numbers & identifiers
	// and unary-ize appropriate operators
	// etc. errors
	return 0;
}
