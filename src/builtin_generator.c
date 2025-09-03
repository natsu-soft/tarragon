#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "mem_sys.h"
#include "perfect_json.h"
#include "map32.h"
#include "allocators/arena.h"
#include "allocators/scratch.h"
#include "gdscript/gdscript_ids.h"
#include "benchmark.h"
// ok, this isnt funni anymore >:(
#define LOG_FILE_HERE
#include "logger.h"

#ifdef _WIN32
typedef struct _iobuf FILE; typedef unsigned long long size_t;
#else
typedef struct _IO_FILE FILE;
typedef unsigned long size_t;
#endif

FILE* log_fp;
static FILE* file;

static map32_t* op_map = NULL;
static arena_t* str_arena = NULL;

static void global_enum_values(json_t* json)
{
	char* str;
	int32_t len;
	json_t dest = { 0x00 };
	while (pj_next_key(&str, &len, json))
	{
		const uint8_t flags = pj_next_value(&dest, json);
		if (strncmp("name", str, len) == 0)
		{
			pj_str(&str, &len, &dest);
			fprintf(file, "\tENUM_VALUE(\"");
			for (int32_t i = 0; i < len; ++i)
			{
				fprintf(file, "%c", str[i]);
			}
			fprintf(file, "\", %d, ", len);
		}
		else if (strncmp("value", str, len) == 0)
		{
			int32_t val;
			pj_int(&val, &dest);
			fprintf(file, "%d, ", val);
		}
		else if (strncmp("description", str, len) == 0)
		{
			pj_str(&str, &len, &dest);
			fprintf(file, "\"");
			for (int32_t i = 0; i < len; ++i)
			{
				fprintf(file, "%c", str[i]);
			}
			fprintf(file, "\", %d)\n", len);
		}
	}
}

static void global_enums(json_t* json)
{
	char* str;
	int32_t len;
	json_t dest = { 0x00 };
	while (pj_next_key(&str, &len, json))
	{
		const uint8_t flags = pj_next_value(&dest, json);
		if (strncmp(str, "name", len) == 0 && (flags & 0x0f) == JSON_STRING)
		{
			char* name;
			pj_str(&name, &len, &dest);
			printf(" + ");
			fprintf(file, "GLOBAL_ENUM(\"");
			for (int32_t i = 0; i < len; ++i)
			{
				printf("%c", name[i]);
				fprintf(file, "%c", name[i]);
			}
			fprintf(file, "\", %d, ", len);
			printf("\n");
		}
		else if (strncmp(str, "is_bitfield", len) == 0 && flags & JSON_BOOLEAN)
		{
			fprintf(file, "%x)\n", (flags & 0x0f) == JSON_TRUE);
		}
		else if (strncmp(str, "values", len) == 0 && (flags & 0x0f) == JSON_ARRAY)
		{
			pj_mapc_array(global_enum_values, &dest);
		}
	}
}

static void function_params(json_t* json)
{
	char* str;
	int32_t len;
	json_t dest = { 0x00 };
	while (pj_next_key(&str, &len, json))
	{
		pj_next_value(&dest, json);
		if (strncmp("name", str, len) == 0)
		{
			pj_str(&str, &len, &dest);
			fprintf(file, "\tFUNC_PARAM(\"");
			for (int32_t i = 0; i < len; ++i)
			{
				fprintf(file, "%c", str[i]);
			}
			fprintf(file, "\", %d, ", len);
		}
		if (strncmp("type", str, len) == 0)
		{
			pj_str(&str, &len, &dest);
			if (strncmp("enum::", str, len) == 0)
			{
				str += 6;
				len -= 6;
			}
			fprintf(file, "\"");
			for (int32_t i = 0; i < len; ++i)
			{
				fprintf(file, "%c", str[i]);
			}
			fprintf(file, "\", %d)\n", len);
		}
	}
}

