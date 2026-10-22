#include "lex.h"
/* #include <stdlib.h> */
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "../allocators/scratch.h"
#include "../memery.h"
#include "token_ids_types.h"
#include "token_id_map.h"
#include "diagnostics.h"
#include "../stdout_ring.h"
/* #include "../cursor.h" */
#include "../strings.h"
#include "../allocators/arena.h"
#include "../macro_stack.h"
#include "../macro_bitset.h"
/* #include "../macro_ring.h" */
// it's lite, it's fast; it'll do.

// this mite be SLIGHTLY faster

enum __attribute__((packed))
{
	LWS_NONE		= 0x00,
	LWS_SPACES		= 0x01, // bitflag
	LWS_TABS		= 0x02, // bitflag
	// bitwise or of the previous 2
	LWS_BOTH_ERR	= 0x03, // too clever to remember
};
#define LWS_MASK 0x0f

enum __attribute__((packed))
{
	LEX_DEFAULT			= 0x00,
	LEX_FUNC_DECL		= 0x10,
	LEX_IF_ELSE_STMT	= 0x20,
	LEX_IN_ONELINE		= 0x40,
	/* LEX_CONST_OR_VAR	= 0x80, */
};
#define LEX_MASK 0xf0

enum __attribute__((packed))
{
	CTRL_DEFAULT		= 0x00,
	CTRL_DUMP_EXPECTED	= 0x01,
	CTRL_FIRST_NON_LWS	= 0x02,
	CTRL_START_ONELINER = 0x04,
	CTRL_END_ONELINER	= 0x08,
};
#define CTRL_MASK 0x0f

struct token
{
	int32_t offset;
	int32_t len;
	token_id_t id;
};

struct stack_token
{
	int32_t offset;
	token_id_t id;
};

struct str_map
{
	string* values;
	char**  keys;
	int8_t* lens;
	int8_t cap;
	int8_t size;
};

static inline int8_t flat_hash(const char* str, const int8_t len)
{
	/* int64_t hash = 5381; */
	int32_t hash = 0;
	for (int8_t i = 0; i < len; ++i)
	{
		/* hash += ((hash << 5) + hash) + str[i]; */
		hash += str[i];
	}
	return hash & INT8_MAX;
}

#define USE_DJB2
static inline int8_t djb2(const char* str, const int8_t len)
{
	int64_t hash = 5381;
	for (int8_t i = 0; i < len; ++i)
	{
		hash += ((hash << 5) + hash) + str[i];
	}
	return hash & INT8_MAX;
}

static void str_map_init(struct str_map* map, uint8_t* allocation, const int8_t cap)
{
	assert((cap & (cap - 1)) == 0 && cap > 1);
	map->cap = cap;
	map->values = allocation;
	map->keys = map->values + cap;
	map->lens = map->keys + cap;
	memset(map->lens, 0x00, cap * sizeof *map->lens);
	map->size = 0;
}

static void str_map_cache(struct str_map* map, char* key, int8_t len, string value)
{
	const int8_t cap = map->cap;
	const uint8_t cap_mask = cap - 1;
	#ifdef USE_DJB2
	const int8_t index = djb2(key, len) & cap_mask;
	#else
	const int8_t index = flat_hash(key, len) & cap_mask;
	#endif
	int8_t* lens = map->lens;
	for (int8_t i = 0; i < cap; ++i)
	{
		const int8_t j = (index + i) & cap_mask;
		if (lens[j] == 0)
		{
			map->keys[j] = key;
			lens[j] = len;
			map->values[j] = value;
			map->size++;
			return;
		}
		else if (lens[j] < len)
		{
			char* temp_key = map->keys[j];
			int8_t temp_len = lens[j];
			string temp_val = map->values[j];
			map->keys[j] = key;
			lens[j] = len;
			map->values[j] = value;
			key = temp_key;
			len = temp_len;
			value = temp_val;
		}
	}
}

static string str_map_fetch(struct str_map* map, char* key, int8_t len)
{
	const int8_t cap = map->cap;
	const uint8_t cap_mask = cap - 1;
	#ifdef USE_DJB2
	const int8_t index = djb2(key, len) & cap_mask;
	#else
	const int8_t index = flat_hash(key, len) & cap_mask;
	#endif
	int8_t* lens = map->lens;
	char** keys = map->keys;
	for (int8_t i = 0; i < cap; ++i)
	{
		const int8_t j = (index + i) & cap_mask;
		const int8_t l = lens[j];
		if (l == len && strncmp(key, keys[j], len) == 0)
		{
			return map->values[j];
		}
		else if (l == 0)
		{
			break;
		}
	}
	return (string){0};
}

