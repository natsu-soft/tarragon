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
	char** graphs;
	char** keywords;
	int16_t* graph_lens;
	int16_t* keyword_lens;
	gdscript_id_t* graph_ids;
	gdscript_id_t* keyword_ids;
	int32_t num_graphs;
	int32_t num_keywords;
} flex_table = {0};

/* #define BESPOKE_DELIM_CHARS \ */
/* 	X('@') */

__attribute__((constructor))
static void init_flexer(void)
{
	int16_t i = 0;
	int16_t l = 0;
#define X(ID, STR, LEN) ++i; l += LEN;
	GDSCRIPT_STRUCTURE
	GDSCRIPT_OPERATORS
	int32_t num_graphs = i;
	i = 0;
	GDSCRIPT_KEYWORDS
	GDSCRIPT_LITERALS
	GDSCRIPT_ANNOTATIONS
	int32_t num_keywords = i;
#undef X
	const size_t sum_size = sizeof(char*) + sizeof(int16_t) + sizeof(gdscript_id_t);
	uint8_t* addr = malloc(sum_size * (num_graphs + num_keywords) + l + 1);
	assert(addr != NULL && "BRO HOW?");
	char** graphs = addr;
	char** keywords = graphs + num_graphs;
	int16_t* graph_lens = keywords + num_keywords;
	int16_t* keyword_lens = graph_lens + num_graphs;
	gdscript_id_t* graph_ids = keyword_lens + num_keywords;
	gdscript_id_t* keyword_ids = graph_ids + num_graphs;
	const char* char_alloc = keyword_ids + num_keywords;
	int16_t char_offset = 0;
	i = 0;
#define X(ID, STR, LEN) \
	graphs[i] = char_alloc + char_offset; \
	memcpy(char_alloc + char_offset, STR, LEN); \
	/* graphs[i] = STR; */ /* arena_push(&char_arena, STR, LEN); */	\
	char_offset += LEN; \
	graph_lens[i] = LEN; \
	graph_ids[i] = GDSCRIPT_##ID; \
	++i;
	GDSCRIPT_STRUCTURE
	GDSCRIPT_OPERATORS
#undef X
	i = 0;
#define X(ID, STR, LEN) \
	keywords[i] = char_alloc + char_offset; \
	memcpy(char_alloc + char_offset, STR, LEN); \
	/* keywords[i] = STR; *//* arena_push(&char_arena, STR, LEN); */	\
	char_offset += LEN; \
	keyword_lens[i] = LEN; \
	keyword_ids[i] = GDSCRIPT_##ID; \
	++i;
	GDSCRIPT_KEYWORDS
	GDSCRIPT_LITERALS
	GDSCRIPT_ANNOTATIONS
#undef X
	flex_table.graphs = graphs;
	flex_table.keywords = keywords;
	flex_table.graph_lens = graph_lens;
	flex_table.keyword_lens = keyword_lens;
	flex_table.graph_ids = graph_ids;
	flex_table.keyword_ids = keyword_ids;
	flex_table.num_graphs = num_graphs;
	flex_table.num_keywords = num_keywords;
	/* for (int32_t j = 0; j < num_graphs; ++j) */
	/* { */
	/* 	printf("\n> "); */
	/* 	const char* str = graphs[j]; */
	/* 	const int16_t len = graph_lens[j]; */
	/* 	for (int32_t k = 0; k < len; ++k) */
	/* 	{ */
	/* 		printf("%c", str[k]); */
	/* 	} */
	/* 	printf(", %d", len); */
	/* } */
	/* for (int32_t j = 0; j < num_keywords; ++j) */
	/* { */
	/* 	printf("\n> "); */
	/* 	const char* str = keywords[j]; */
	/* 	const int16_t len = keyword_lens[j]; */
	/* 	for (int32_t k = 0; k < len; ++k) */
	/* 	{ */
	/* 		printf("%c", str[k]); */
	/* 	} */
	/* 	printf(", %d", len); */
	/* } */
	/* fflush(stdout); */
}

