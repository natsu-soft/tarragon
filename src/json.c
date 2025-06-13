#include "json.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include "file_sys.h"
#include "token.h"
#include "arena.h"
#include "scratch.h"

#define TOKENIZE_RESIZE_OVERHEAD 2048

/* struct token_ll */
/* { */
/* 	struct token_ll *next; */
/* 	token_t* token; */
/* }; */

static json_id_t id_token(const token_t* token, const uint32_t line, const uint32_t col);

/* static struct token_ll* new_token(arena_t* arena, const char* src, const uint32_t n, const int line, const int col); */

static void make_token(arena_t* token_arena, arena_t* id_arena, const char* src, const uint32_t pos, const uint32_t len, const uint32_t line, const uint32_t col);

static uint32_t tokenize(token_t** tokens, json_id_t** ids, arena_t* arena, const char* src, const int n);

static uint32_t find_end(const json_id_t closing_id, const json_id_t* ids, const token_t* token, const uint32_t num, const char* src);

static uint32_t find_end_quiet(const json_id_t closing_id, const json_id_t* ids, const uint32_t num);

static uint16_t analyze_object(json_id_t* ids, const token_t* tokens, const uint32_t num, const char* src);

static uint16_t analyze_array(json_id_t* ids, const token_t* tokens, const uint32_t num, const char* src);

#define JSON_CHARS \
	JSON_CHAR('{') ||  \
	JSON_CHAR('}') ||  \
	JSON_CHAR('[') ||  \
	JSON_CHAR(']') ||  \
	JSON_CHAR(',') ||  \
	JSON_CHAR(':')

#define DIGITS  \
	DIGIT_X('0') && \
	DIGIT_X('1') && \
	DIGIT_X('2') && \
	DIGIT_X('3') && \
	DIGIT_X('4') && \
	DIGIT_X('5') && \
	DIGIT_X('6') && \
	DIGIT_X('7') && \
	DIGIT_X('8') && \
	DIGIT_X('9')

#define MATCH_DEPTH \
	else if (id & JSON_OBJECT_START) \
	{ \
		++depth; \
	} \
	else if (id & JSON_OBJECT_END) \
	{ \
		if (depth == 0) \
		{ \
			do \
			{ \
					X \
			} \
			while(0); \
		} \
		else \
		{ \
			--depth; \
		} \
	}


/* static struct token_ll* new_token(arena_t* arena, const char* src, const uint32_t n, const int line, const int col) */
/* { */
/* 	struct token_ll* res = arena_alloc(arena, sizeof *res); */
/* 	res->next = NULL; */
/* 	token_t* token = arena_alloc(arena, sizeof *token); */
/* 	token->line = line; */
/* 	token->col = col - n; */
/* 	char* str = arena_alloc(arena, n + 1); */
/* 	memcpy(str, src, n); */
/* 	str[n] = '\0'; */
/* 	token->string = str; */
/* 	res->token = token; */
/* 	return res; */
/* } */

static void make_token(arena_t* token_arena, arena_t* id_arena, const char* src, const uint32_t pos, const uint32_t len, const uint32_t line, const uint32_t col)
{
	token_t* token = arena_alloc(token_arena, sizeof *token);
	token->string = src + pos;
	token->pos = pos;
	token->len = len;
	const json_id_t token_id  = id_token(token, line, col);
	// if it IS a json string remove the wrapping double quotes
	// but do some cheeky branchless memery
	const uint8_t str_flag = token_id == JSON_STRING;
	token->string += str_flag;
	token->pos += str_flag;
	token->len -= str_flag;
	json_id_t* id = arena_alloc(id_arena, sizeof *id);
	*id = token_id;
}


