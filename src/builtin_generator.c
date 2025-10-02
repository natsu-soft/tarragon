#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "mem_sys.h"
#include "perfect_json.h"
#include "hashmaps/map16.h"
#include "allocators/arena.h"
#include "allocators/scratch.h"
#include "allocators/workpad.h"
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

#define MAPPED_OPERATORS \
	X(IS,		  "is",				2) \
	X(IN,		  "in",				2) \
	X(AS,		  "as",				2) \
	X(NOT,		  "not",				3) \
	X(OR,		  "or",				2) \
	X(AND,		  "and",				3) \
	X(OP_BITWISE_XOR,		  "xor",				3) \
	X(OP_POWER,					    "**",			2) \
	X(OP_PRODUCT,				    "*",			1) \
	X(OP_SUM,					    "+",			1) \
	X(OP_DIFFERENCE,			    "-",			1) \
	X(OP_DIVISION,				    "/",			1) \
	X(OP_MODULO,				    "%",			1) \
	X(OP_BOOL_AND,				    "&&",			2) \
	X(OP_BITWISE_AND,			    "&",			1) \
	X(OP_BOOL_OR,				    "||",			2) \
	X(OP_BITWISE_OR,				"|",			1) \
	X(OP_BITSHIFT_LEFT,				"<<",			2) \
	X(OP_BITSHIFT_RIGHT,			">>",			2) \
	X(OP_BITWISE_XOR,				"^",			1) \
	X(OP_EQUALS,					"==",			2) \
	X(OP_INEQUALS,					"!=",			2) \
	X(OP_LESS_THAN_EQUALS,			"<=",			2) \
	X(OP_LESS_THAN,					"<",			1) \
	X(OP_GREATER_THAN_EQUALS,		">=",			2) \
	X(OP_GREATER_THAN,				">",			1) \
	X(OP_BITWISE_NOT,	 "~",	1)	\
	X(OP_BOOL_NOT,		 "!",	1)

static map16_t op_map = { 0 };
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

static void functions(json_t* json)
{
	char* str;
	int32_t len;
	json_t dest = { 0x00 };
	uint8_t func_flags = 0x00;
	while (pj_next_key(&str, &len, json))
	{
		const uint8_t flags = pj_next_value(&dest, json);
		if (strncmp("name", str, len) == 0)
		{
			pj_str(&str, &len, &dest);
			fprintf(file, "\tFUNC(\"");
			printf(" + ");
			for (int32_t i = 0; i < len; ++i)
			{
				fprintf(file, "%c", str[i]);
				printf("%c", str[i]);
			}
			printf("\n");
			fprintf(file, "\", %d)\n", len);
		}
		else if (strncmp("return_type", str, len) == 0)
		{
			pj_str(&str, &len, &dest);
			if (strncmp("enum::", str, len) == 0)
			{
				str += 6;
				len -= 6;
			}
			fprintf(file, "\tFUNC_RET(\"");
			for (int32_t i = 0; i < len; ++i)
			{
				fprintf(file, "%c", str[i]);
			}
			fprintf(file, "\", %d)\n", len);
		}
		else if (strncmp("is_const", str, len) == 0)
		{
			func_flags |= (flags & 0x0f) == JSON_TRUE;
		}
		else if (strncmp("is_vararg", str, len) == 0)
		{
			func_flags |= ((flags & 0x0f) == JSON_TRUE) << 1;
		}
		else if (strncmp("is_static", str, len) == 0)
		{
			func_flags |= ((flags & 0x0f) == JSON_TRUE) << 2;
		}
		else if (strncmp("is_virtual", str, len) == 0)
		{
			func_flags |= ((flags & 0x0f) == JSON_TRUE) << 3;
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
	fprintf(file, "\tFUNC_FLAGS(%#04x)\n", func_flags);
}

static void utility_functions(json_t* json)
{
	char* str;
	int32_t len;
	json_t dest = { 0x00 };
	while (pj_next_key(&str, &len, json))
	{
		uint8_t flags = pj_next_value(&dest, json);
		if (strncmp("name", str, len) == 0)
		{
			pj_str(&str, &len, &dest);
			fprintf(file, "GLOBAL_FUNC(\"");
			printf(" + ");
			for (int32_t i = 0; i < len; ++i)
			{
				fprintf(file, "%c", str[i]);
				printf("%c", str[i]);
			}
			fprintf(file, "\", %d)\n", len);
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
			fprintf(file, "\tFUNC_RET(\"");
			for (int32_t i = 0; i < len; ++i)
			{
				fprintf(file, "%c", str[i]);
			}
			fprintf(file, "\", %d)\n", len);
		}
		else if (strncmp("is_vararg", str, len) == 0)
		{
			uint8_t is_vararg = (flags & 0x0f) == JSON_TRUE;
			/* fprintf(stdout, "\tvararg? %s\n", is_vararg ? "yes" : "no"); */
			fprintf(file, "\tFUNC_FLAGS(%#04x)\n", is_vararg << 1);
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

static void members(json_t* json)
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
			fprintf(file, "\tMEMBER(\"");
			for (int32_t i = 0; i < len; ++i)
			{
				fprintf(file, "%c", str[i]);
			}
			fprintf(file, "\", %d, ", len);
		}
		if (strncmp("type", str, len) == 0)
		{
			pj_str(&str, &len, &dest);
			fprintf(file, "\"");
			for (int32_t i = 0; i < len; ++i)
			{
				fprintf(file, "%c", str[i]);
			}
			fprintf(file, "\", %d, ", len);
		}
		if (strncmp("description", str, len) == 0)
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
				const int32_t id = map16_fetch(&op_map, str, len);
				assert(id > -1 && "AAAAAAAAAAAAAAAAAA");
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
			fprintf(file, "\t\tOPERATOR_RIGHT(\"");
			for (int32_t i = 0; i < len; ++i)
			{
				fprintf(file, "%c", str[i]);
			}
			fprintf(file, "\", %d)\n", len);
		}
		else if (strncmp("return_type", str, len) == 0)
		{
			pj_str(&str, &len, &dest);
			fprintf(file, "\t\tOPERATOR_RETURN(\"");
			for (int32_t i = 0; i < len; ++i)
			{
				fprintf(file, "%c", str[i]);
			}
			fprintf(file, "\", %d)\n", len);
		}
		else if (strncmp("description", str, len) == 0)
		{
			pj_str(&str, &len, &dest);
			fprintf(file, "\t\tOPERATOR_DESC(\"");
			for (int32_t i = 0; i < len; ++i)
			{
				fprintf(file, "%c", str[i]);
			}
			fprintf(file, "\", %d)\n", len);
		}
	}
}

static void classes(json_t* json)
{
	char* str;
	int32_t len;
	json_t dest = { 0x00 };
	uint8_t class_flags = 0x00;
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
			fprintf(file, "\", %d)\n", len);
			printf("\n");
		}
		else if (strncmp("members", str, len) == 0)
		{
			pj_mapc_array(members, &dest);
		}
		else if (strncmp("operators", str, len) == 0)
		{
			pj_mapc_array(operators, &dest);
		}
		else if (strncmp("methods", str, len) == 0)
		{
			pj_mapc_array(functions, &dest);
		}
		else
		{
			printf("\tunprocessed key: ");
			for (int32_t i = 0; i < len; ++i)
			{
				printf("%c", str[i]);
			}
			printf("\n");
		}
	}
	// TODO: class flags (see func flags)
}

