#include "perfect_json.h"
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include "allocators/arena.h"
#include "allocators/scratch.h"
#include "macro_stack.h"
#include "benchmark.h"
STACK(id, json_id_t)

#define WHITESPACE			\
	X(SPACE,			' ')	\
	X(TAB,				'\t')	\
	X(NEWLINE,			'\n')	\
	X(CARRIAGE_RETURN,	'\r')

static int32_t token_len(const json_t* json, const int32_t index)
{
	const uint8_t bitmask = json->bitmasks[index];
	if ((bitmask & 0x0f) == JSON_STRING) // strings start AFTER the opening dubquote
	{
		const char* src = json->src;
		const int32_t i0 = json->offsets[index];
		const int32_t i1 = json->offsets[index + 1];
		const int32_t delta =  1 + ((json->bitmasks[index + 1] & 0x0f) == JSON_STRING);
		char c0 = src[i0 - 1];
		for (int32_t i = i1 - delta; i >= 0; --i)
		{
			const char c = src[i];
			if (c == '"')
			{
				return i - i0;
			}
		}
		return -1;
	}
	else if (bitmask & JSON_NUMBER)
	{
		const char* src = json->src;
		const int32_t i0 = json->offsets[index];
		const int32_t i1 = json->offsets[index + 1];
		for (int32_t i = i1 - 1; i >= 0; --i)
		{
			const char c = src[i];
			if ('0' <= c && c  <= '9')
			{
				return i - i0 + 1;
			}
		}
		return -1;
	}
	else if ((bitmask & 0x0f) == JSON_TRUE || (bitmask & 0x0f) == JSON_NULL)
	{
		return 4;
	}
	else if ((bitmask & 0x0f) == JSON_FALSE)
	{
		return 5;
	}
	else if ((bitmask & 0x0f) < JSON_INTEGER)
	{
		return 1;
	}
	else
	{
		return -1;
	}
}

static void str_at(char** str, int32_t* str_len, const json_t* json, const int32_t index)
{
	*str = json->src + json->offsets[index];
	*str_len = token_len(json, index);
}

static void int_at(int32_t* dest, const json_t* json, const int32_t index)
{
	char* str = json->src + json->offsets[index];
	int32_t len = token_len(json, index);
	char* temp = alloca(len + 1);
	memcpy(temp, str, len);
	temp[len] = '\0';
	*dest = atoi(temp);
}

static void float_at(float* dest, const json_t* json, const int32_t index)
{
	char* str = json->src + json->offsets[index];
	int32_t len = token_len(json, index);
	char* temp = alloca(len + 1);
	memcpy(temp, str, len);
	temp[len] = '\0';
	*dest = atof(temp);
}

void pj_allman(const json_t* json)
{
	int32_t depth = 0;
	for (int32_t i = 0; i < json->num_tokens; ++i)
	{
		const uint8_t bitmask = json->bitmasks[i];
		for (int32_t d = 0; d < depth; ++d)
		{
			printf("  ");
		}
		if (bitmask & JSON_KEY)
		{
			char* key;
			int32_t len;
			str_at(&key, &len, json, i);
			printf("\"");
			for (int32_t l = 0; l < len; ++l)
			{
				printf("%c", key[l]);
			}
			printf("\": " );
		}
		else if ((bitmask & 0x0f) >= JSON_INTEGER)
		{
			switch(bitmask & 0x0f)
			{
			case JSON_TRUE: printf("TRVKE"); break;
			case JSON_FALSE: printf("false"); break;
			case JSON_NULL: printf("null"); break;
			case JSON_STRING:
				{
					char* str;
					int32_t len;
					str_at(&str, &len, json, i);
					printf("\"");
					for (int32_t l = 0; l < len; ++l)
					{
						printf("%c", str[l]);
					}
					printf("\"");
				}
				break;
			case JSON_INTEGER:
				{
					int32_t val;
					int_at(&val, json, i);
					printf("%d", val);
				}
				break;
			case JSON_FLOAT:
				{
					float val;
					float_at(&val, json, i);
					printf("%f", val);
				}
				break;
			}
			printf(",\n");
		}
		else if ((bitmask & 0x0f) == JSON_OBJECT_START || (bitmask & 0x0f) == JSON_ARRAY_START)
		{
			printf("%c\n", (bitmask & 0x0f) == JSON_OBJECT_START ? '{' : '[');
			++depth;
		}
		else if ((bitmask & 0x0f) == JSON_OBJECT_END || (bitmask & 0x0f) == JSON_ARRAY_END)
		{
			printf("\b\b%c\n", (bitmask & 0x0f) == JSON_OBJECT_END ? '}' : ']');
			--depth;
		}
		else
		{
			printf("%d: ERR!", i);
		}
	}
}