static void utility_functions(json_t* json)
{
	char* str;
	int32_t len;
	json_t dest = { 0x00 };
	while (pj_next_key(&str, &len, json))
	{
		pj_next_value(&dest, json);
		if (strncmp("name", str, len) == 0)
		{
			pj_str(&str, &len, &dest);
			fprintf(file, "UTILITY_FUNC(\"");
			printf(" + ");
			for (int32_t i = 0; i < len; ++i)
			{
				fprintf(file, "%c", str[i]);
				printf("%c", str[i]);
			}
			fprintf(file, "\", %d, ", len);
			printf("\n");
		}
		else if (strncmp("return_type", str, len) == 0)
		{
			pj_str(&str, &len, &dest);
			if (strncmp("enum::", str, len) == 0)
			{
				str += 6;
				len -= 6;
			}
			fprintf(file, "\"");
			for (int32_t i = 0; i < len; ++i)
			{
				fprintf(file, "%c", str[i]);
			}
			fprintf(file, "\", %d, ", len);
		}
		else if (strncmp("is_vararg", str, len) == 0)
		{
			fprintf(file, "%d)\n", (pj_next_value(&dest, json) & 0x0f) == JSON_TRUE);
		}
		else if (strncmp("arguments", str, len) == 0)
		{
			pj_mapc_array(function_params, &dest);
		}
		else if (strncmp("description", str, len) == 0)
		{
			fprintf(file, "\tFUNC_DESC(\"");
			pj_str(&str, &len, &dest);
			for (int32_t i = 0; i < len; ++i)
			{
				fprintf(file, "%c", str[i]);
			}
			fprintf(file, "\", %d)\n", len);
		}
	}
}

static void operators(json_t* json)
{
	char* str;
	int32_t len;
	json_t dest = { 0x00 };
	while (pj_next_key(&str, &len, json))
	{
		const uint8_t flags = pj_next_value(&dest, json);
		if (strncmp("name", str, len) == 0)
		{
			pj_str(&str, &len, &dest);
			if (strncmp("unary", str, 5) != 0)
			{
				const int32_t id = map32_fetch(op_map, str, len, str_arena->allocation);
				assert(id != -1 && "AAAAAAAAAAAAAAAAAA");
				fprintf(file, "\tOPERATOR(%d)\n", id);
			}
			else
			{
				fprintf(file, "\tOPERATOR(GDSCRIPT_OP_%s)\n", str[5] == '+' ? "IDENTITY" : "NEGATION");
			}
		}
		else if (strncmp("right_type", str, len) == 0)
		{
			pj_str(&str, &len, &dest);
			fprintf(file, "\t\tOPERATOR_RIGHT(GDSCRIPT_OP_%s)\n", str[5] == '+' ? "IDENTITY" : "NEGATION");
		}
		else if (strncmp("return_type", str, len) == 0)
		{
		}
		else if (strncmp("description", str, len) == 0)
		{
		}
	}
}

static void builtin_classes(json_t* json)
{
	char* str;
	int32_t len;
	json_t dest = { 0x00 };
	while (pj_next_key(&str, &len, json))
	{
		const uint8_t flags = pj_next_value(&dest, json);
		if (strncmp("name", str, len) == 0)
		{
			pj_str(&str, &len, &dest);
			fprintf(file, "CLASS(\"");
			printf(" + ");
			for (int32_t i = 0; i < len; ++i)
			{
				fprintf(file, "%c", str[i]);
				fprintf(stdout, "%c", str[i]);
			}
			fprintf(file, "\", %d, ", len);
			printf("\n");
		}
		else if (strncmp("is_keyed", str, len) == 0)
		{
			fprintf(file, "%d)\n", (flags & 0x0f) == JSON_TRUE);
		}
		// TODO: members
		else if (strncmp("operators", str, len) == 0)
		{
			pj_mapc_array(operators, &dest);
		}
	}
}

