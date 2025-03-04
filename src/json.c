#include "json.h"
#include <ctype.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "tokens.h"
#include "file_sys.h"
#include "arena.h"

#define JSON_CHARS \
	JSON_CHAR('{') NEXT \
	JSON_CHAR('}') NEXT \
	JSON_CHAR('[') NEXT \
	JSON_CHAR(']') NEXT \
	JSON_CHAR(',') NEXT \
	JSON_CHAR(':')

static uint8_t tokenize(char* src, tokens_t** tokens, const int n)
{
	if (src == NULL || *tokens != NULL) return JSON_ERROR;
	arena_t a = { 0 };
	arena_init(&a, 2 * n); // NOTE: if segfault just give it more memory why not fuck it we ball
	enum {
		CHAR_TOKEN = 0x00,
		BASIC_TOKEN,
		STRING_TOKEN
	} token_flag = CHAR_TOKEN;
	uint32_t pos = 0;
#define JSON_CHAR(C) c == C
#define NEXT ||
	for (int i = 0; i < n; ++i)
	{
		char c = src[i];
		switch (token_flag)
		{
		case STRING_TOKEN:
			if (c == '"' && src[i - 1] != '\\')
			{
				int len = i - pos + 1;
				*tokens = arena_alloc(&a, sizeof **tokens);
				(*tokens)->token = arena_alloc(&a, len + 1);
				memcpy((*tokens)->token, &src[pos], len);
				(*tokens)->token[len] = '\0';
				(*tokens)->list.next = NULL;
				tokens = &(*tokens)->list.next;
				token_flag = CHAR_TOKEN;
            }
            break;
		case BASIC_TOKEN:
			if(isspace(c) || JSON_CHARS)
			{
				int len = i - pos;
				*tokens = arena_alloc(&a, sizeof **tokens);
				(*tokens)->token = arena_alloc(&a, len + 1);
				memcpy((*tokens)->token, &src[pos], len);
				(*tokens)->token[len] = '\0';
				(*tokens)->list.next = NULL;
				tokens = &(*tokens)->list.next;
				token_flag = CHAR_TOKEN;
			}
			__attribute__((fallthrough));
	   default:
			if (isspace(c))
			{
				continue;
			}
			else if (JSON_CHARS)
			{
				*tokens = arena_alloc(&a, sizeof **tokens);
				(*tokens)->token = arena_alloc(&a, 2);
				(*tokens)->token[0] = c;
				(*tokens)->token[1] = '\0';
				(*tokens)->list.next = NULL;
				tokens = &(*tokens)->list.next;
			}
			else
			{
				pos = i;
				token_flag = c == '"' ? STRING_TOKEN : BASIC_TOKEN;
			}
		}
	}
	if (token_flag != CHAR_TOKEN)
	{
		*tokens = NULL;
		return JSON_ERROR;
	}
#undef JSON_CHAR
#undef NEXT
	*tokens = a.memory; // DOPAMINE DOPAMNINE DOPAMINE
	return JSON_READY;
}

static uint8_t static_analysis(tokens_t* tokens)
{
	int32_t object_depth = 0;
	int32_t array_depth = 0;
	for(; tokens != NULL; tokens = tokens->list.next)
	{
	}
	return 0;
}

/* int8_t json_parse_filep(FILE* fp, json_t* json) */
/* { */
/* 	int len; */
/* 	char* src = fs_read_stream(fp, &len); */
/* 	return json_parse_str(src, json, len); */
/* } */

/* int8_t json_parse_str(char* src, json_t* json, const int n) */
/* { */
/* 	if (src == NULL || n < 2) */
/* 	{ */
/* 		return JSON_ERROR; */
/* 	} */
/* 	json->src = src; */
/* 	json->length = n; */
/* 	json->offset = 0; */
/* 	json->depth = 0; */
/* 	switch (src[0]) */
/* 	{ */
/* 	case '{': */
/* 		json->offset = 1; */
/* 		return JSON_OBJECT; */
/* 	case '[': */
/* 		json->offset = 1; */
/* 		return JSON_ARRAY; */
/* 	default: */
/* 		return JSON_ERROR; */
/* 	} */
/* } */

/* int8_t next_key(json_t* json, char* buffer) */
/* { */
/* 	uint32_t key_start = 0; */
/* 	for (uint32_t i = json->offset; i < json->length; ++i) */
/* 	{ */
/* 		char c = json->src[i]; */
/* 		if (isspace(c)) */
/* 		{ */
/* 			continue; */
/* 		} */
/* 		switch (c) */
/* 		{ */
/* 			case '"': */
/* 			if (json->src[i - 1] != '\\') */
/* 			{ */
/* 				if (key_start) */
/* 				{ */
/* 					memcpy(buffer, ) */
/* 					return JSON_PARSE_SUCCESS; */
/* 				} */
/* 				else */
/* 				{ */
/* 					key_start = i + 1; */
/* 				} */
/* 			} */
/* 			break; */
/* 		} */
/* 	} */
/* 	return JSON_ERROR; */
/* } */


/* int8_t next_elem(json_t* json) */
/* { */
/* 	for (uint32_t i = json->offset; i < json->length; ++i) */
/* 	{ */
/* 		char c = json->src[i]; */
/* 		if (isspace(c)) */
/* 		{ */
/* 			continue; */
/* 		} */
/* 		switch (c) */
/* 		{ */
/* 		case ',': */
/* 			continue; */
/* 		case '{': */
/* 			json->offset = i; */
/* 			return JSON_OBJECT; */
/* 		case '[': */
/* 			json->offset = i; */
/* 			return JSON_ARRAY; */
/* 		case ']': */
/* 			json->offset = i; */
/* 			return JSON_END_ARRAY; */
/* 		case 'n': */
/* 			if (strncmp(&json->src[0], "null", 4) == 0) ; */
/* 			break; */
/* 		case 't': */
/* 			if (strncmp(&json->src[0], "true", 4) == 0) ; */
/* 			break; */
/* 		case 'f': */
/* 			if (strncmp(&json->src[0], "false", 5) == 0) ; */
/* 			break; */
/* 		} */
/* 	} */
/* } */