static int32_t tokenize(int32_t* offsets, uint8_t* bitmasks, const char* src, const int32_t len)
{
	int32_t num_tokens = 0;
	SCRATCH* scratch = scratch_checkout_size(len / 2 + 1);
	id_stack stack =
	{
		.allocation = scratch->allocation,
		.cap = len / 2 + 1
	};
	for (int32_t i = 0; i < len; ++i)
	{
		const char c = src[i];
		if (c == ',') // yeah commas are most likely lol lmao
		{
			/* offsets[num_tokens] = i; */
			/* bitmasks[num_tokens] = JSON_COMMA; */
			/* if (id_stack_peek(&stack) == JSON_COLON) */
			/* { */
			/* 	printf("colon pop: %d\n", i); */
			/* 	--stack.size; */
			/* } */
			stack.size -= id_stack_peek(&stack) == JSON_COLON;
			/* ++num_tokens; */
		}
		else if (c == ':')
		{
			/* offsets[num_tokens] = i; */
			/* bitmasks[num_tokens] = JSON_COLON; */
			id_stack_push(&stack, JSON_COLON);
			/* ++num_tokens; */
		}
		else if (c == '{' || c == '[')
		{
			offsets[num_tokens] = i;
			const json_id_t id = c == '{' ? JSON_OBJECT_START : JSON_ARRAY_START;
			bitmasks[num_tokens] = id | (i > 0 ? JSON_VALUE : 0x00);
			id_stack_push(&stack, id);
			++num_tokens;
		}
		else if (c == '}' || c == ']')
		{
			offsets[num_tokens] = i;
			const json_id_t id = c == '}' ? JSON_OBJECT_END : JSON_ARRAY_END;
			bitmasks[num_tokens] = id;
			--stack.size;
			++num_tokens;
		}
		else if (c == '"')
		{
			offsets[num_tokens] = i + 1;
			bitmasks[num_tokens] = JSON_STRING | (JSON_VALUE << (id_stack_peek(&stack) != JSON_COLON));
			++num_tokens;
			// FF to end of string, THANKS!
			char c0 = c;
			for (++i; i < len; ++i)
			{
				char c1 = src[i];
				if (c1 == '"' && c0 != '\\')
				{
					break;
				}
				c0 = c1;
			}
		}
		else if (c == ' ' || c == '\n' || c == '\t' || c == '\r')
		{
			// just skip over whitespace lmao
			for (++i; i < len; ++i)
			{
				const char c1 = src[i];
				if (!(c1 == ' ' || c1 == '\n' || c1 == '\t' || c1 == '\r'))
				{
					break;
				}
			}
			--i;
		}
		else
		{
			offsets[num_tokens] = i;
			// BUT WHAT IS IT?
			bitmasks[num_tokens] = JSON_VALUE;
			switch (c)
			{
			// WOE! ASSUMPTIONS BE UPON YE!
			case 't':
				bitmasks[num_tokens] |= JSON_TRUE | JSON_BOOLEAN;
				i += 3;
				break;
			case 'f':
				bitmasks[num_tokens] |= JSON_FALSE | JSON_BOOLEAN;
				i += 4;
				break;
			case 'n':
				i += 3;
				bitmasks[num_tokens] |= JSON_NULL;
				break;
			default:
				bitmasks[num_tokens] |= JSON_NUMBER;
				// IF IT AINT AN INT ITS A FLOAT
				// I dont fuck with scientific numbers
				int8_t float_flag = 0;
				for (; i < len; ++i)
				{
					const char c1 = src[i];
#define X(VAL, CHAR) c1 == CHAR ||
					if (JSON_STRUCTURE_LIST WHITESPACE 0)
					{
						break;
					}
					if (c1 == '.')
					{
						float_flag = 1;
						break;
					}
				}
				bitmasks[num_tokens] |= JSON_INTEGER + float_flag;
				// FF until ws or structure char
				for (; i < len; ++i)
				{
					const char c1 = src[i];
					if (JSON_STRUCTURE_LIST WHITESPACE 0)
					{
						break;
					}
#undef X
				}
				--i;
			}
			++num_tokens;
		}
	}
	return num_tokens;
}