struct byte_set
{
	uint8_t* buffer;
};

static inline void byte_set_add(struct byte_set* byte_set, const uint8_t value)
{
	byte_set->buffer[value >> 3] |= (1 << (value & 0x07));
}

/* RING(token, struct token) // used outside of expressions */
STACK(token, struct stack_token)
STACK(i16, int16_t)

static token_id_map_t grapheme_map = {0};
static token_id_map_t keyword_map = {0};
static struct str_map str_delim_map = {0};

BITSET_SPAN_TYPE(char, char)
BITSET_SPAN_TYPE(id, token_id_t)

/* static struct byte_set delimiters = {0}; */
/* static struct byte_set unary_triggers = {0}; */
static struct char_bitset delimiters = {0};
static struct id_bitset unary_triggers = {0};

/* static inline uint8_t delimiter(const uint8_t value) */
/* { */
/* 	return delimiters.buffer[value >> 3] & (1 << (value & 0x07)); */
/* } */

#define delimiter(CHAR) (char_bitset_has(&delimiters, CHAR))
#define make_unary(TOKEN) (id_bitset_has(&unary_triggers, TOKEN))

/* static inline uint8_t make_unary(const uint8_t value) */
/* { */
/* 	return unary_triggers.buffer[value >> 3] & (1 << (value & 0x07)); */
/* } */

#define STRING_DELIMITERS						\
	X("'",       1, "'",      1)				\
	X("\"",      1, "\"",     1)				\
	X("r'",      2, "'",      1)				\
	X("r\"",     2, "\"",     1)				\
	X("&\"",     2, "\"",     1)				\
	X("^\"",     2, "\"",     1)				\
	X("'''",     3, "'''",    3)				\
	X("\"\"\"",  3, "\"\"\"", 3)				\

#define UNARY_TRIGGERS \
	GDSCRIPT_OPERATORS \
	X(LINE_END) \
	X(COMMA) \
	X(PAREN_OPEN) \
	X(BRACE_OPEN) \
	X(BRACKET_OPEN) \
	X(SCOPE_OPEN) \
	X(SCOPE_CLOSE)