static const gdscript_id_t missing(const gdscript_id_t id)
{
	#define MISSING \
		X(PAREN_CLOSE) \
		X(BRACE_CLOSE) \
		X(BRACKET_CLOSE)
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
	const char* token = src + i0;
	const char c = token[0];
	const int32_t max_len = n - i0;
	char** graphs = flex_table.graphs;
	const int16_t* len_graphs = flex_table.graph_lens;
	const int32_t num_graphs = flex_table.num_graphs;
	if (('&' <= c && c <= '/') || (':' <= c && c <= '?') || ('[' <= c && c <= '^') || ('{' <= c && c <= '~') || c == '!' || c == '"' || c == '#')
	{
		for (int32_t i = 0; i < num_graphs; ++i)
		{
			const int32_t l = len_graphs[i];
			if (max_len >= l && strncmp(graphs[i], token, l) == 0)
			{
				*out_len = l;
				return flex_table.graph_ids[i];
			}
		}
	}
	int32_t i = i0 + 1;
	bool_t break_flag = false;
	for (; i < n; ++i)
	{
		const int32_t c = src[i];
		if (c == ' ' || c == '\n' || c == '\t' || break_flag)
		{
			i -= break_flag;
			break;
		}
		for (int32_t j = 0; j < num_graphs; ++j)
		{
			const int32_t len = len_graphs[j];
			if (c == *graphs[j])
			{
					break_flag = true;
					break;
			}
		}
	}
	const int16_t len = i - i0;
	*out_len = len;
	char** keywords = flex_table.keywords;
	const int16_t* len_keywords = flex_table.keyword_lens;
	const int32_t num_keywords = flex_table.num_keywords;
	for (i = 0; i < num_keywords; ++i)
	{
		if (len_keywords[i] == len && strncmp(keywords[i], token, len) == 0)
		{
			return flex_table.keyword_ids[i];
		}
	}
	return GDSCRIPT_IDENTIFIER;
}

/* static int32_t delimiter(const char* src, const int32_t i0, const int32_t n) */
/* { */
/* 	const char* delims = flex_table.delim_chars; */
/* 	const int32_t num_delims = flex_table.num_delim_chars; */
/* 	for (int32_t i = i0; i < n; ++i) */
/* 	{ */
/* 		const char c = src[i]; */
/* 		for (int8_t j = 0; j < num_delims; ++j) */
/* 		{ */
/* 			if (c == delims[j]) */
/* 			{ */
/* 				return i; */
/* 			} */
/* 		} */
/* 	} */
/* 	return n; */
/* } */

