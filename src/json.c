#include "json.h"
#include <ctype.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "file_sys.h"
#include "token.h"
#include "arena.h"

struct token_ll
{
	struct token_ll *next;
	token_t* token;
};

static uint16_t id_token(const token_t* token);

static struct token_ll* new_token(arena_t* arena, const char* src, const uint32_t n, const int line, const int col);

static uint32_t tokenize(token_t** tokens, json_id_t** ids, const char* src, const int n);

static uint32_t find_end(const json_id_t closing_id, const json_id_t* ids, const token_t* token, const uint32_t num);

static uint32_t find_end_quiet(const json_id_t closing_id, const json_id_t* ids, const uint32_t num);

static uint16_t analyze_object(json_id_t* ids, const token_t* tokens, const uint32_t num);

static uint16_t analyze_array(json_id_t* ids, const token_t* tokens, const uint32_t num);

#define JSON_CHARS(NEXT) \
	JSON_CHAR('{') NEXT  \
	JSON_CHAR('}') NEXT  \
	JSON_CHAR('[') NEXT  \
	JSON_CHAR(']') NEXT  \
	JSON_CHAR(',') NEXT  \
	JSON_CHAR(':')

#define DIGITS(NEXT)  \
	DIGIT_X('0') NEXT \
	DIGIT_X('1') NEXT \
	DIGIT_X('2') NEXT \
	DIGIT_X('3') NEXT \
	DIGIT_X('4') NEXT \
	DIGIT_X('5') NEXT \
	DIGIT_X('6') NEXT \
	DIGIT_X('7') NEXT \
	DIGIT_X('8') NEXT \
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
					DEPTH_X \
			} \
			while(0); \
		} \
		else \
		{ \
			--depth; \
		} \
	}


static struct token_ll* new_token(arena_t* arena, const char* src, const uint32_t n, const int line, const int col)
{
	struct token_ll* res = arena_alloc(arena, sizeof *res);
	res->next = NULL;
	token_t* token = arena_alloc(arena, sizeof *token);
	token->line = line;
	token->col = col - n;
	char* str = arena_alloc(arena, n + 1);
	memcpy(str, src, n);
	str[n] = '\0';
	token->string = str;
	res->token = token;
	return res;
}

static uint32_t tokenize(token_t** tokens, json_id_t** ids, const char* src, const int n)
{
	if (src == NULL || *tokens != NULL) return 0;
	arena_t a = { 0 };
	// TODO: better upper bound for how much space the arena will need
	arena_init(&a, 16 * n); // NOTE: if segfault just give it more memory why not fuck it we ball
	enum {
		CHAR_TOKEN = 0x00,
		BASIC_TOKEN,
		STRING_TOKEN
	} token_flag = CHAR_TOKEN;
	uint32_t pos = 0;
	uint32_t num_tokens = 0;
	uint32_t num_chars = 0;
	struct token_ll** token = &a.memory; //  What do I use this for? PROPER LINKAGE
	uint32_t line = 1;
	uint32_t col = 1;
	#define JSON_CHAR(C) c == C
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
				*token = new_token(&a, src + pos, len, line, col);
				token = &(*token)->next;
				num_tokens += 1;
				/* num_chars += len + 1; */
				num_chars += len - 1; // NOTE: doing this to exclude the quotes around strings
				token_flag = CHAR_TOKEN;
            }
            break;
		case BASIC_TOKEN:
			if(isspace(c) || JSON_CHARS(||))
			{
				int len = i - pos;
				*token = new_token(&a, src + pos, len, line, col);
				token = &(*token)->next;
				num_tokens += 1;
				num_chars += len + 1;
				token_flag = CHAR_TOKEN;
				__attribute__((fallthrough));
			}
			else {
				break;
			}
	   default:
		   if (JSON_CHARS(||))
		   {
				*token = new_token(&a, src + i, 1, line, col);
				token = &(*token)->next;
				num_tokens += 1;
				num_chars += 2;
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
	arena_t token_arena =
	{
		.memory = malloc(num_tokens * sizeof **tokens),
		.cap = num_tokens * sizeof **tokens
	};
	uint8_t* addr = malloc(num_chars + num_tokens * sizeof **ids);
	arena_t id_arena =
	{
		.memory = addr,
		.cap = num_tokens * sizeof(json_id_t)
	};
	arena_t char_arena =
	{
		.memory = id_arena.memory + id_arena.cap,
		.cap = num_chars
	};
	for (struct token_ll* t = a.memory /* DOPAMINE DOPAMINE DOPAMINE */; t != NULL; t = t->next)
	{
		token_t* t_tok = t->token;
		token_t* token = arena_alloc(&token_arena, sizeof *token);
		json_id_t* id = arena_alloc(&id_arena, sizeof *id);
		*id = id_token(t_tok);
		token->col = t_tok->col;
		token->line = t_tok->line;
		int n = strlen(t_tok->string) + 1;
		if (*id == JSON_STRING)
		{
			token->string = arena_alloc(&char_arena, n - 2);
			memcpy(token->string, t_tok->string + 1, n - 2);
			token->string[n - 3] = '\0';
		}
		else
		{
			token->string = arena_alloc(&char_arena, n);
			memcpy(token->string, t_tok->string, n);
		}
	}
	free(a.memory);
	*tokens = token_arena.memory;
	*ids = id_arena.memory;
	return num_tokens;
}

