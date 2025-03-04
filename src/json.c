#include "json.h"
#include <ctype.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "file_sys.h"
#include "arena.h"

#define JSON_CHARS \
	JSON_X('{') \
	JSON_X('}') \
	JSON_X('[') \
	JSON_X(']') \
	JSON_X(',') \
	JSON_X(':')

int8_t json_lex(char* src, json_t* json, const int n)
{
	if (src == NULL || json == NULL) return JSON_ERROR;
	arena_t a = {0};
	arena_init(&a, 2 * n);
	enum
	{
		NONE = 0x00,
		TOKEN = 0x01,
		STRING = 0x02,
	} lex_flag = TOKEN;
	uint32_t pos = 0;
	json_tokens_t* token = NULL;
	for (int i = 0; i < n; ++i)
	{
		char c = src[i];
		switch (lex_flag)
		{
		case STRING:
			if (c == '"' && src[i - 1] !='\\')
			{
				lex_flag = TOKEN;
				token = arena_alloc(&a, sizeof *token);
				int len = i - pos - 1;
				token->value = arena_alloc(&a, len + 1);
				token->value = memcpy(&token->value[0], &src[pos], len);
				token->value[len] = '\0';
				token->list.next = NULL;
				token = token->list.next;
			}
			break;
		case TOKEN:
			lex_flag = isspace(c) ? NONE : TOKEN;
			#define JSON_X(A) lex_flag &= c == A ? NONE : TOKEN;
			JSON_CHARS
			#undef JSON_X
			if (lex_flag == TOKEN) continue;
			token = arena_alloc(&a, sizeof *token);
			token->value = arena_alloc(&a, i - pos);
			token->value = memcpy(&token->value[0], &src[pos], i - pos - 1)
			if (isspace(c)) continue;
		case NONE:
			if (isspace(c)) continue;
			uint8_t json_flag = 0;
			#define JSON_X(A) json_flag |= c == A;
			JSON_CHARS
			#undef JSON_X
			if (json_flag)
			{
				token = arena_alloc(&a, sizeof *token);
				token->value = arena_alloc(&a, 2);
				token->value[0] = c;
				token->value[1] = '\0';
				token->list.next = NULL;
				token = token->list.next;
			}
			else if (c == '"')
			{
				lex_flag = STRING;
				pos = i + 1;
			}
			else
			{
				lex_flag = TOKEN;
				pos = i;
			}
		}
	}
	if (lex_flag != NONE) return JSON_ERROR;
}


int8_t json_parse_filep(FILE* fp, json_t* json)
{
	int len;
	char* src = fs_read_stream(fp, &len);
	return json_parse_str(src, json, len);
}

int8_t json_parse_str(char* src, json_t* json, const int n)
{
	if (src == NULL || n < 2)
	{
		return JSON_ERROR;
	}
	json->src = src;
	json->length = n;
	json->offset = 0;
	json->depth = 0;
	switch (src[0])
	{
	case '{':
		json->offset = 1;
		return JSON_OBJECT;
	case '[':
		json->offset = 1;
		return JSON_ARRAY;
	default:
		return JSON_ERROR;
	}
}

int8_t next_key(json_t* json, char* buffer)
{
	uint32_t key_start = 0;
	for (uint32_t i = json->offset; i < json->length; ++i)
	{
		char c = json->src[i];
		if (isspace(c))
		{
			continue;
		}
		switch (c)
		{
			case '"':
			if (json->src[i - 1] != '\\')
			{
				if (key_start)
				{
					memcpy(buffer, )
					return JSON_PARSE_SUCCESS;
				}
				else
				{
					key_start = i + 1;
				}
			}
			break;
		}
	}
	return JSON_ERROR;
}


int8_t next_elem(json_t* json)
{
	for (uint32_t i = json->offset; i < json->length; ++i)
	{
		char c = json->src[i];
		if (isspace(c))
		{
			continue;
		}
		switch (c)
		{
		case ',':
			continue;
		case '{':
			json->offset = i;
			return JSON_OBJECT;
		case '[':
			json->offset = i;
			return JSON_ARRAY;
		case ']':
			json->offset = i;
			return JSON_END_ARRAY;
		case 'n':
			if (strncmp(&json->src[0], "null", 4) == 0) ;
			break;
		case 't':
			if (strncmp(&json->src[0], "true", 4) == 0) ;
			break;
		case 'f':
			if (strncmp(&json->src[0], "false", 5) == 0) ;
			break;
		}
	}
}