static int32_t num_delimiter(const char* src, const int32_t i0, const int32_t n)
{
	for (int32_t i = i0; i < n; ++i)
	{
		const char c = src[i];
		const bool_t num_flag = '0' <= c && c <= '9';
		const bool_t hex_flag = ('a' <= c && c <= 'f') || ('A' <= c && c <= 'F');
		const bool_t etc_flag = c == '.' || c == '_' || c == '-' || c == 'x';
		if (num_flag || hex_flag || etc_flag)
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
	char c0 = src[i0 + 2];
	for (int32_t i = i0 + 3; i < n - 3; ++i)
	{
		const char c = src[i];
		if (c0 != '\\' && c == '"' && src[i + 1] == '"' && src[i + 2] == '"')
		{
			return i + 3;
		}
		c0 = c;
	}
	return n;
}

static int32_t str31_delimiter(const char* src, const int32_t i0, const int32_t n)
{
	for (int32_t i = i0 + 3; i < n - 3; ++i)
	{
		if (src[i] == '\'' && src[i + 1] == '\'' && src[i + 2] == '\'')
		{
			return i + 3;
		}
	}
	return n;
}

static int32_t str12_delimiter(const char* src, const int32_t i0, const int32_t n)
{
	char c0 = i0 > 0 ? src[i0 - 1] : 0;
	for (int32_t i = i0 + 1; i < n; ++i)
	{
		const char c = src[i];
		if (c0 != '\\' && c == '"')
		{
			return i + 1;
		}
		c0 = c;
	}
	return n;
}

static int32_t str11_delimiter(const char* src, const int32_t i0, const int32_t n)
{
	for (int32_t i = i0 + 1; i < n; ++i)
	{
		if (src[i] == '\'')
		{
			return i + 1;
		}
	}
	return n;
}

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

static void hustle(const gdscript_id_t id0, int32_t* const offsets, int16_t* const lens, gdscript_id_t* const ids, int32_t* const num, id_stack* const expected)
{
}

// arena_t has atleast 8 * n + 4096 bytes. Extras for inserting scope tokens & missing closing pairs.
// NOTE: 2 hidden scratch checkouts
// NOTE: semicolon must be followed by an EXPRESSION.  A REAL expression!
// FUCK IT WE BALL
// NOTE: DO NOT ADD SCOPES AROUND ONE LINERS -- JUST THUNG IT OUT.
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
				const int32_t N = expected.size;
				if (N == 0) { /* i can FEEL the branch being predicted */ }
				else
				{
						int32_t num0 = num > 0 ? num - 1 : 0;
						const int32_t offset0 = offsets[num0];
						for (int j = N; j > 0; --j)
						{
							const gdscript_id_t open_id = expected.allocation[j - 1] + 3;
							for (int k = num0; k >= 0; --k)
							{
								if (ids[k] == open_id)
								{
									num0 = k;
									break;
								}
							}
							offsets[num + N - j] = offsets[num0];
							lens[num + N - j] = offset0 - offsets[num0];
							ids[num + N - j] = missing(open_id - 3);
						}
						num += N;
						expected.size = 0;
				}
				offsets[num] = i;
				lens[num] = 1;
				ids[num] = GDSCRIPT_LINE_END;
				++num;
				lex_mask &= ~LWS_INACTIVE;
				lws = 0;
			}
			else if (c == ';')
			{
				// TODO: semicolon
			}
			else
			{
				const char next_char = i + 1 < n ? src[i + 1]: 0;
				const bool_t num_flag = ('0' <= c && c <= '9') ||
					(c == '.' && '0' < next_char && next_char <= '9');
				lex_mask &= ~STR_MASK;
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
					++num;
					--i;
				}
				else if (lex_mask & STR_MASK)
				{
					const bool_t raw_flag = (lex_mask & STR_RAW) >> 6;
					lex_mask |= (raw_flag && next_char == '\'') << 4;
					lex_mask |= (raw_flag && next_char == '"') << 5;
					const int32_t i0 = i;
					const char c2 = i0 + 1 + raw_flag < n ? src[i0 + 1 + raw_flag] : 0;
					const char c3 = i0 + 2 + raw_flag < n ? src[i0 + 2 + raw_flag] : 0;
					const char quote = lex_mask & STR_QUOTE ? '\'' : '"';
					lex_mask |= (c2 == quote && c3 == quote) * STR_TRIPLE;
					const uint8_t str_mask = lex_mask & ~STR_RAW & STR_MASK;
					if (str_mask == STR_DOUBLE)
					{
						i = str12_delimiter(src, i0 + raw_flag, n);
						fflush(stdout);
					}
					else if (str_mask == STR_QUOTE)
					{
						i = str11_delimiter(src, i0 + raw_flag, n);
					}
					else if (str_mask == (STR_TRIPLE | STR_DOUBLE))
					{
						i = str32_delimiter(src, i0 + raw_flag, n);
					}
					else if (str_mask == (STR_TRIPLE | STR_QUOTE))
					{
						i = str31_delimiter(src, i0 + raw_flag, n);
					}
					offsets[num] = i0;
					lens[num] = i - i0;
					ids[num] = GDSCRIPT_STRING;
					++num;
					--i;
				}
				else
				{
					int16_t len;
					gdscript_id_t id = tokenize(src, i, n, &len); // NOTE: DO NOT ADD 1 TO i
					const gdscript_id_t id1 = num > 0 ? ids[num - 1] : GDSCRIPT_NONE;
					const gdscript_id_t id2 = num > 1 ? ids[num - 2] : GDSCRIPT_NONE;
					const gdscript_id_t type_delta = GDSCRIPT_OP_TYPE - GDSCRIPT_COLON;
					id += (id == GDSCRIPT_COLON) * (id2 == GDSCRIPT_VAR) * type_delta;
					id += (id == GDSCRIPT_COLON) * (id2 == GDSCRIPT_CONST) * type_delta;
					id += (id == GDSCRIPT_COLON) * (expected.size > 0) * type_delta;
					const gdscript_id_t comma_delta = GDSCRIPT_EXTRA_COMMA - GDSCRIPT_COMMA;
					id += (id == GDSCRIPT_COMMA) * (id1 == GDSCRIPT_EXTRA_COMMA) * comma_delta;
					id += (id == GDSCRIPT_COMMA) * (id1 == GDSCRIPT_COMMA) * comma_delta;
					id += (id == GDSCRIPT_COMMA) * (expected.size == 0) * comma_delta;
					/* i += len - 1; */
					/* int16_t id_mask = id; */
					if (id_stack_peek(&expected) == id)
					{
						--expected.size;
					}
					else if (id == GDSCRIPT_PAREN_OPEN || id == GDSCRIPT_BRACE_OPEN || id == GDSCRIPT_BRACKET_OPEN)
					{
						id_stack_push(&expected, id - 3);
					}
					// TODO: BURIED PAIRS & UNEXPECTED CLOSURES
					#define X(ID, ...) || id == GDSCRIPT_##ID
					else if (expected.size > 0 && id == GDSCRIPT_COLON GDSCRIPT_ASSIGNMENT_OPERATORS)
					#undef X
					{
						const int32_t N = expected.size;
						int32_t num0 = num > 0 ? num - 1 : 0;
						const int32_t offset0 = offsets[num0];
						for (int j = N; j > 0; --j)
						{
							const gdscript_id_t open_id = expected.allocation[j - 1] + 3;
							for (int k = num0; k >= 0; --k)
							{
								if (ids[k] == open_id)
								{
									num0 = k;
									break;
								}
							}
							offsets[num + N - j] = offsets[num0];
							lens[num + N - j] = offset0 - offsets[num0];
							ids[num + N - j] = missing(open_id - 3);
						}
						num += N;
						expected.size = 0;
						// dump expected
					}
					// check if in stack
					// push ids onto expected
					offsets[num] = i;
					lens[num] = len;
					ids[num] = id;
					++num;
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
				table->num_tokens = 1;
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
			const int32_t depth0 = i16_stack_peek(&depths);
			if (lws == depth0)
			{
				lex_mask |= LWS_INACTIVE;
				--i;
				continue;
			}
			else if (lws > depth0)
			{
				i16_stack_push(&depths, lws);
				offsets[num] =  i;
				lens[num] = 0;
				ids[num] = GDSCRIPT_SCOPE_START;
				++num;
			}
			else
			{
				bool_t match_flag = false;
				int32_t pop_index = 0;
				int32_t fallback_index = 0;
				const int32_t N = depths.size;
				for (; pop_index < N; ++pop_index)
				{
					const int16_t depth = depths.allocation[pop_index];
					if (depth < lws)
					{
						fallback_index = pop_index;
					}
					else if (depth == lws)
					{
						match_flag = true;
						break;
					}
				}
				if (match_flag)
				{
					for (int32_t k = N; k > pop_index + 1; --k)
					{
						offsets[num + N - k] = i;
						lens[num + N - k] = 0;
						ids[num + N - k] = GDSCRIPT_SCOPE_END;
					}
					depths.size = pop_index + 1;
					num += N - pop_index - 1;
				}
				else
				{
					const int32_t num0 = num - 1;
					for (; num >= 0; --num)
					{
						if (ids[num] == GDSCRIPT_SCOPE_START && --depths.size == fallback_index + 1)
						{
							lens[num] = offsets[num0] - offsets[num] + lens[num0];
							ids[num] = GDSCRIPT_IMPROPER_DEPTH;
							break;
						}
					}
					depths.size = fallback_index + 2;
					depths.allocation[fallback_index + 1] = lws;
					offsets[num + 1] =  i;
					lens[num + 1] = 0;
					ids[num + 1] = GDSCRIPT_SCOPE_START;
					num += 2;
				}
			}
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
	// NOTE: run this whilestve shunting
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




int32_t gdscript_shunt_etc(token_table_t* const dest, const token_table_t* src, int32_t n)
{
	dest->num_tokens = n;
	// shunt
	// also check numbers & identifiers
	// and unary-ize appropriate operators
	// and sublimate some errors to the END
	// whereas condense missing errors into the desired token
	// allow perversions to pass to type check
	// etc. errors
	return 0;
}
