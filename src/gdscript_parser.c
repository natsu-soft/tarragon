#include "gdscript_parser.h"
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "token.h"
#include "arena.h"

#define TOKENIZE_RESIZE_OVERHEAD  2048

static gdscript_id_t encountered_delimiter(const char* str);
static void new_token(arena_t* token_arena, arena_t* char_arena, const char* str, const int n, const uint32_t line, const uint32_t col);

// TODO: dont segfault -- check OoB
static gdscript_id_t encountered_delimiter(const char* str)
{
	#define GDSCRIPT_X(STR, ID) \
	for (unsigned int i = 0; i < strlen(STR); ++i) \
	{ \
		if (str[i] != STR[i]) \
		{ \
			goto stop_##ID; \
		} \
	} \
	return ID; \
	stop_##ID: // DEVILISH
	GDSCRIPT_DELIMITING_TOKENS
	return 0;
	#undef GDSCRIPT_X
}

static void new_token(arena_t* token_arena, arena_t* char_arena, const char* str, const int n, const uint32_t line, const uint32_t col)
{
	token_t* t = arena_alloc(token_arena, sizeof *t);
	t->line = line;
	t->col = col;
	char* token_str = arena_alloc(char_arena, n + 1);
	memcpy(token_str, str, n);
	token_str[n] = '\0';
	t->string = token_str;
}

uint32_t gdscript_tokenize(arena_t* arena, token_t** tokens, gdscript_id_t** ids, const char* str, const uint32_t n)
{
	uint32_t num_chars = 0;
	uint32_t num_tokens = 0;
	uint32_t pos = 0;
	enum {
		CHAR_TOKEN = 0x00,
		LEADING_WHITESPACE_TOKEN = 0x01,
		BASIC_TOKEN = 0x02,
		STRING_TOKEN = 0x03,
		ANNOTATION_TOKEN = 0x04,
		RAW_STRING_TOKEN = 0x08
	} token_flag = LEADING_WHITESPACE_TOKEN;
	// figure out malloc size
	// TODO: BOUNDS CHECKING
	for (int i = 0; i < n; ++i)
	{
		char c = str[i];
	}
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
		arena->memory = malloc(req_size + TOKENIZE_RESIZE_OVERHEAD);
	}
	arena->size = 0;
	arena_t token_arena =
	{
		.memory = arena->memory,
		.cap = num_tokens * sizeof **tokens
	};
	arena_t id_arena =
	{
		.memory = token_arena.memory + token_arena.cap,
		.cap = num_tokens * sizeof **ids
	};
	arena_t char_arena =
	{
		.memory = id_arena.memory + id_arena.cap,
		.cap = num_chars,
	};
	uint32_t line = 1;
	uint32_t column = 1;
	token_flag = LEADING_WHITESPACE_TOKEN;
	for (uint32_t i = 0; i < n; ++i)
	{
		char c = str[i];
		if (c == '\n')
		{
			line++;
			column = 1;
		}
		switch (token_flag & (RAW_STRING_TOKEN - 1))
		{
		case STRING_TOKEN:
			if (c == '\"' || c == '\'')
			{
				uint32_t len = 0;
				// TODO: bounds checking
				// NOTE: less likely
				if (str[i - 1] == '\\')
				{
					continue;
				}
				else if (str[i + 1] != c)
				{
					len = pos - i; // ummmm, sweaty?
				}
				else if  (str[i + 1] == c && str[i + 2] == c)
				{
					len = pos - i; // frfr???
				}
				else
				{
				}
				new_token(&token_arena, &char_arena, str + pos, len, line, column);
				gdscript_id_t* id = arena_alloc(&id_arena, sizeof *id);
				*id = token_flag & RAW_STRING_TOKEN ? GDSCRIPT_RAW_STRING : GDSCRIPT_REGULAR_STRING;
			}
			break;
		case BASIC_TOKEN:
		{
			uint32_t delim_len;
			if (isspace(c))
			{
				num_chars += pos - i;
				num_tokens++;
				token_flag = c != '\n' ? CHAR_TOKEN : LEADING_WHITESPACE_TOKEN;
			}
			else if ((delim_len = encountered_delimiter(str + i)) > 0)
			{
				num_chars += pos - i;
				num_chars += delim_len + 1;
				num_tokens += 2;
				token_flag = c != '\n' ? CHAR_TOKEN : LEADING_WHITESPACE_TOKEN;
			}
			break;
		}
		case LEADING_WHITESPACE_TOKEN:
			if (c == ' ' || c == '\t')
			{
				num_chars += 2;
				num_tokens++;
				continue;
			}
			else
			{
				token_flag = CHAR_TOKEN;
			}
			__attribute__((fallthrough));
		default:
			if (isspace(c))
			{
				continue;
			}
			switch (c)
			{
			case 'r':
				if (str[i + 1] == '"' || str[i + 1] == '\'')
				{
					c = str[++i];
					__attribute__((fallthrough));
				}
				else
				{
					pos = i;
					token_flag = BASIC_TOKEN;
					break;
				}
			case '\'': __attribute__((fallthrough));
			case '"':
				if (c == str[i + 1] && c == str[i + 2])
				{
					i += 2;
				}
				pos = i;
				token_flag = STRING_TOKEN;
				break;
			case '\\':
				num_chars += 2;
				num_tokens++;
				break;
			case '\n':
				num_chars += 2;
				num_tokens++;
				token_flag = str[i - 1] != '\\' ? CHAR_TOKEN : LEADING_WHITESPACE_TOKEN;
				break;
			default:
				pos = i;
				token_flag = BASIC_TOKEN;
			}
		}
		column++;
	}
	*tokens = token_arena.memory;
	*ids = id_arena.memory;
	return num_tokens;
}

int main(int argc, char** argv)
{
	return 0;
}