static uint16_t id_token(const token_t* token)
{
	char* str = token->string;
	switch (str[0])
	{
	#define JSON_X(C, A) case C: return A;
	JSON_STRUCTURE_LIST
	#undef JSON_X
	case '-': __attribute__((fallthrough));
	#define DIGIT_X(C) case C:
	DIGITS(__attribute__((fallthrough));)
	#undef DIGIT_X
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
						fprintf(stderr, "[JSON] ERROR: Numbers can only have a single decimal point!%d:%d %s\n", token->line, token->col + i, str);
						return JSON_ERROR;
					}
				}
				else if (str[i] == 'e' || str[i] == 'E')
				{
					fprintf(stderr, "[NERD] LMAO: This parser doesn't support scientific notation, LOSER!\n%d:%d %s\n", token->line, token->col + i, str);
					return JSON_ERROR;
				}
				#define DIGIT_X(C) str[i] != C
				else if(DIGITS(&&))
				{
					fprintf(stderr, "[JSON] ERROR: Numbers can only contain digits and a single decimal point!\n%d:%d %s\n", token->line, token->col + i, str);
					return JSON_ERROR;
				}
				#undef DIGIT_X
			}
			return int_flag ? JSON_INTEGER : JSON_FLOAT;
		}
	#define JSON_X(STRING, VALUE) \
		if (strcmp(str, STRING) == 0) return VALUE; \
		else										\
		{ \
			fprintf(stderr, "[JSON] ERROR: Unacceptable literal name, did you mean " #STRING "?\n%d:%d %s\n", token->line, token->col, str); \
		} \
		break;
	case 'n':
		JSON_X("null", JSON_NULL);
	case 't':
		JSON_X("true", JSON_TRUE);
	case 'f':
		JSON_X("false", JSON_FALSE);
	#undef JSON_X
	case '"':
	{
		int n = strlen(str);
		for (int i = 1; i < n - 1; ++i)
		{
			if (str[i] == '"' && str[i - 1] != '\\')
			{
				fprintf(stderr, "[JSON] ERROR: Closing quotes before final character in string!\n%d:%d %s\n", token->line, token->col + i, str);
				return JSON_ERROR;
			}
		}
		if (str[n - 1] != '"')
		{
				fprintf(stderr, "[JSON] ERROR: A String must close with quotes!\n%d:%d %s\n", token->line, token->col + n, str);
				return JSON_ERROR;
		}
		return JSON_STRING;
	}
	default:
		fprintf(stderr, "[JSON] ERROR: Bad token!\n%d:%d %s\n", token->line, token->col, str);
	}
	return JSON_ERROR;
}

static uint32_t find_end(const json_id_t closing_id, const json_id_t* ids, const token_t* token, const uint32_t num)
{
	uint8_t depth = 0;
	for (uint32_t i = 1; i < num; ++i)
	{
		json_id_t id = ids[i];
		if (id & closing_id && depth == 0)
		{
			return i;
		}
		#define DEPTH_X  \
				fprintf(stderr, "[JSON] ERROR: failed to find closing '%c' within search tokens!\n%d:%d %s is unpaired!\n", \
						closing_id == JSON_OBJECT_END ? '}' : ']', token->line, token->col, token->string); \
				return 0;
		MATCH_DEPTH
		#undef DEPTH_X
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
		#define DEPTH_X return 0;
		MATCH_DEPTH
		#undef DEPTH_X
	}
	return 0;
}

static uint16_t analyze_object(json_id_t* ids, const token_t* tokens, const uint32_t num)
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
					if ((offset = find_end(JSON_OBJECT_END, ids + i, tokens + i, num - i)) == 0)
					{
						return JSON_ERROR;
					}
					else if (JSON_ERROR & analyze_object(ids + i, tokens + i, offset + 1))
					{
						return JSON_ERROR;
					}
					else {
						increment = offset + 1;
						expected = JSON_VALUE_SEP;
					}
					break;
				case JSON_ARRAY:
					if ((offset = find_end(JSON_ARRAY_END, ids + i, tokens + i, num - i)) == 0)
					{
						return JSON_ERROR;
					}
					else if (JSON_ERROR & analyze_array(ids + i, tokens + i, offset + 1))
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
			fprintf(stderr, "[JSON] ERROR: Unexpcted token in object!\n%d:%d %s\n", tokens[i].line, tokens[i].col, tokens[i].string);
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
		fprintf(stderr, "[JSON] ERROR: Unexpected token in object! Expected: '}'!\n%d:%d %s\n", t.line, t.col, t.string);
		ids[num - 1] = JSON_ERROR;
		return JSON_ERROR;
	}
}