static uint32_t tokenize(token_t** tokens, json_id_t** ids, arena_t* arena, const char* src, const int n)
{
	/* printf("-- json size: %d bytes\n", n); */
	if (arena == NULL || src == NULL || *tokens != NULL) return 0;
	enum {
		CHAR_TOKEN = 0x00,
		BASIC_TOKEN,
		STRING_TOKEN
	} token_flag = CHAR_TOKEN;
	uint32_t pos = 0;
	uint32_t num_tokens = 0;
	/* uint32_t num_chars = 0; */
	#define JSON_CHAR(C) c == C
	for (int i = 0; i < n; ++i)
	{
		char c = src[i];
		switch (token_flag)
		{
		case STRING_TOKEN:
			if (c == '"' && src[i - 1] != '\\')
			{
				/* int len = i - pos + 1; */
				num_tokens += 1;
				/* num_chars += len - 1; // NOTE: doing this to exclude the quotes around strings */
				token_flag = CHAR_TOKEN;
            }
            break;
		case BASIC_TOKEN:
			if(isspace(c) || JSON_CHARS)
			{
				/* int len = i - pos; */
				num_tokens += 1;
				/* num_chars += len + 1; */
				token_flag = CHAR_TOKEN;
				__attribute__((fallthrough));
			}
			else {
				break;
			}
	   default:
		   if (JSON_CHARS)
		   {
				num_tokens += 1;
				/* num_chars += 2; */
		   }
		   else if (!isspace(c))
		   {
			   pos = i;
			   token_flag = c == '"' ? STRING_TOKEN : BASIC_TOKEN;
		   }
		}
	}
	/* const size_t req_size = num_tokens * (sizeof **tokens + sizeof **ids) + num_chars; */
	const size_t req_size = num_tokens * (sizeof **tokens + sizeof **ids);
	if (arena->cap < req_size)
	{
		if (arena->allocation != NULL)
		{
			free(arena->allocation);
		}
		arena->allocation = malloc(req_size + TOKENIZE_RESIZE_OVERHEAD);
		arena->cap = req_size;
	}
	arena->size = 0;
	// NOTE: has to be in THIS order or get fucked by alignment
	arena_t token_arena =
	{
		.allocation = arena->allocation,
		.cap = num_tokens * sizeof **tokens
	};
	arena_t id_arena =
	{
		.allocation = token_arena.allocation + token_arena.cap,
		.cap = num_tokens * sizeof **ids
	};
	/* arena_t char_arena = */
	/* { */
	/* 	.memory = id_arena.memory + id_arena.cap, */
	/* 	.cap = num_chars */
	/* }; */
	// TODO: better upper bound for how much space the arena will need
	/* arena_init(&a, 16 * n); // NOTE: if segfault just give it more memory why not fuck it we ball */
	/* struct token_ll** token = &a.memory; //  What do I use this for? PROPER LINKAGE */
	uint32_t line = 1;
	uint32_t col = 1;
	pos = 0;
	for (int i = 0; i < n; ++i)
	{
		char c = src[i];
		if (c == '\n')
		{
			++line;
			col = 1;
		}
		switch (token_flag)
		{
		case STRING_TOKEN:
			if (c == '"' && src[i - 1] != '\\')
			{
				int len = i - pos + 1;
				/* *token = new_token(&a, src + pos, len, line, col); */
				make_token(&token_arena, &id_arena, src, pos, len, line, col);
				/* token = &(*token)->next; */
				/* num_tokens += 1; */
				/* num_chars += len + 1; */
				/* num_chars += len - 1; // NOTE: doing this to exclude the quotes around strings */
				token_flag = CHAR_TOKEN;
            }
            break;
		case BASIC_TOKEN:
			if(isspace(c) || JSON_CHARS)
			{
				int len = i - pos;
				make_token(&token_arena, &id_arena, src, pos, len, line, col);
				/* *token = new_token(&a, src + pos, len, line, col); */
				/* token = &(*token)->next; */
				/* num_tokens += 1; */
				/* num_chars += len + 1; */
				token_flag = CHAR_TOKEN;
				__attribute__((fallthrough));
			}
			else {
				break;
			}
	   default:
		   if (JSON_CHARS)
		   {
			   make_token(&token_arena, &id_arena, src, i, 1, line, col);
				/* *token = new_token(&a, src + i, 1, line, col); */
				/* token = &(*token)->next; */
				/* num_tokens += 1; */
				/* num_chars += 2; */
		   }
		   else if (!isspace(c))
		   {
			   pos = i;
			   token_flag = c == '"' ? STRING_TOKEN : BASIC_TOKEN;
		   }
		}
		++col;
	}
	#undef JSON_CHAR
	// NOTE: ALIGNMENT!!!! WOW! ITS REAL!
	/* arena_t token_arena = */
	/* { */
	/* 	.memory = malloc(num_tokens * sizeof **tokens), */
	/* 	.cap = num_tokens * sizeof **tokens */
	/* }; */
	/* uint8_t* addr = malloc(num_chars + num_tokens * sizeof **ids); */
	/* arena_t id_arena = */
	/* { */
	/* 	.memory = addr, */
	/* 	.cap = num_tokens * sizeof(json_id_t) */
	/* }; */
	/* arena_t char_arena = */
	/* { */
	/* 	.memory = id_arena.memory + id_arena.cap, */
	/* 	.cap = num_chars */
	/* }; */
	/* for (struct token_ll* t = a.memory /\* DOPAMINE DOPAMINE DOPAMINE *\/; t != NULL; t = t->next) */
	/* { */
	/* 	token_t* t_tok = t->token; */
	/* 	token_t* token = arena_alloc(&token_arena, sizeof *token); */
	/* 	json_id_t* id = arena_alloc(&id_arena, sizeof *id); */
	/* 	*id = id_token(t_tok); */
	/* 	token->col = t_tok->col; */
	/* 	token->line = t_tok->line; */
	/* 	int n = strlen(t_tok->string) + 1; */
	/* 	if (*id == JSON_STRING) */
	/* 	{ */
	/* 		token->string = arena_alloc(&char_arena, n - 2); */
	/* 		memcpy(token->string, t_tok->string + 1, n - 2); */
	/* 		token->string[n - 3] = '\0'; */
	/* 	} */
	/* 	else */
	/* 	{ */
	/* 		token->string = arena_alloc(&char_arena, n); */
	/* 		memcpy(token->string, t_tok->string, n); */
	/* 	} */
	/* } */
	/* free(a.memory); */
	*tokens = token_arena.allocation;
	*ids = id_arena.allocation;
	return num_tokens;
}