// if u free ur bitchmade
int main(int argc, char** argv)
{
	ms_handle_t handle;
	int32_t json_len;
	char* json_str = ms_map_entire_file("./extension_api.json", &json_len, &handle);
	arena_t* scratch = scratch_checkout();
	json_t json = { 0x00 };
	uint8_t res = pj_parse(&json, scratch, json_str, json_len);
	if (res & JSON_OBJECT)
	{
		const int32_t cap = 32;
		arena_t* op_scratch = scratch_checkout_size(32 * 4 * 10);
		str_arena = scratch_checkout_size(256);
		op_map = alloca(sizeof *op_map);
		op_map->rkeys = op_scratch->allocation;
		op_map->values = op_map->rkeys + 4 * cap;
		op_map->lens = op_map->values + 4 * cap;
		op_map->cap = 4 * cap;
		op_map->size = 0;
		memset(op_map->rkeys, 0x00, 4 * cap * sizeof op_map->rkeys);
		memset(op_map->lens, 0x00, 4 * cap * sizeof op_map->lens);
		char* op_str;
#define X(ID, STR, LEN) \
		op_str = arena_alloc(str_arena, LEN); \
		memcpy(op_str, STR, LEN); \
		map32_cache(op_map, op_str, LEN, GDSCRIPT_##ID, str_arena->allocation);
		GDSCRIPT_REGULAR_OPERATORS
#undef X
		map32_print(op_map, str_arena);
		return 0;
		file = fopen("./src/builtin.gen", "w");
		fprintf(file, "/* this file is generated in builtin_generator.c & relies upon extension_api.json */\n\n");
		json_t header_json = { 0x00 };
		json_t dest = { 0x00 };
		char* key;
		int32_t len;
		// cache class pass
		json.index = 0;
		while ((res = pj_next_key(&key, &len, &json)))
		{
			printf("> ");
			for (int32_t i = 0; i < len; ++i)
			{
				printf("%c", key[i]);
			}
			printf("\n");
			if (strncmp(key, "header", len) == 0 && pj_next_value(&header_json, &json) == JSON_OBJECT)
			{
				while ((res = pj_next_key(&key, &len, &header_json)))
				{
					printf("  - ");
					for (int32_t i = 0; i < len; ++i)
					{
						printf("%c", key[i]);
					}
					if (strncmp(key, "version_major", len) == 0 && pj_next_value(&dest, &header_json) == JSON_INTEGER)
					{
						int32_t val;
						pj_int(&val, &dest);
						fprintf(file, "SUPPORTED_VERSION(major, %d)\n", val);
					}
					else if (strncmp(key, "version_minor", len) == 0 && pj_next_value(&dest, &header_json) == JSON_INTEGER)
					{
						int32_t val;
						pj_int(&val, &dest);
						fprintf(file, "SUPPORTED_VERSION(minor, %d)\n", val);
					}
					else if (strncmp(key, "version_patch", len) == 0 && pj_next_value(&dest, &header_json) == JSON_INTEGER)
					{
						int32_t val;
						pj_int(&val, &dest);
						fprintf(file, "SUPPORTED_VERSION(patch, %d)\n", val);
					}
					printf("\n");
				}
			}
			else if (strncmp(key, "global_constants", len) == 0 && pj_next_value(&header_json, &json) == JSON_ARRAY)
			{
				printf(" + global_constants is an array\n");
				if (pj_next_value(&dest, &header_json) == JSON_ERROR)
				{
					printf(" + it is empty\n");
					fprintf(file, "\n/* there are no global constants */\n");
				}
				else
				{
					fprintf(stderr, "HOLY SHIT THERE ARE GLOBAL CONSTANTS??? PANIC! PANIC! PANIC!\n");
					fclose(file);
					return 1;
				}
			}
			else if (strncmp(key, "global_enums", len) == 0 && pj_next_value(&header_json, &json) == JSON_ARRAY)
			{
				fprintf(file, "\n/* global enums */\n");
				pj_mapc_array(global_enums, &header_json);
			}
			else if (strncmp(key, "utility_functions", len) == 0 && pj_next_value(&header_json, &json) == JSON_ARRAY)
			{
				fprintf(file, "\n/* utility functions */\n");
				pj_mapc_array(utility_functions, &header_json);
			}
			else if (strncmp(key, "builtin_classes", len) == 0 && pj_next_value(&header_json, &json) == JSON_ARRAY)
			{
				fprintf(file, "\n/* bultin classes (read: types) */\n");
				pj_mapc_array(builtin_classes, &header_json);
			}
		}
		fclose(file);
		return 0;
	}
	else
	{
		fprintf(stderr, "OH FUG\n");
		return 1;
	}
}