json_id_t pj_parse(json_t* json, arena_t* arena, const char* src, const int32_t len)
{
	if (json == NULL || src == NULL || len < 2) return JSON_ERROR;
	arena_grow(arena, 5 * len);
	json->src = src;
	json->index = 0;
	int32_t* offsets = arena->allocation;
	uint8_t* bitmasks = offsets + len;
	const int32_t num_tokens = tokenize(offsets, bitmasks, src, len);
	if (num_tokens > 0)
	{
		memmove(offsets + num_tokens, bitmasks, num_tokens);
		json->offsets = offsets;
		json->bitmasks = offsets + num_tokens;
		json->num_tokens = num_tokens;
		return bitmasks[0] & (JSON_OBJECT_START | JSON_ARRAY_START);
	}
	else
	{
		json->num_tokens = 0;
		return JSON_ERROR;
	}
}

// return the value of the key
uint8_t pj_next_key(json_t* json, char** key, int32_t* key_len)
{
	int32_t depth = 0;
	json->index += json->index == 0;
	/* printf("next_key(): index0: %d, ", json->index); */
	const int32_t n = json->num_tokens;
	for (int32_t i = json->index; i < n; ++i)
	{
		const uint8_t bitmask = json->bitmasks[i];
		depth += (bitmask & 0x0f) == JSON_OBJECT_START || (bitmask & 0x0f) == JSON_ARRAY_START;
		depth -= (bitmask & 0x0f) == JSON_OBJECT_END || (bitmask & 0x0f) == JSON_ARRAY_END;
		/* printf("d: %d, ", depth); */
		if (bitmask & JSON_KEY && depth == 0)
		{
			*key = json->src + json->offsets[i];
			*key_len = token_len(json, i);
			json->index = i + 1;
			/* printf("index1: %d\n", json->index); */
			return json->bitmasks[i + 1];
		}
	}
	/* printf("END!\n"); */
	json->index = n - 1;
	return JSON_ERROR;
}

uint8_t pj_next_value(json_t* json, json_t* value)
{
	json->index += json->index == 0;
	/* printf("next_value(): index0: %d, ", json->index); */
	const int32_t n = json->num_tokens;
	for (int32_t i = json->index; i < n; ++i)
	{
		const uint8_t bitmask = json->bitmasks[i];
		if (bitmask & JSON_VALUE)
		{
			value->src = json->src;
			value->offsets = json->offsets + i;
			value->bitmasks = json->bitmasks + i;
			value->index = 0;
			if (!((bitmask & 0x0f) == JSON_OBJECT_START || (bitmask & 0x0f) == JSON_ARRAY_START))
			{
				value->num_tokens = 1;
			}
			else
			{
				int32_t depth = 1;
				const json_id_t closing_pair = (bitmask & 0x0f) + 1;
				const int32_t i0 = i;
				for (++i; i < json->num_tokens; ++i)
				{
					const uint8_t id = json->bitmasks[i] & 0x0f;
					depth += id == JSON_OBJECT_START || id == JSON_ARRAY_START;
					depth -= id == JSON_OBJECT_END || id == JSON_ARRAY_END;
					if (id == closing_pair && depth == 0)
					{
						value->num_tokens = i - i0 + 1;
						break;
					}
				}
			}
			json->index = i + 1;
			/* printf("index1: %d, ", json->index); */
			return bitmask & 0x3f;
		}
	}
	json->index = n - 1;
	return JSON_ERROR;
}