static json_id_t id_token(const token_t* token, const uint32_t line, const uint32_t col)
{
	const uint32_t n = token->len;
	char* str = alloca(n + 1);
	memcpy(str, token->string, n);
	str[n] = '\0';
	switch (str[0])
	{
	#define JSON_X(C, A) case C: return A;
	JSON_STRUCTURE_LIST
	#undef JSON_X
	case '-': __attribute__((fallthrough));
	case '0' ... '9':
		{
			uint8_t int_flag = 1;
			for (uint32_t i = 1; i < strlen(str); ++i)
			{
				if (str[i] == '.')
				{
					if (int_flag)
					{
						int_flag = 0;
					}
					else
					{
						fprintf(stderr, "[JSON] ERROR: Numbers can only have a single decimal point!%d:%d %s\n", line, col + i, str);
						return JSON_ERROR;
					}
				}
				else if (str[i] == 'e' || str[i] == 'E')
				{
					fprintf(stderr, "[NERD] LMAO: This parser doesn't support scientific notation, LOSER!\n%d:%d %s\n", line, col + i, str);
					return JSON_ERROR;
				}
				#define DIGIT_X(C) str[i] != C
				else if(DIGITS)
				{
					fprintf(stderr, "[JSON] ERROR: Numbers can only contain digits and a single decimal point!\n%d:%d %s\n", line, col + i, str);
					return JSON_ERROR;
				}
				#undef DIGIT_X
			}
			return int_flag ? JSON_INTEGER : JSON_FLOAT;
		}
	#define X(STRING, VALUE) \
		if (strcmp(str, STRING) == 0) return VALUE; \
		else										\
		{ \
			fprintf(stderr, "[JSON] ERROR: Unacceptable literal name, did you mean " #STRING "?\n%d:%d %s\n", line, col, str); \
		} \
		break;
	case 'n':
		X("null", JSON_NULL);
	case 't':
		X("true", JSON_TRUE);
	case 'f':
		X("false", JSON_FALSE);
	#undef X
	case '"':
	{
		int n = strlen(str);
		for (int i = 1; i < n - 1; ++i)
		{
			if (str[i] == '"' && str[i - 1] != '\\')
			{
				fprintf(stderr, "[JSON] ERROR: Closing quotes before final character in string!\n%d:%d %s\n", line, col + i, str);
				return JSON_ERROR;
			}
		}
		if (str[n - 1] != '"')
		{
				fprintf(stderr, "[JSON] ERROR: A String must close with quotes!\n%d:%d %s\n", line, col + n, str);
				return JSON_ERROR;
		}
		return JSON_STRING;
	}
	default:
		fprintf(stderr, "[JSON] ERROR: Bad token!\n%d:%d %s\n", line, col, str);
	}
	return JSON_ERROR;
}

static uint32_t find_end(const json_id_t closing_id, const json_id_t* ids, const token_t* token, const uint32_t num, const char* src)
{
	uint8_t depth = 0;
	for (uint32_t i = 1; i < num; ++i)
	{
		json_id_t id = ids[i];
		if (id & closing_id && depth == 0)
		{
			return i;
		}
		#define X  \
				uint32_t line = 1; \
				uint32_t col = 1; \
				for (int j = 0; j <= token->pos; ++i) \
				{ \
					uint8_t newline_flag = src[j] == '\n'; \
					col = (1 - newline_flag) * col + 1; \
					line += newline_flag; \
				} \
				fprintf(stderr, "[JSON] ERROR: failed to find closing '%c' within search tokens!\n%d:%d %c is unpaired!\n", \
						closing_id == JSON_OBJECT_END ? '}' : ']', line, col, token->string[0]); \
				return 0;
		MATCH_DEPTH
		#undef X
	}
	return 0;
}

static uint32_t find_end_quiet(const json_id_t closing_id, const json_id_t* ids, const uint32_t num)
{
	uint8_t depth = 0;
	for (uint32_t i = 1; i < num; ++i)
	{
		json_id_t id = ids[i];
		if (id & closing_id && depth == 0)
		{
			return i;
		}
		else if (id & JSON_OBJECT_START)
		{
			++depth;
		}
		#define X return 0;
		MATCH_DEPTH
		#undef X
	}
	return 0;
}

static uint16_t analyze_object(json_id_t* ids, const token_t* tokens, const uint32_t num, const char* src)
{
	json_id_t expected = JSON_STRING; // NOTE: never expect close object because you check for the proper closing at start
	/* printf("object: "); */
	/* for (uint32_t i = 1; i < num - 1; ++i) */
	/* { */
	/* 	printf("%s ", tokens[i].string); */
	/* } */
	/* printf("\n"); */
	uint32_t increment;
	for (uint32_t i = 1; i < num - 1; i += increment)
	{
		json_id_t id = ids[i];
		if (id & expected)
		{
			increment = 1;
			if (id & JSON_STRING && expected != JSON_ALL_VALUES)
			{
				expected = JSON_NAME_SEP;
				ids[i] |= JSON_KEY;
			}
			else if (id & JSON_NAME_SEP)
			{
				expected = JSON_ALL_VALUES;
			}
			else if (id & JSON_ALL_VALUES)
			{
				uint32_t offset = 0;
				ids[i] |= JSON_VALUE;
				switch (id & JSON_ALL_VALUES)
				{
				case JSON_OBJECT:
					if ((offset = find_end(JSON_OBJECT_END, ids + i, tokens + i, num - i, src)) == 0)
					{
						return JSON_ERROR;
					}
					else if (JSON_ERROR & analyze_object(ids + i, tokens + i, offset + 1, src))
					{
						return JSON_ERROR;
					}
					else {
						increment = offset + 1;
						expected = JSON_VALUE_SEP;
					}
					break;
				case JSON_ARRAY:
					if ((offset = find_end(JSON_ARRAY_END, ids + i, tokens + i, num - i, src)) == 0)
					{
						return JSON_ERROR;
					}
					else if (JSON_ERROR & analyze_array(ids + i, tokens + i, offset + 1, src))
					{
						return JSON_ERROR;
					}
					else {
						increment = offset + 1;
						expected = JSON_VALUE_SEP;
					}
					break;
				default:
					expected = JSON_VALUE_SEP;
				}
			}
			else if (id & JSON_VALUE_SEP)
			{
				expected = JSON_STRING;
			}
			else if (id & JSON_OBJECT_START)
			{
				expected = JSON_STRING;
			}
		}
		else
		{
			uint32_t line = 1;
			uint32_t col = 1;
			for (uint32_t j = 0; j < tokens[i].pos; ++j)
			{
				const uint8_t newline_flag = src[j] == '\n';
				col = col * (1 - newline_flag) + 1;
				line += newline_flag;
			}
			char* str = alloca(tokens[i].len + 1);
			memcpy(str, tokens[i].string, tokens[i].len);
			str[tokens[i].len] = '\0';
			fprintf(stderr, "[JSON] ERROR: Unexpcted token in object!\n%d:%d %s\n", line, col, str);
			for (uint32_t j = i; j < num - 1; j++)
			{
				ids[i] = JSON_ERROR;
			}
			return JSON_ERROR;
		}
	}
	if (expected & JSON_VALUE_SEP || num == 2)
	{
		return  JSON_SUCCESS | JSON_OBJECT;
	}
	else
	{
		const token_t t  = tokens[num - 1];
		uint32_t line = 1;
		uint32_t col = 1;
		for (uint32_t i = 0; i < t.pos; ++i)
		{
			const uint8_t newline_flag = src[i] == '\n';
			col = col * (1 - newline_flag) + 1;
			line += newline_flag;
		}
		char* str = alloca(t.len + 1);
		memcpy(str, t.string, t.len);
		str[t.len] = '\0';
		fprintf(stderr, "[JSON] ERROR: Unexpected token in object! Expected: '}'!\n%d:%d %s\n", line, col, str);
		ids[num - 1] = JSON_ERROR;
		return JSON_ERROR;
	}
}

static uint16_t analyze_array(json_id_t* ids, const token_t* tokens, const uint32_t num, const char* src)
{
	json_id_t expected = JSON_ALL_VALUES; // NOTE: never expect close array because you check for the proper closing at start
	/* printf("array: "); */
	/* for (uint32_t i = 1; i < num - 1; ++i) */
	/* { */
	/* 	printf("%s ", tokens[i].string); */
	/* } */
	/* printf("\n"); */
	uint32_t increment;
	for (uint32_t i = 1; i < num - 1; i += increment)
	{
		json_id_t id = ids[i];
		if (id & expected)
		{
			increment = 1;
			uint32_t offset = 0;
			ids[i] |= JSON_VALUE;
			switch (id & expected)
			{
			case JSON_OBJECT:
				if ((offset = find_end(JSON_OBJECT_END, ids + i, tokens + i, num - i, src)) == 0)
				{
					return JSON_ERROR;
				}
				else if (JSON_ERROR & analyze_object(ids + i, tokens + i, offset + 1, src))
				{
					return JSON_ERROR;
				}
				else {
					increment = offset + 1;
					expected = JSON_VALUE_SEP;
				}
				break;
			case JSON_ARRAY:
				if ((offset = find_end(JSON_ARRAY_END, ids + i, tokens + i, num - i, src)) == 0)
				{
					return JSON_ERROR;
				}
				else if (JSON_ERROR & analyze_array(ids + i, tokens + i, offset + 1, src))
				{
					return JSON_ERROR;
				}
				else {
					increment = offset + 1;
					expected = JSON_VALUE_SEP;
				}
				break;
			case JSON_VALUE_SEP:
				expected = JSON_ALL_VALUES;
				break;
			default:
				expected = JSON_VALUE_SEP;
			}
		}
		else
		{
			uint32_t line = 1;
			uint32_t col = 1;
			for (uint32_t j = 0; j < tokens[i].pos; ++j)
			{
				const uint8_t newline_flag = src[j] == '\n';
				col = col * (1 - newline_flag) + 1;
				line += newline_flag;
			}
			char* str = alloca(tokens[i].len + 1);
			memcpy(str, tokens[i].string, tokens[i].len);
			str[tokens[i].len] = '\0';
			fprintf(stderr, "[JSON] ERROR: Unexpected token in array!\n%d:%d %s\n", line, col, str);
			for (uint32_t j = i; j < num - 1; j++)
			{
				ids[i] = JSON_ERROR;
			}
			return JSON_ERROR;
		}
	}
	if (expected & JSON_VALUE_SEP || num == 2)
	{
		return  JSON_SUCCESS | JSON_ARRAY;
	}
	else
	{
		const token_t t  = tokens[num - 1];
		uint32_t line = 1;
		uint32_t col = 1;
		for (uint32_t i = 0; i < t.pos; ++i)
		{
			const uint8_t newline_flag = src[i] == '\n';
			col = col * (1 - newline_flag) + 1;
			line += newline_flag;
		}
		char* str = alloca(t.len + 1);
		memcpy(str, t.string, t.len);
		str[t.len] = '\0';
		fprintf(stderr, "[JSON] ERROR: Unexpected token in array! Expected ']'!\n%d:%d %s\n", line, col, t.string);
		ids[num - 1] = JSON_ERROR;
		return JSON_ERROR;
	}
}


static json_id_t json_syntax_analysis(json_id_t* ids, const token_t* tokens, const uint32_t num_tokens, const char* src)
{
	if (ids == NULL || tokens == NULL) return JSON_ERROR;
	if (ids[0] & JSON_NESTED_START)
	{
		if (find_end(ids[0] << 1, ids, tokens, num_tokens, src) > 0)
		{
			switch (ids[0] & JSON_OBJECT)
			{
			case JSON_OBJECT_START:
				return analyze_object(ids, tokens, num_tokens, src);
			default:
				return  analyze_array(ids, tokens, num_tokens, src);
			}
		}
		else
		{
			return JSON_ERROR;
		}
	}
	else
	{
		uint32_t line = 1;
		uint32_t col = 1;
		for (uint32_t i = 0; i < tokens->pos; ++i)
		{
			const uint8_t newline_flag = src[i] == '\n';
			col = col * (1 - newline_flag) + 1;
			line += newline_flag;
		}
		char* str = alloca(tokens->len + 1);
		memcpy(str, tokens->string, tokens->len);
		str[tokens->len] = '\0';
		fprintf(stderr, "[JSON] ERROR: JSON must either be an array or an object!\n%d:%d %s", line, col, str);
		return JSON_ERROR;
	}
}

static json_id_t subtokens_at(json_t* dest, const json_t* src, const uint32_t pos)
{
	json_id_t id = src->ids[pos];
	dest->pos = 0;
	if (id & JSON_SINGLE_TOKEN_VALUE)
	{
		dest->tokens = src->tokens + pos;
		dest->ids = src->ids + pos;
		dest->num_tokens = 1;
		return JSON_SUCCESS | id;
	}
	else if (id & JSON_NESTED_START)
	{
		uint32_t n = find_end_quiet(id << 1, src->ids + pos, src->num_tokens - pos - 1);
		if (n > 0)
		{
			dest->tokens = src->tokens + pos;
			dest->num_tokens = n + 1;
			return JSON_SUCCESS | id;
		}
	}
	dest->num_tokens = 0;
	dest->tokens = NULL;
	return JSON_ERROR;
}

json_id_t json_next_key(json_t* json, char** key)
{
	uint32_t depth = 0;
	if (json->pos == 0) json->pos = 1;
	for (uint32_t i = json->pos; i < json->num_tokens - 1; ++i) {
		json_id_t id = json->ids[i];
		if (id & JSON_KEY && depth == 0)
		{
			*key = json->tokens[i];
			json->pos = i + 1;
			return JSON_SUCCESS | id;
		}
		#define X json->pos = json->num_tokens; return json->ids[0] << 1; // BIG BRAIN
		MATCH_DEPTH
		#undef X
	}
	return JSON_ERROR;
}

json_id_t json_next_value(json_t* json, json_t* value)
{
	uint32_t depth = 0;
	if (json->pos == 0) json->pos = 1;
	for (uint32_t i = json->pos; i < json->num_tokens - 1; ++i)
	{
		json_id_t id = json->ids[i];
		if (id & JSON_VALUE && depth == 0)
		{
			json_id_t res = subtokens_at(value, json, i);
			if (res)
			{
				json->pos += value->num_tokens + 1;
				return JSON_SUCCESS | res;
			}
		}
		#define X json->pos = json->num_tokens; return json->ids[0] << 1; // BIG BRAIN
		MATCH_DEPTH
		#undef X
	}
	return JSON_ERROR;
}

uint8_t json_c(void* dest, const json_t* json)
{
	if (json->num_tokens == 1)
	{
		// NOTE: do NOT handle JSON_NULL--you already know its value via json_value
		switch(json->ids[0] & JSON_SINGLE_TOKEN_VALUE)
		{
		case JSON_INTEGER:
			{
				int* p = dest;
				*p = atoi(json->tokens[0]);
			}
			break;
		case JSON_FLOAT:
			{
				float* p = dest;
				*p = atof(json->tokens[0]);
			}
			break;
		case JSON_STRING:
			{
				char** p = dest;
				*p = json->tokens[0];
			}
			break;
		case JSON_BOOL:
			{
				unsigned char* p = dest;
				*p = strcmp(json->tokens[0], "false"); // strcmp
			}
			break;
		}
		return 0x01;
	}
	else
	{
		return 0x00;
	}
}

/* void json_free(json_t* json) */
/* { */
/* 	free(json->tokens); */
/* 	free(json->ids); // NOTE: remember the _actual_ token strings & ids are allocated in the same block as json->ids */
/* } */


json_id_t json_parse(json_t* json, arena_t* arena, const char* src, const uint32_t len)
{
	if (json == NULL || src == NULL) return JSON_ERROR;
	token_t* tokens = NULL;
	json_id_t* ids = NULL;
	const uint32_t num_tokens = tokenize(&tokens, &ids, arena, src, len);
	if (num_tokens == 0)
	{
		return JSON_ERROR;
	}
	json_id_t res = json_syntax_analysis(ids, tokens, num_tokens, src);
	if (res & JSON_SUCCESS)
	{
		/* json->tokens = malloc(num_tokens * sizeof *json->tokens); */
		json->tokens = tokens; // DEVILISH
		for (uint32_t i = 1; i < num_tokens; ++i)
		{
			json->tokens[i] = tokens[i].string;
		}
		json->ids = ids;
		json->num_tokens = num_tokens;
		json->pos = 0;
		/* free(tokens); */
		return res;
	}
	else
	{
		// NOTE: handle arena cleanup (if desired) in error handling
		/* free(tokens); */
		/* free(ids); NOTE: same allocation as tokens */
		return JSON_ERROR;
	}
}

int json_test(int argc, char** argv)
{
	printf(" > testing json tokenizing\n");
	char* foo = "{\"lmao\":true,\"lol\":[0,-1.022,null,{\"bruh\":\"L, loser\"},false],\"OwO\":\"UwU\"}";
	SCRATCH* dest = scratch_checkout();
	#define JS_ARENA dest
	JS_OBJECT(
		JS_MEMBER(lmao, true, COMMA)
		JS_MEMBER_MACRO(lol, JS_ARRAY(
							JS_VAL(0, COMMA)
							JS_VAL(-1.022, COMMA)
							JS_VAL(null, COMMA)
							JS_OBJECT(JS_MEMBER(bruh, "L, loser"), COMMA)
							JS_VAL(false)),
						COMMA)
		JS_MEMBER(OwO, "UwU"))
		char* bar = JS_DUMP;
	assert(strncmp(foo, bar, strlen(bar)) == 0);
	printf("foo: %s\n", foo);
	printf("bar: %s\n", bar);
	SCRATCH* json_arena = scratch_checkout();
	json_t json = {0};
	json_id_t p_res = json_parse(&json, json_arena, bar, dest->size - 1);
	printf("parse_result: %s\n", p_res & JSON_SUCCESS ? "SUCCESS!" : "error");
	if (p_res & JSON_SUCCESS)
	{
		if (p_res & JSON_OBJECT)
		{
			char* key;
			json_t tmp = {0};
			while ((p_res = json_next_key(&json, &key)) & JSON_KEY)
			{
				p_res = json_next_value(&json, &tmp);
				printf("%s: %#06x\n", key, p_res);
				switch (p_res & JSON_SINGLE_TOKEN_VALUE)
				{
					case JSON_BOOL:
						{
							uint8_t b;
							json_c(&b, &tmp);
							printf("bool: %s\n", b ? "true" : "false");
						}
						break;
				case JSON_STRING:
					{
						char* str;
						json_c(&str, &tmp);
						printf("string: %s\n", str);
					}
					break;
				}
				/* printf("value: %#06x", json_value(&json, &tmp)); */
			}
		}
	}
	/* json_free(&json); */
	return 0;
}