/* __attribute__((constructor)) */
void lex_init(void)
{
	// tbqh I should just mmap 10mb for all of these inits instead of faffing around with counting.
	virtual_buffer vb = memalloc(5 * KB); // and just "leak" it lol lmao
	arena_t arena = { .addr = vb.addr, .cap = vb.size };
	const int32_t token_id_map_scale = sizeof(char*) + sizeof(int8_t) + sizeof(token_id_t);
	token_id_map_init(&grapheme_map, arena_alloc(&arena, 256 * token_id_map_scale), 256);
	arena_align2(&arena, 64);
	token_id_map_init(&keyword_map, arena_alloc(&arena, 256 * token_id_map_scale), 256);
	/* delimiters.buffer = arena_alloc(&arena, 16); */
	/* unary_triggers.buffer = arena_alloc(&arena, 16); */
	arena_align2(&arena, 64);
	const int32_t str_map_scale = sizeof(string) + sizeof(char*) + sizeof(int8_t);
	str_map_init(&str_delim_map, arena_alloc(&arena, 32 * str_map_scale), 32);
	/* memset(delimiters.buffer, 0x00, 16); */
#define X(_, STR,...) char_bitset_add(&delimiters, STR[0]);
	GDSCRIPT_DELIMITING_TOKENS
#undef X
	char_bitset_add(&delimiters, ' ');
#define X(_, STR,...) assert(delimiter(STR[0]) && "delimiters doesn't contain leading character from " #_ ": '" STR "'\n");
	GDSCRIPT_GRAPHEMES
#undef X
		assert(delimiter(' '));
		assert(delimiter('.'));
#define X(ID, ...) id_bitset_add(&unary_triggers, GDSCRIPT_##ID);
		UNARY_TRIGGERS
#undef X
#define X(ID, ...) assert(make_unary(GDSCRIPT_##ID) && "unary triggers doesn't contain GDSCRIPT_" #ID "\n");
		UNARY_TRIGGERS
#undef X
#define X(ENUM, STR, LEN) \
		token_id_map_cache(&grapheme_map, arena_push(&arena, STR, LEN), LEN, GDSCRIPT_##ENUM);
		GDSCRIPT_GRAPHEMES
#undef X
#define X(ENUM, STR, LEN) \
	token_id_map_cache(&keyword_map, arena_push(&arena, STR, LEN), LEN, GDSCRIPT_##ENUM);
	GDSCRIPT_KEYWORD_OPERATORS
	GDSCRIPT_KEYWORDS
	GDSCRIPT_ANNOTATIONS
#undef X
#define X(A, A_LEN, B, B_LEN) \
		str_map_cache(&str_delim_map, arena_push(&arena, A, A_LEN), A_LEN, STRINGN(B, B_LEN));
		STRING_DELIMITERS
#undef X
#define X(ENUM, STR, LEN) \
		assert(token_id_map_fetch(&grapheme_map, STR, LEN) == GDSCRIPT_##ENUM); \
		GDSCRIPT_GRAPHEMES
#undef X
#define X(ENUM, STR, LEN) \
		assert(token_id_map_fetch(&keyword_map, STR, LEN) == GDSCRIPT_##ENUM);
		GDSCRIPT_KEYWORD_OPERATORS
		GDSCRIPT_KEYWORDS
		GDSCRIPT_ANNOTATIONS
#undef X
#define X(A, A_LEN, B, B_LEN) \
		assert(str_map_fetch(&str_delim_map, A, A_LEN).length);
		STRING_DELIMITERS
#undef X
		/* token_id_map_print(&grapheme_map); */
		/* token_id_map_print(&keyword_map); */
}

static inline void string_token(struct token* token, const string end_delimiter, int32_t* cursor, const char* src, const int32_t length)
{
	const int32_t end_len = end_delimiter.length;
	for (; *cursor < length - end_len + 1; ++(*cursor))
	{
		const char c = src[*cursor];
		if (c != '\\')
		{
			if (strncmp(src + *cursor, end_delimiter.chars, end_len) == 0)
			{
				token->len = *cursor - token->offset + end_len;
				*cursor += end_len;
				return;
			}
		}
		else
		{
			++(*cursor);
		}
	}
	token->len = length - token->offset;
	token->id = GDSCRIPT_UNTERMINATED_STRING;
}

static inline void number_token(struct token* token, int32_t* cursor, const char* src, const int32_t length)
{
	enum __attribute__((packed))
	{
		NUM_NONE = 0x00,
		NUM_HAS_PERIOD = 0x01,
		NUM_IN_EXPONENT = 0x02
	};
	uint8_t number_flags = NUM_NONE;
	for (; *cursor < length; ++*cursor)
	{
		const char c = src[*cursor];
		if ('0' <= c && c <= '9') { continue; }
		else if (c != '-' && c != '.' && delimiter(c))
		{
			token->len = *cursor - token->offset;
			return;
		}
		else if (c == '.')
		{
			token->id = number_flags ? GDSCRIPT_PERVERTED_NUMBER : GDSCRIPT_FLOAT;
			number_flags |= NUM_HAS_PERIOD;
		}
		else if (c == 'e')
		{
			token->id = (number_flags & NUM_IN_EXPONENT) ? GDSCRIPT_PERVERTED_NUMBER : GDSCRIPT_FLOAT;
			number_flags |= NUM_IN_EXPONENT;
		}
		else if (c == '-' && src[*cursor - 1] == 'e')
		{
			continue;
		}
		else
		{
			token->id = GDSCRIPT_PERVERTED_NUMBER;
		}
	}
	token->len = length - token->offset;
}

static inline void bin_token(struct token* token, int32_t* cursor, const char* src, const int32_t length)
{
	for (; (*cursor) < length; ++(*cursor))
	{
		const char c = src[*cursor];
		if (delimiter(c))
		{
			token->len = *cursor - token->offset;
			return;
		}
		else if (c != '0' || c != '1')
		{
			token->id = GDSCRIPT_PERVERTED_BINARY;
		}

	}
	token->len = length - token->offset;
}

static inline void hex_token(struct token* token, int32_t* cursor, const char* src, const int32_t length)
{
	for (; *cursor < length; ++(*cursor))
	{
		const char c = src[*cursor];
		if (delimiter(c))
		{
			token->len = *cursor - token->offset;
			return;
		}
		else if (!(('0' <= c && c <= '9') || ('a' <= c && c <= 'f') || ('A' <= c && c <= 'F')))
		{
			token->id = GDSCRIPT_PERVERTED_HEXADECIMAL;
		}
	}
	token->len = length - token->offset;
}

enum
{
	MIXED_LWS = -1
};

static int8_t scan_lws(int32_t* cursor, uint8_t* lex_state, const char* src, const int32_t length, char* bad_lws)
{
	int32_t i = *cursor;
	for (; *cursor < length; ++*cursor)
	{
		const char c = src[*cursor];
		if (c == ' ' || c == '\t')
		{
			*lex_state |= c == ' ' ? LWS_SPACES : LWS_TABS;
			if ((*lex_state & LWS_MASK) != LWS_BOTH_ERR)
			{
				*bad_lws = c;
				continue;
			}
			else
			{
				return MIXED_LWS;
			}
		}
		else
		{
			break;
		}
	}
	return 0;
}

static void bad_lws_diagnostics(const char bad_lws, const char* src, const int32_t length)
{
	uint8_t lws_mask = LWS_NONE;
	for (int i = 0; i < length; ++i)
	{
		const char c = src[i];
		if (!(c == ' ' || c == '\t'))
		{
			for (; i < length && src[i] != '\n'; ++i);
		}
		else if (c == bad_lws)
		{
			const int32_t i0 = i;
			for (; i < length && src[i] == bad_lws; ++i);
			diagnostics_add(DIAGNOSTIC_LWS_USE_TABS + bad_lws == ' ', src, length, "TODO: path", i0, i - i0);
		}
	}
}

static inline void next_token(struct token* token, int32_t* cursor, const char* src, const int32_t length)
{
	for (; *cursor < length; ++*cursor)
	{
		start_iteration: /* GOOD OLE nop */;
		const int32_t offset0 = *cursor;
		token->offset = offset0;
		const char c = src[offset0];
		if (!(c == '\n' || c == ';' || c == '#' || c == '\t' || c == ' ' || c == '\\'))
		{
			const int32_t max_len = length - offset0;
			// DONE: (0) check for graphemes
			char* src0 = src + offset0;
			if (('&' <= c && c <= '/') ||
				(':' <= c && c <= '?') ||
				('[' <= c && c <= '^') ||
				('{' <= c && c <= '~') || c == '!')
			{
				for (int8_t l = max_len > 3 ? 3 : max_len; l > 0; --l)
				{
					const token_id_t id = token_id_map_fetch(&grapheme_map, src0, l);
					if (id != GDSCRIPT_NONE)
					{
						token->len = l;
						token->id = id;
						*cursor += l;
						goto advance_to_next_token;
					}
				}
			}
			// (1) check for numbers
			else if (('0' <= c && c <= '9') || c == '.')
			{
				token->id = GDSCRIPT_INTEGER;
				if (c == '0' && length - offset0 > 0 &&
					src[offset0 + 1] == 'b')
				{
					*cursor += 2;
					bin_token(token, cursor, src, length);
				}
				else if (c == '0' && length - offset0 > 0 &&
					src[offset0 + 1] == 'x')
				{
					*cursor += 2;
					hex_token(token, cursor, src, length);
				}
				else if (c != '.' || ('0' <= src[offset0 + 1] && src[offset0 + 1] <= '9'))
				{
					number_token(token, cursor, src, length);
				}
				else
				{
					token->id = GDSCRIPT_PERIOD;
					token->len = 1;
					++*cursor;
				}
				goto advance_to_next_token;
			}
			// (2) check for string
			else if (c == '"' || c == '\'' || c == '&' || c == '^' || c == 'r')
			{
				for (int8_t l = max_len > 3 ? 3 : max_len; l > 0; --l)
				{
					const string end_delimiter = str_map_fetch(&str_delim_map, src0, l);
					if (end_delimiter.length > 0)
					{
						if (!(c == '&' || c == '^'))
						{
							token->id = GDSCRIPT_STRING;
						}
						else if (c == '&')
						{
							token->id = GDSCRIPT_STRING_NAME;
						}
						else if (c == '^')
						{
							token->id = GDSCRIPT_NODE_PATH;
						}
						*cursor += l;
						string_token(token, end_delimiter, cursor, src, length);
						goto advance_to_next_token;
					}
				}
			}
			// if it's NONE of the above it's either a keyword or an identifier
			// NOTE: do NOT else this!!!
			for (++(*cursor); *cursor < length; ++(*cursor))
			{
				if (delimiter(src[*cursor]))
				{
					break;
				}
			}
			const int32_t len = *cursor - offset0;
			token->len = len;
			token_id_t id = token_id_map_fetch(&keyword_map, src0, len);
			if (id != GDSCRIPT_NONE)
			{
				token->id = id;
			}
			else
			{
				token->id = c == '@' ? GDSCRIPT_UNKNOWN_ANNOTATION : GDSCRIPT_IDENTIFIER;
			}
		}
		else if (c == '\n' || c == ';')
		/* else if (0) */
		{
			for (++(*cursor); src[*cursor] == c && *cursor < length; ++(*cursor))
			{
			}
			/* token->offset = *cursor - 1; */
			token->offset = *cursor - 1;
			token->len = 1;
			if (c == '\n')
			{
				token->id = GDSCRIPT_LINE_END;
			}
			else
			{
				token->id = GDSCRIPT_SEMICOLON;
				for (; *cursor < length; ++(*cursor))
				{
					const char d = src[*cursor];
					if (!(d == ' ' || d == '\t'))
					{
						return;
					}
				}
			}
			return;
		}
		/* else if (c == '\t' || c == ' ') */
		/* else if (0) */
		/* { */
		/* 	token->id = GDSCRIPT_LEADING_WHITESPACE; */
		/* 	for (++(*cursor); src[*cursor] == c && *cursor < length; ++(*cursor)) */
		/* 	{ */
		/* 		/\* LMAO *\/ */
		/* 	} */
		/* 	token->len = *cursor - offset0; */
		/* 	return; */
		/* } */
		else if (c == '#')
		{
			// NOTE: returning comments SOLELY for paren matching
			// they don't get cached in the table
			token->id = GDSCRIPT_LINE_END;
			for (++(*cursor); *cursor < length; ++(*cursor))
			{
				if (src[*cursor] == '\n')
				{
					for (++(*cursor); src[*cursor] == '\n' && *cursor < length; ++(*cursor)) ;
					token->len = *cursor - offset0;
					return;
					/* goto advance_to_next_token; */
				}
			}
		}
		else if (c == '\\')
		{
			if (src[++(*cursor)] == '\n')
			{
				for(++(*cursor); *cursor < length; ++(*cursor))
				{
					const char d = src[*cursor];
					if (!(d == ' ' || d == '\t'))
					{
						goto start_iteration;
					}
				}
				break;
			}
			else
			{
				token->offset = *cursor; // keep this one
				token->id = GDSCRIPT_TOKENS_AFTER_CONTINUATION;
				for (++(*cursor); *cursor < length; ++(*cursor))
				{
					if (src[*cursor] != '\n')
					{
						// AEIOU
					}
					else
					{
						break;
					}
				}
				token->len = *cursor - token->offset;
				return;
			}
		}
		// finally: skip to the next non-whitespace-sans-newline char
	advance_to_next_token: // NOTE: leave this HERE!
		for ( ; *cursor < length; ++(*cursor))
		{
			if (!(src[*cursor] == ' ' || src[*cursor] == '\t'))
			{
				return;
			}
		}
	}
	token->offset = length;
	token->len = 0;
	token->id = GDSCRIPT_EOF;
}

int32_t lex(source_data_t* src_data, arena_t* backing_arena, arena_t* work_arena, const char* src, const int32_t length)
{
	src_data->src = src;
	int32_t* offsets = arena_alloc(backing_arena, (length + 128) * sizeof *offsets);
	int16_t* lens = arena_alloc(backing_arena, (length + 128) * sizeof *lens);
	token_id_t* ids = arena_alloc(backing_arena, (length + 128) * sizeof *ids);
	int32_t n = 0;
	char bad_lws;
	// NOTE: very memey setup
	for (uint8_t i = 0; i < TOKEN_START; ++i)
	{
		offsets[i] = 0;
		lens[i] = 0;
		ids[i] = i;
	}
	n += TOKEN_START;
	struct token_stack scope_stack =
	{
		.buffer = arena_alloc(work_arena, length * sizeof *scope_stack.buffer),
		.cap = length
	};
	token_stack_push(&scope_stack, (struct stack_token){.offset = 0, .id = GDSCRIPT_NONE});
	struct i16_stack depth_stack =
	{
		.buffer = arena_alloc(work_arena, length * sizeof *depth_stack.buffer),
		.cap = length
	};
	i16_stack_push(&depth_stack, 0);
	uint8_t lws_state = LEX_DEFAULT | LWS_NONE;
	int32_t cursor = 0;
	int32_t paren_depth = 0;
	token_id_t prev_id = GDSCRIPT_NONE;
	struct token t;
	uint8_t newline = 1;
	const uint8_t unary_delta = GDSCRIPT_OP_IDENTITY - GDSCRIPT_OP_SUM;
	const uint8_t semicolon_delta = GDSCRIPT_LINE_END - GDSCRIPT_SEMICOLON;
	const uint8_t missing_delta = GDSCRIPT_MISSING_PAREN_CLOSE - GDSCRIPT_PAREN_CLOSE;
	const uint8_t extra_delta = GDSCRIPT_EXTRA_PAREN_CLOSE - GDSCRIPT_PAREN_CLOSE;
	/* const uint8_t ternary_delta = GDSCRIPT_TERNARY_IF - GDSCRIPT_IF; */
	for (next_token(&t, &cursor, src, length); t.id != GDSCRIPT_EOF; next_token(&t, &cursor, src, length))
	{
		const token_id_t id = t.id;
		const uint8_t unary_diff = make_unary(prev_id) &&
			(id == GDSCRIPT_OP_SUM || id == GDSCRIPT_OP_DIFFERENCE) ?
			unary_delta : 0;
		uint8_t scope_diff = 0;
		if (id == GDSCRIPT_PAREN_OPEN || id == GDSCRIPT_BRACKET_OPEN || id == GDSCRIPT_BRACKET_OPEN)
		{
			struct stack_token temp = { .offset = t.offset, .id = id + 3};
			token_stack_push(&scope_stack, temp);
		}
		else if (id == GDSCRIPT_PAREN_CLOSE || id == GDSCRIPT_BRACKET_CLOSE || id == GDSCRIPT_BRACKET_CLOSE)
		{
			for (int i = scope_stack.size - 1; i >= 0; --i)
			{
				if (scope_stack.buffer[i].id == id)
				{
					for (int32_t j = scope_stack.size - 1; j > i; --j)
					{
						struct stack_token missing_scope = scope_stack.buffer[j];
						offsets[n] = missing_scope.offset;
						lens[n] = t.offset - missing_scope.offset;
						ids[n++] = missing_scope.id + missing_delta;
					}
					scope_stack.size = i;
					goto append_token;
				}
			}
			scope_diff += extra_delta;
		}
	append_token:
		/* if (id != GDSCRIPT_LINE_END) || (scan_lws(&cursor, &lws_state, src, length) != MIXED_LWS && scope_stack.size == 1)) */
		if (id != GDSCRIPT_LINE_END)
		{
			offsets[n] = t.offset;
			lens[n] = t.len;
			ids[n++] = id + unary_diff + scope_diff;
			prev_id= id;
		}
		else if (scan_lws(&cursor, &lws_state, src, length, &bad_lws) == MIXED_LWS)
		{
			bad_lws_diagnostics(bad_lws, src, length);
			src_data->num_tokens = 0;
			return 0;
		}
		else
		{
			offsets[n] = t.offset;
			lens[n] = t.len;
			ids[n++] = id + scope_diff;
			prev_id = id;
		}
	}
	// token is now EOF, but you have to cleanup
	// NOTE: i > 0 to keep depth = 0
	for (int32_t i = scope_stack.size - 1; i > 0; --i)
	{
		struct stack_token missing_scope = scope_stack.buffer[i];
		offsets[n] = missing_scope.offset;
		lens[n] = length - missing_scope.offset;
		ids[n++] = missing_scope.id + missing_delta;
	}
	if (scope_stack.size > 0)
	{
		offsets[n] = length;
		lens[n] = 0;
		ids[n++] = GDSCRIPT_LINE_END;
	}
	lens = memmove(offsets + n + 1, lens, n * sizeof *lens);
	ids = memmove(lens  + n + 1, ids, n * sizeof *ids);
	offsets[n] = length;
	lens[n] = 0;
	ids[n] = GDSCRIPT_EOF;
	backing_arena->size = (ids + n) - backing_arena->addr;
	src_data->token_offsets = offsets;
	src_data->token_lens = lens;
	src_data->token_ids = ids;
	src_data->num_tokens = n;
	return n;
}

const int32_t lex_backing_size(const int32_t length)
{
	return (length + 128) * (sizeof(int32_t) + sizeof(int16_t) + sizeof(token_id_t));
}

const int32_t lex_work_size(const int32_t length)
{
	/* return length * (sizeof(int16_t) + 2 * sizeof(token_id_t)); */
	return length * (sizeof(struct stack_token) + sizeof(int16_t));
}