json_id_t pj_str(char** str, int32_t* len, const json_t* json)
{
	const int32_t i = json->index;
	if ((json->bitmasks[i] & 0x0f) == JSON_STRING)
	{
		*str = json->src + json->offsets[i];
		*len = token_len(json, i);
		return JSON_STRING;
	}
	else
	{
		return JSON_ERROR;
	}
}

json_id_t pj_int(int* dest, const json_t* json)
{
	const int32_t i = json->index;
	if ((json->bitmasks[i] & 0x0f) == JSON_INTEGER)
	{
		const int32_t len = token_len(json, i);
		char* temp = alloca(len + 1);
		memcpy(temp, json->src + json->offsets[i], len);
		temp[len] = '\0';
		*dest = atoi(temp);
		return JSON_INTEGER;
	}
	else
	{
		return JSON_ERROR;
	}
}

json_id_t pj_float(float* dest, const json_t* json)
{
	const int32_t i = json->index;
	if ((json->bitmasks[i] & 0x0f) == JSON_FLOAT)
	{
		const int32_t len = token_len(json, i);
		char* temp = alloca(len + 1);
		memcpy(temp, json->src + json->offsets[i], len);
		temp[len] = '\0';
		*dest = atof(temp);
		return JSON_FLOAT;
	}
	else
	{
		return JSON_ERROR;
	}
}

int perfect_json_test(int argc, char** argv)
{
	printf(" > testing perfect json tokenizing\n");
	char* foo = "{\"lmao\":true,\"heck\":5.69,\"lol\":[0,-1.022,null,{\"bruh\":\"L, loser\"},false],\"OwO\":\"UwU\",\"guh\":\"\"}";
	const int32_t foo_len = strlen(foo);
	SCRATCH* json_arena = scratch_checkout();
	json_t* json = alloca(sizeof *json);
	printf("test json:\n%s\n", foo);
	uint8_t p_res = pj_parse(json, json_arena, foo, foo_len);
	printf("parse_result: %s\n", p_res != JSON_ERROR ? "SUCCESS!" : "error");
	for (int32_t i = 0; i < json->num_tokens; ++i)
	{
		const int32_t len = token_len(json, i);
		const int32_t offset = json->offsets[i];
		printf("(%d,%#04x: %d): ", offset, json->bitmasks[i], len);
		for (int32_t j = 0; j < len; ++j)
		{
			printf("%c", foo[offset + j]);
		}
		printf("\n");
	}
	pj_allman(json);
	if (p_res)
	{
		int8_t res;
		char * key;
		int32_t k_len;
		json_t temp = {0};
		/* printf("offset: %d\n", json->offsets[json->index]); */
		while ((res = pj_next_key(json, &key, &k_len)))
		{
			/* printf("offset: %d\n", json->offsets[json->index]); */
			res = pj_next_value(json, &temp);
			/* printf("offset: %d\n", json->offsets[json->index]); */
			for (int32_t i = 0; i < k_len; ++i)
			{
				printf("%c", key[i]);
			}
			printf(": %#04x\n", res);
			switch (res & 0x0f)
			{
			case JSON_TRUE:
				printf("  bool: true\n");
				break;
			case JSON_FALSE:
				printf("  bool: false\n");
				break;
			case JSON_STRING:
				{
					char* str;
					int32_t len;
					pj_str(&str, &len, &temp);
					printf("  string: \"");
					for (int32_t i = 0; i < len; ++i)
					{
						printf("%c", str[i]);
					}
					printf("\"\n");
				}
				break;
			case JSON_FLOAT:
				printf(" float\n");
				break;
			case JSON_INTEGER:
				printf(" integer\n");
				break;
			case JSON_ARRAY:
				{
				}
				break;
			}
		}
	}
	return 1 - (p_res != JSON_ERROR);
}