// if u free ur bitchmade
int main(int argc, char** argv)
{
	ms_handle_t handle;
	int32_t json_len;
	char* json_str = ms_map_entire_file("./extension_api.json", &json_len, &handle);
	arena_t* scratch = workpad_checkout(0);
	json_t json = { 0x00 };
	uint8_t res = pj_parse(&json, scratch, json_str, json_len);
	if (res & JSON_OBJECT)
	{
		const int32_t cap = 32;
		arena_t* op_scratch = workpad_checkout(4 * cap * 12);
		str_arena = workpad_checkout(256);
		map16_init(&op_map, 4 * cap, op_scratch->allocation);
#define X(ID, STR, LEN)											\
		do														\
		{														\
			char* op_str = arena_alloc(str_arena, LEN);			\
			memcpy(op_str, STR, LEN);							\
			map16_cache(&op_map, op_str, LEN, GDSCRIPT_##ID);	\
		} while(0);
		MAPPED_OPERATORS
#undef X
		file = fopen("./src/builtin.gen", "w");
		fprintf(file, "/* this file is generated in builtin_generator.c & relies upon extension_api.json */\n\n");
		fprintf(file, "#include \"builtin_macros.h\"\n\n");
		json_t header_json = { 0x00 };
		json_t dest = { 0x00 };
		char* key;
		int32_t len;
		fprintf(file, "/* init string arena--it's static so never free :^) */\n\n");
		fprintf(file, "arena_init(str_arena, %d /* fuck it, why not? */);\n\n", json_len);
		fprintf(file, "/* fuck off, im using relative ptrs */\n");
		fprintf(file, "#pragma GCC diagnostic push\n");
		fprintf(file, "#pragma GCC diagnostic ignore \"-Wint-conversion\"\n\n");
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
				pj_mapc_array(classes, &header_json);
			}
			else if (strncmp(key, "classes", len) == 0 && pj_next_value(&header_json, &json) == JSON_ARRAY)
			{
				fprintf(file, "\n/* bultin classes (read: types) */\n");
				pj_mapc_array(classes, &header_json);
			}
		}
		fprintf(file, "#pragma GCC diagnostic pop\n");
		fclose(file);
		return 0;
	}
	else
	{
		fprintf(stderr, "OH FUG\n");
		return 1;
	}
}