static uint16_t analyze_array(json_id_t* ids, const token_t* tokens, const uint32_t num)
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
				if ((offset = find_end(JSON_OBJECT_END, ids + i, tokens + i, num - i)) == 0)
				{
					return JSON_ERROR;
				}
				else if (JSON_ERROR & analyze_object(ids + i, tokens + i, offset + 1))
				{
					return JSON_ERROR;
				}
				else {
					increment = offset + 1;
					expected = JSON_VALUE_SEP;
				}
				break;
			case JSON_ARRAY:
				if ((offset = find_end(JSON_ARRAY_END, ids + i, tokens + i, num - i)) == 0)
				{
					return JSON_ERROR;
				}
				else if (JSON_ERROR & analyze_array(ids + i, tokens + i, offset + 1))
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
			fprintf(stderr, "[JSON] ERROR: Unexpected token in array!\n%d:%d %s\n", tokens[i].line, tokens[i].col, tokens[i].string);
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
		fprintf(stderr, "[JSON] ERROR: Unexpected token in array! Expected ']'!\n%d:%d %s\n", t.line, t.col, t.string);
		ids[num - 1] = JSON_ERROR;
		return JSON_ERROR;
	}
}


static json_id_t json_syntax_analysis(json_id_t* ids, const token_t* tokens, const uint32_t num_tokens)
{
	if (ids == NULL || tokens == NULL) return JSON_ERROR;
	if (ids[0] & JSON_NESTED_START)
	{
		if (find_end(ids[0] << 1, ids, tokens, num_tokens) > 0)
		{
			switch (ids[0] & JSON_OBJECT)
			{
			case JSON_OBJECT_START:
				return analyze_object(ids, tokens, num_tokens);
			default:
				return  analyze_array(ids, tokens, num_tokens);
			}
		}
		else
		{
			return JSON_ERROR;
		}
	}
	else
	{
		fprintf(stderr, "[JSON] ERROR: JSON must either be an array or an object!\n%d:%d %s", tokens->line, tokens->col, tokens->string);
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
		#define DEPTH_X json->pos = json->num_tokens; return JSON_OBJECT_END;
		MATCH_DEPTH
		#undef DEPTH_X
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
		/* #define DEPTH_X return JSON_OBJECT_END; */
		#define DEPTH_X return json->ids[0] << 1; // BIG BRAIN
		MATCH_DEPTH
		#undef DEPTH_X
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

void json_free(json_t* json)
{
	free(json->tokens);
	free(json->ids); // NOTE: remember the _actual_ token strings & ids are allocated in the same block as json->ids
}


json_id_t json_parse(json_t* json, const char* src)
{
	if (json == NULL || src == NULL) return JSON_ERROR;
	token_t* tokens = NULL;
	json_id_t* ids = NULL;
	const uint32_t num_tokens = tokenize(&tokens, &ids, src, strlen(src));
	if (num_tokens == 0)
	{
		return JSON_ERROR;
	}
	json_id_t res = json_syntax_analysis(ids, tokens, num_tokens);
	if (res & JSON_SUCCESS)
	{
		json->tokens = malloc(num_tokens * sizeof *json->tokens);
		/* json->tokens = tokens; // DEVILISH */
		for (uint32_t i = 1; i < num_tokens; ++i)
		{
			json->tokens[i] = tokens[i].string;
		}
		json->ids = ids;
		json->num_tokens = num_tokens;
		json->pos = 0;
		free(tokens);
		return res;
	}
	else
	{
		free(tokens);
		free(ids);
		return JSON_ERROR;
	}
}

int json_test(int argc, char** argv)
{
	printf("testing json tokenizing\n");
	char* foo = "{ \"lmao\":true,\"lol\":[0,-1.022,null,{\"bruh\":\"L, loser\"},false], \"OwO\":\"UwU\"}";
	/* char* foo = "{ \"lmao\": true, \"lol\" :     null}"; */
	/* token_t* tokens = NULL; */
	/* json_id_t* ids = NULL; */
	/* uint32_t num_tokens = tokenize(&tokens, &ids, foo, strlen(foo)); */
	/* for (int i = 0; i < num_tokens; ++i) */
	/* { */
	/* 	printf("%d:%d:\t%#06x - %s\n", tokens[i].line, tokens[i].col, ids[i], tokens[i].string); */
	/* } */
	/* free(tokens); */
	json_t json = {0};
	json_id_t p_res = json_parse(&json, foo);
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
	json_free(&json);
	return 0;
}
