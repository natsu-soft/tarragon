#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "mem_sys.h"
#include "perfect_json.h"
#include "allocators/arena.h"
#include "allocators/scratch.h"
#include "allocators/workpad.h"
#include "gdscript/gdscript_ids.h"
#include "gdscript/hashmaps.h"
#include "benchmark.h"
#include "builtin.h"

#ifdef _WIN32
typedef struct _iobuf FILE;
#else
typedef struct _IO_FILE FILE;
#endif

FILE* file;

static inline void copy_str(char* dest, const char* src, int32_t len)
{
	memcpy(dest, src, len);
	dest[len] = '\0';
}

#define NUMBERS \
	X(type) \
	X(class) \
	X(func) \
	X(param) \
	X(enum) \
	X(enum_value) \
	X(operator) \
	X(constant) \
	X(signal) \
	X(member)

#define X(A) static int32_t num_##A = 0;
NUMBERS
#undef X
static int32_t num_chars = 0;

static inline void zero_nums(void)
{
	#define X(A) num_##A = 0;
	NUMBERS
	#undef X
}

/* PHASE ONE */

static void p1_enums(json_t* json);
static void p1_enum_values(json_t* json);
static void p1_funcs(json_t* json);
static void p1_params(json_t* json);
static void p1_types(json_t* json);
static void p1_classes(json_t* json);
static void p1_operators(json_t* json);
static void p1_constants(json_t* json);
static void p1_members(json_t* json);
static void p1_signals(json_t* json);

static void p1_enums(json_t* json)
{
	char* str;
	int32_t len;
	json_t dest = {0};
	++num_enum;
	uint8_t flags;
	while((flags = pj_next_pair(&str, &len, &dest, json)))
	{
		if (strncmp("name", str, len) == 0)
		{
			pj_str(&str, &len, &dest);
			num_chars += len;
		}
		else if (strncmp("values", str, len) == 0)
		{
			pj_mapc_array(p1_enum_values, &dest);
		}
	}
}

static void p1_enum_values(json_t* json)
{
	char* str;
	int32_t len;
	json_t dest = {0};
	++num_enum_value;
	uint8_t flags;
	while((flags = pj_next_pair(&str, &len, &dest, json)))
	{
		if (strncmp("name", str, len) == 0 /* || strncmp("description", str, len) == 0 */)
		{
			pj_str(&str, &len, &dest);
			num_chars += len + 1;
			return;
		}
	}
}

static void p1_funcs(json_t* json)
{
	char* str;
	int32_t len;
	json_t dest = {0};
	++num_func;
	uint8_t flags;
	while((flags = pj_next_pair(&str, &len, &dest, json)))
	{
		if (strncmp("name", str, len) == 0 /* || strncmp("description", str, len) == 0 */)
		{
			pj_str(&str, &len, &dest);
			num_chars += len + 1;
		}
		else if (strncmp("arguments", str, len) == 0)
		{
			pj_mapc_array(p1_params, &dest);
		}
	}
}

static void p1_params(json_t* json)
{
	char* str;
	int32_t len;
	json_t dest = {0};
	++num_param;
	uint8_t flags;
	while((flags = pj_next_pair(&str, &len, &dest, json)))
	{
		if (strncmp("name", str, len) == 0)
		{
			pj_str(&str, &len, &dest);
			num_chars += len + 1;
			return;
		}
	}
}

static void p1_types(json_t* json)
{
	char* str;
	int32_t len;
	json_t dest = {0};
	++num_type;
	uint8_t flags;
	while((flags = pj_next_pair(&str, &len, &dest, json)))
	{
		if (strncmp("name", str, len) == 0)
		{
			pj_str(&str, &len, &dest);
			num_chars += len + 1;
			/* char* name = scratch_alloc(len + 1); */
			/* copy_str(name, str, len); */
			/* printf("%s\n", name); */
		}
		else if (strncmp("methods", str, len) == 0 || strncmp("constructors", str, len) == 0)
		{
			pj_mapc_array(p1_funcs, &dest);
		}
		else if (strncmp("members", str, len) == 0 || strncmp("properties", str, len) == 0)
		{
			pj_mapc_array(p1_members, &dest);
		}
		else if(strncmp("constants", str, len) == 0)
		{
			pj_mapc_array(p1_constants, &dest);
		}
		else if(strncmp("enums", str, len) == 0)
		{
			pj_mapc_array(p1_enums, &dest);
		}
		/* else if(strncmp("signals", str, len) == 0) */
		/* { */
		/* 	pj_mapc_array(p1_signals, &dest); */
		/* } */
		/* else if(strncmp("description", str, len) == 0 || strncmp("brief_description", str, len) == 0) */
		/* { */
		/* 	pj_str(&str, &len, &dest); */
		/* 	num_chars += len; */
		/* } */
		else if(strncmp("operators", str, len) == 0)
		{
			/* pj_mapc_array(p1_operators, &dest); */
			pj_mapc_array(p1_operators, &dest);
		}
		else
		{
			/* char* key = scratch_alloc(len  + 1); */
			/* copy_str(key, str, len); */
			/* printf("\t- %s\n", key); */
		}
	}
}

static void p1_classes(json_t* json)
{
	char* str;
	int32_t len;
	json_t dest = {0};
	++num_class;
	uint8_t flags;
	while((flags = pj_next_pair(&str, &len, &dest, json)))
	{
		if (strncmp("name", str, len) == 0)
		{
			pj_str(&str, &len, &dest);
			num_chars += len + 1;
			/* char* name = scratch_alloc(len + 1); */
			/* copy_str(name, str, len); */
			/* printf("%s\n", name); */
		}
		else if (strncmp("methods", str, len) == 0 || strncmp("constructors", str, len) == 0)
		{
			pj_mapc_array(p1_funcs, &dest);
		}
		else if (strncmp("members", str, len) == 0 || strncmp("properties", str, len) == 0)
		{
			pj_mapc_array(p1_members, &dest);
		}
		else if(strncmp("constants", str, len) == 0)
		{
			pj_mapc_array(p1_constants, &dest);
		}
		else if(strncmp("enums", str, len) == 0)
		{
			pj_mapc_array(p1_enums, &dest);
		}
		else if(strncmp("signals", str, len) == 0)
		{
			pj_mapc_array(p1_signals, &dest);
		}
		/* else if(strncmp("description", str, len) == 0 || strncmp("brief_description", str, len) == 0) */
		/* { */
		/* 	pj_str(&str, &len, &dest); */
		/* 	num_chars += len; */
		/* } */
		else
		{
			/* char* key = scratch_alloc(len  + 1); */
			/* copy_str(key, str, len); */
			/* printf("\t- %s\n", key); */
		}
	}
}
static void p1_operators(json_t* json)
{
	++num_operator;
}

static void p1_members(json_t* json)
{
	char* str;
	int32_t len;
	json_t dest = {0};
	++num_member;
	uint8_t flags;
	while((flags = pj_next_pair(&str, &len, &dest, json)))
	{
		if (strncmp("name", str, len) == 0 /* || strncmp("description", str, len) == 0 */)
		{
			pj_str(&str, &len, &dest);
			num_chars += len + 1;
		}
		else
		{
			char* key = scratch_alloc(len  + 1);
			copy_str(key, str, len);
			/* printf("members:%s\n", key); */
		}
	}
}


static void p1_constants(json_t* json)
{
	char* str;
	int32_t len;
	json_t dest = {0};
	++num_constant;
	uint8_t flags;
	while((flags = pj_next_pair(&str, &len, &dest, json)))
	{
		if (strncmp("name", str, len) == 0)
		{
			pj_str(&str, &len, &dest);
			num_chars += len + 1;
			return;
		}
		else if (strncmp("value", str, len) == 0)
		{
		}
		/* else */
		/* { */
		/* 	char* key = scratch_alloc(len  + 1); */
		/* 	copy_str(key, str, len); */
		/* 	printf("constants:%s\n", key); */
		/* } */
	}
}

static void p1_signals(json_t* json)
{
	char* str;
	int32_t len;
	json_t dest = {0};
	++num_signal;
	uint8_t flags;
	while((flags = pj_next_pair(&str, &len, &dest, json)))
	{
		if (strncmp("name", str, len) == 0 /* || strncmp("description", str, len) == 0 */)
		{
			pj_str(&str, &len, &dest);
			num_chars += len + 1;
		}
		else if (strncmp("arguments", str, len) == 0)
		{
			pj_mapc_array(p1_params, &dest);
		}
	}
}

/* PHASE DEUCE */

static void p2_global_enums(json_t* json);
static void p2_global_funcs(json_t* json);
static void p2_global_types(json_t* json);
static void p2_global_classes(json_t* json);

#define X(A) static void p2_sum_##A(json_t* json) { ++num_##A; }
NUMBERS
#undef X

static void p2_type_maps(json_t* json);
static void p2_class_maps(json_t* json);

static void p2_types(json_t* json)
{
	int8_t defined_operators[GDSCRIPT_NUM_BUILTIN] = {0};
	char* str;
	int32_t len;
	json_t dest = {0};
	zero_nums();
	uint8_t flags;
	while((flags = pj_next_pair(&str, &len, &dest, json)))
	{
		if (strncmp("name", str, len) == 0)
		{
			pj_str(&str, &len, &dest);
			char* temp = scratch_alloc(len + 1);
			copy_str(temp, str, len);
			fprintf(file, "\n/* %s */\n", temp);
		}
		else if (strncmp("methods", str, len) == 0)
		{
			pj_mapc_array(p2_sum_func, &dest);
		}
		else if (strncmp("members", str, len) == 0)
		{
			pj_mapc_array(p2_sum_member, &dest);
		}
		else if (strncmp("constants", str, len) == 0)
		{
			pj_mapc_array(p2_sum_constant, &dest);
		}
		else if (strncmp("enums", str, len) == 0)
		{
			pj_mapc_array(p2_sum_enum, &dest);
		}
		else if (strncmp("operators", str, len) == 0)
		{
			json_t operator = {0};
			json_t elem = {0};
			while (pj_next_value(&operator, &dest))
			{
				while(pj_next_pair(&str, &len, &elem, &operator))
				{
					if (strncmp("name", str, len) == 0)
					{
						pj_str(&str, &len, &elem);
						const gdscript_id_t op_id = builtin_operators(str, len);
						if (op_id)
						{
							defined_operators[op_id] = 1;
						}
						else if (strncmp("unary", str, 5) == 0)
						{
							defined_operators[str[5] == '+' ? GDSCRIPT_OP_IDENTITY : GDSCRIPT_OP_NEGATION]  = 1;
						}
					}
				}
			}
		}
	}
	int32_t cum =
#define X(A) num_##A +
			NUMBERS
#undef X
	0;
	if (cum)
	{
		fprintf(file, "ptr_map_count += hashmap_size(%d);\n", cum);
	}
	cum = 0;
	for (int32_t i = 0; i < GDSCRIPT_NUM_BUILTIN; ++i)
	{
		cum += defined_operators[i];
	}
	if (cum)
	{
		fprintf(file, "op_map_count += hashmap_size(%d);\n", cum);
	}
}

static void p2_classes(json_t* json)
{
	char* str;
	char* name;
	int32_t len;
	json_t dest = {0};
	zero_nums();
	uint8_t flags;
	while((flags = pj_next_pair(&str, &len, &dest, json)))
	{
		if (strncmp("name", str, len) == 0)
		{
			pj_str(&str, &len, &dest);
			name = scratch_alloc(len + 1);
			copy_str(name, str, len);
		}
		else if (strncmp("methods", str, len) == 0)
		{
			pj_mapc_array(p2_sum_func, &dest);
		}
		else if (strncmp("members", str, len) == 0 || strncmp("properties", str, len) == 0)
		{
			pj_mapc_array(p2_sum_member, &dest);
		}
		else if (strncmp("constants", str, len) == 0)
		{
			pj_mapc_array(p2_sum_constant, &dest);
		}
		else if (strncmp("enums", str, len) == 0)
		{
			pj_mapc_array(p2_sum_enum, &dest);
		}
		else if (strncmp("signals", str, len) == 0)
		{
			pj_mapc_array(p2_sum_signal, &dest);
		}
	}
	const int32_t cum =
#define X(A) num_##A +
			NUMBERS
#undef X
	0;
	if (cum)
	{
		fprintf(file, "ptr_map_count += hashmap_size(%d); /* %s */\n", cum, name);
	}
	else
	{
		fprintf(file, "/* %s has noting to hash!*/\n", name);
	}
}

static void p2_type_maps(json_t* json)
{
	int8_t defined_operators[GDSCRIPT_NUM_BUILTIN] = {0};
	char* str;
	char* name;
	int32_t len;
	json_t dest = {0};
	zero_nums();
	uint8_t flags;
	while((flags = pj_next_pair(&str, &len, &dest, json)))
	{
		if (strncmp("name", str, len) == 0)
		{
			pj_str(&str, &len, &dest);
			name = scratch_alloc(len + 1);
			copy_str(name, str, len);
		}
		else if (strncmp("methods", str, len) == 0)
		{
			pj_mapc_array(p2_sum_func, &dest);
		}
		else if (strncmp("members", str, len) == 0)
		{
			pj_mapc_array(p2_sum_member, &dest);
		}
		else if (strncmp("constants", str, len) == 0)
		{
			pj_mapc_array(p2_sum_constant, &dest);
		}
		else if (strncmp("enums", str, len) == 0)
		{
			pj_mapc_array(p2_sum_enum, &dest);
		}
		else if (strncmp("operators", str, len) == 0)
		{
			json_t operator = {0};
			json_t elem = {0};
			while (pj_next_value(&operator, &dest))
			{
				while(pj_next_pair(&str, &len, &elem, &operator))
				{
					if (strncmp("name", str, len) == 0)
					{
						pj_str(&str, &len, &elem);
						const gdscript_id_t op_id = builtin_operators(str, len);
						if (op_id)
						{
							defined_operators[op_id] = 1;
						}
						else if (strncmp("unary", str, 5) == 0)
						{
							defined_operators[str[5] == '+' ? GDSCRIPT_OP_IDENTITY : GDSCRIPT_OP_NEGATION]  = 1;
						}
					}
				}
			}
		}
	}
	fprintf(file, "\t{ /* %s */\n", name);
	int32_t cum =
#define X(A) num_##A +
			NUMBERS
#undef X
	0;
	if (cum)
	{
		fprintf(file, "\t\tconst int32_t cap_ptr = hashmap_size(%d);\n", cum);
		fprintf(file, "\t\tptr_map_init(&types[i].tagged_map, arena_alloc(&ptr_map_arena, ptr_map_scale * cap_ptr), cap_ptr);\n");
	}
	cum = 0;
	for (int32_t i = 0; i < GDSCRIPT_NUM_BUILTIN; ++i)
	{
		cum += defined_operators[i];
	}
	if (cum)
	{
		fprintf(file, "\t\tconst int32_t cap_op = hashmap_size(%d);\n", cum);
		fprintf(file, "\t\top_map_init(&types[i].op_map, arena_alloc(&op_map_arena, op_map_scale * cap_op), cap_op);\n");
	}
	fprintf(file, "\t\t++i;\n""\t}\n");
}

static void p2_class_maps(json_t* json)
{
	char* str;
	char* name;
	int32_t len;
	json_t dest = {0};
	zero_nums();
	uint8_t flags;
	while((flags = pj_next_pair(&str, &len, &dest, json)))
	{
		if (strncmp("name", str, len) == 0)
		{
			pj_str(&str, &len, &dest);
			name = scratch_alloc(len + 1);
			copy_str(name, str, len);
		}
		else if (strncmp("methods", str, len) == 0)
		{
			pj_mapc_array(p2_sum_func, &dest);
		}
		else if (strncmp("members", str, len) == 0 || strncmp("properties", str, len) == 0)
		{
			pj_mapc_array(p2_sum_member, &dest);
		}
		else if (strncmp("constants", str, len) == 0)
		{
			pj_mapc_array(p2_sum_constant, &dest);
		}
		else if (strncmp("enums", str, len) == 0)
		{
			pj_mapc_array(p2_sum_enum, &dest);
		}
		else if (strncmp("signals", str, len) == 0)
		{
			pj_mapc_array(p2_sum_signal, &dest);
		}
	}
	const int32_t cum =
#define X(A) num_##A +
			NUMBERS
#undef X
		0;
	if (cum)
	{
	fprintf(file,
			"\t{ /* %s */ \n"
			"\t\tconst int32_t cap = hashmap_size(%d);\n"
			"\t\tptr_map_init(&classes[i++].tagged_map, arena_alloc(&ptr_map_arena, ptr_map_scale * cap), cap);\n"
			"\t}\n", name, cum);
	}
	else
	{
		fprintf(file, "\tclasses[i++].tagged_map.cap = 0; /* %s has nothing to hash! */\n", name);
	}
}

/* PHASE THREE -- everything but types */

static void p3_global_enums(json_t* json);
static void p3_global_funcs(json_t* json);
static void p3_enum_values(json_t* json);

static void p3_params(json_t* json);
static void p3_types(json_t* json);
static void p3_classes(json_t* json);
static void p3_operators(json_t* json);
static void p3_methods(json_t* json);
static void p3_constructors(json_t* json);
static void p3_enums(json_t* json);
static void p3_members(json_t* json); // NOTE: also used for properties
static void p3_constants(json_t* json);
static void p3_signals(json_t* json);

static void p3_singletons(json_t* json);

static void p3_global_enums(json_t* json)
{
	char* str;
	int32_t len;
	json_t dest = {0};
	uint8_t flags;
	fprintf(file, "BEGIN_ENUM\n");
	while((flags = pj_next_pair(&str, &len, &dest, json)))
	{
		if (strncmp("name", str, len) == 0)
		{
			pj_str(&str, &len, &dest);
			char* temp = scratch_alloc(len + 1);
			copy_str(temp, str, len);
			printf("enum: %s\n", temp);
			fprintf(file, "\tCACHE_GLOBAL_ENUM(\"%s\", %d)\n", temp, len);
		}
		else if (strncmp("values", str, len) == 0)
		{
			num_enum_value = 0;
			fprintf(file, "\tBEGIN_VALUES\n");
			pj_mapc_array(p3_enum_values, &dest);
			fprintf(file, "\tEND_VALUES(%d)\n", num_enum_value);
		}
		else if (strncmp("is_bitfield", str, len) == 0 && (flags & 0x0f) == JSON_TRUE)
		{
			fprintf(file, "\tENUM_IS_BITFIELD\n");
		}
		/* else */
		/* { */
		/* 	char* temp = scratch_alloc(len + 1); */
		/* 	copy_str(temp, str, len); */
		/* 	printf("\tenum unprocessed: %s\n", temp); */
		/* } */
	}
	fprintf(file, "END\n");
}

static void p3_enum_values(json_t* json)
{
	char* str;
	int32_t len;
	json_t dest = {0};
	uint8_t flags;
	fprintf(file, "\t\tBEGIN_ENUM_VALUE\n");
	++num_enum_value;
	while((flags = pj_next_pair(&str, &len, &dest, json)))
	{
		if (strncmp("name", str, len) == 0)
		{
			pj_str(&str, &len, &dest);
			char* temp = scratch_alloc(len + 1);
			copy_str(temp, str, len);
			fprintf(file, "\t\t\tENUM_VALUE_NAME(\"%s\", %d)\n", temp, len);
		}
		else if (strncmp("value", str, len) == 0)
		{
			int32_t value;
			pj_int(&value, &dest);
			fprintf(file, "\t\t\tENUM_VALUE(%d)\n", value);
		}
	}
	fprintf(file, "\t\tEND\n");
}

// TODO: finish this
static void p3_global_funcs(json_t* json)
{
	char* str;
	int32_t len;
	json_t dest = {0};
	uint8_t flags;
	fprintf(file, "BEGIN_FUNC\n");
	while((flags = pj_next_pair(&str, &len, &dest, json)))
	{
		if (strncmp("name", str, len) == 0)
		{
			pj_str(&str, &len, &dest);
			char* temp = scratch_alloc(len + 1);
			copy_str(temp, str, len);
			printf("func: %s\n", temp);
			/* printf("\tunprocessed:"); */
			fprintf(file, "\tCACHE_GLOBAL_FUNC(\"%s\", %d)\n", temp, len);
		}
		else if (strncmp("arguments", str, len) == 0)
		{
			num_param = 0;
			pj_mapc_array(p3_params, &dest);
			if (num_param)
			{
				fprintf(file, "\tNUM_PARAMS(%d)\n", num_param);
			}
		}
		else if (strncmp("is_vararg", str, len) == 0 && (flags & 0x0f) == JSON_TRUE)
		{
			fprintf(file, "\tVARIADIC\n");
		}
		/* else */
		/* { */
		/* 	char* temp = scratch_alloc(len + 1); */
		/* 	copy_str(temp, str, len); */
		/* 	printf(" %s", temp); */
		/* } */
	}
	/* printf("\n"); */
	fprintf(file, "END\n");
}

static void p3_params(json_t* json)
{
	++num_param;
}


static void p3_types(json_t* json)
{
	char* str;
	int32_t len;
	json_t dest = {0};
	uint8_t flags;
	fprintf(file, "BEGIN_TYPE\n");
	while((flags = pj_next_pair(&str, &len, &dest, json)))
	{
		if (strncmp("name", str, len) == 0)
		{
			pj_str(&str, &len, &dest);
			char* temp = scratch_alloc(len + 1);
			copy_str(temp, str, len);
			printf("type: %s\n", temp);
			/* printf(" -"); */
			fprintf(file, "\tCACHE_GLOBAL_TYPE(\"%s\", %d)\n", temp, len);
		}
		else if (strncmp("is_keyed", str, len) && (flags & 0x0f) == JSON_TRUE)
		{
			fprintf(file, "\tIS_KEYED\n");
		}
		else if (strncmp("operators", str, len) == 0)
		{
			pj_mapc_array(p3_operators, &dest);
		}
		else if (strncmp("methods", str, len) == 0)
		{
			pj_mapc_array(p3_methods, &dest);
		}
		else if (strncmp("constructors", str, len) == 0)
		{
			num_func = 0;
			pj_mapc_array(p3_constructors, &dest);
			if (num_func)
			{
				fprintf(file, "\tNUM_CONSTRUCTORS(%d)\n", num_func);
			}
		}
		else if (strncmp("has_destructor", str, len) && (flags & 0x0f) == JSON_TRUE)
		{
			fprintf(file, "\tHAS_DESTRUCTOR\n");
		}
		else if (strncmp("enums", str, len) == 0)
		{
			pj_mapc_array(p3_enums, &dest);
		}
		else if (strncmp("members", str, len) == 0)
		{
			pj_mapc_array(p3_members, &dest);
		}
		else if (strncmp("constants", str, len) == 0)
		{
			pj_mapc_array(p3_constants, &dest);
		}
		/* else */
		/* { */
		/* 	char* temp = scratch_alloc(len + 1); */
		/* 	copy_str(temp, str, len); */
		/* 	printf(" %s", temp); */
		/* } */
	}
	fprintf(file, "END\n");
}

static void p3_classes(json_t* json)
{
	char* str;
	int32_t len;
	json_t dest = {0};
	uint8_t flags;
	fprintf(file, "BEGIN_CLASS\n");
	while((flags = pj_next_pair(&str, &len, &dest, json)))
	{
		if (strncmp("name", str, len) == 0)
		{
			pj_str(&str, &len, &dest);
			char* temp = scratch_alloc(len + 1);
			copy_str(temp, str, len);
			printf("class: %s\n", temp);
			printf(" -");
			fprintf(file, "\tCACHE_GLOBAL_CLASS(\"%s\", %d)\n", temp, len);
		}
		else if (strncmp("is_keyed", str, len) && (flags & 0x0f) == JSON_TRUE)
		{
			fprintf(file, "\tIS_KEYED\n");
		}
		/* else if (strncmp("operators", str, len) == 0) */
		/* { */
		/* 	pj_mapc_array(p3_operators, &dest); */
		/* } */
		else if (strncmp("methods", str, len) == 0 || strncmp("properties", str, len) == 0)
		{
			pj_mapc_array(p3_methods, &dest);
		}
		else if (strncmp("constructors", str, len) == 0)
		{
			num_func = 0;
			pj_mapc_array(p3_constructors, &dest);
			if (num_func)
			{
				fprintf(file, "\tNUM_CONSTRUCTORS(%d)\n", num_func);
			}
		}
		else if (strncmp("has_destructor", str, len) && (flags & 0x0f) == JSON_TRUE)
		{
			fprintf(file, "\tHAS_DESTRUCTOR\n");
		}
		else if (strncmp("enums", str, len) == 0)
		{
			pj_mapc_array(p3_enums, &dest);
		}
		else if (strncmp("members", str, len) == 0)
		{
			pj_mapc_array(p3_members, &dest);
		}
		else if (strncmp("constants", str, len) == 0)
		{
			pj_mapc_array(p3_constants, &dest);
		}
		else if (strncmp("signals", str, len) == 0)
		{
			pj_mapc_array(p3_signals, &dest);
		}
		else
		{
			char* temp = scratch_alloc(len + 1);
			copy_str(temp, str, len);
			printf(" %s", temp);
		}
	}
	printf("\n");
	fprintf(file, "END\n");
}

static void p3_operators(json_t* json)
{
	char* str;
	int32_t len;
	json_t dest = {0};
	uint8_t flags;
	while((flags = pj_next_pair(&str, &len, &dest, json)))
	{
		if (strncmp("name", str, len) == 0)
		{
			pj_str(&str, &len, &dest);
			const gdscript_id_t op_id = builtin_operators(str, len);
			if (op_id)
			{
				fprintf(file, "\tOPERATOR(%d)\n", op_id);
			}
			else
			{

				fprintf(file, "\tOPERATOR(%s)\n", str[5] == '+' ? "GDSCRIPT_OP_IDENTITY" : "GDSCRIPT_OP_NEGATION");
			}
			char* temp = scratch_alloc(len + 1);

			copy_str(temp, str, len);
			/* printf("   - %s\n", temp); */
		}
	}
}

static void p3_methods(json_t* json)
{
	char* str;
	int32_t len;
	json_t dest = {0};
	uint8_t flags;
	fprintf(file, "\tBEGIN_FUNC\n");
	while((flags = pj_next_pair(&str, &len, &dest, json)))
	{
		if (strncmp("name", str, len) == 0)
		{
			pj_str(&str, &len, &dest);
			char* temp = scratch_alloc(len + 1);
			copy_str(temp, str, len);
			/* printf("   -"); */
			fprintf(file, "\t\tCACHE_FUNC(\"%s\", %d)\n", temp, len);
		}
		else if (strncmp("is_vararg", str, len) == 0 && (flags & 0x0f) == JSON_TRUE)
		{
			fprintf(file, "\t\tVARIADIC\n");
		}
		else if (strncmp("is_const", str, len) == 0 && (flags & 0x0f) == JSON_TRUE)
		{
			fprintf(file, "\t\tCONSTANT\n");
		}
		else if (strncmp("is_static", str, len) == 0 && (flags & 0x0f) == JSON_TRUE)
		{
			fprintf(file, "\t\tSTATIC\n");
		}
		else if (strncmp("arguments", str, len) == 0)
		{
			num_param = 0;
			pj_mapc_array(p3_params, &dest);
			if (num_param)
			{
				fprintf(file, "\t\tNUM_PARAMS(%d)\n", num_param);
			}
		}
		/* else */
		/* { */
		/* 	char* temp = scratch_alloc(len + 1); */
		/* 	copy_str(temp, str, len); */
		/* 	printf(" %s", temp); */
		/* } */
	}
	/* printf("\n"); */
	fprintf(file, "\tEND\n");
}

static void p3_constructors(json_t* json)
{
	char* str;
	int32_t len;
	json_t dest = {0};
	uint8_t flags;
	fprintf(file, "\tBEGIN_FUNC ");
	++num_func;
	while((flags = pj_next_pair(&str, &len, &dest, json)))
	{
		if (strncmp("arguments", str, len) == 0)
		{
			num_param = 0;
			pj_mapc_array(p1_params, &dest);
			if (num_param)
			{
				fprintf(file, "NUM_PARAMS(%d) ", num_param);
			}
		}
	}
	fprintf(file, "END\n");
}

static void p3_enums(json_t* json)
{
	char* str;
	int32_t len;
	json_t dest = {0};
	uint8_t flags;
	fprintf(file, "\tBEGIN_ENUM\n");
	++num_func;
	while((flags = pj_next_pair(&str, &len, &dest, json)))
	{
		if (strncmp("name", str, len) == 0)
		{
			pj_str(&str, &len, &dest);
			char* temp = scratch_alloc(len + 1);
			copy_str(temp, str, len);
			fprintf(file, "\t\tCACHE_ENUM(\"%s\", %d)\n", temp, len);
		}
		else if (strncmp("values", str, len) == 0)
		{
			num_enum_value = 0;
			fprintf(file, "\tBEGIN_VALUES\n");
			pj_mapc_array(p3_enum_values, &dest);
			fprintf(file, "\tEND_VALUES(%d)\n", num_enum_value);
		}
	}
	fprintf(file, "\tEND\n");
}

static void p3_members(json_t* json)
{
	char* str;
	int32_t len;
	json_t dest = {0};
	uint8_t flags;
	while((flags = pj_next_pair(&str, &len, &dest, json)))
	{
		if (strncmp("name", str, len) == 0)
		{
			pj_str(&str, &len, &dest);
			char* temp = scratch_alloc(len + 1);
			copy_str(temp, str, len);
			fprintf(file, "\tCACHE_MEMBER(\"%s\", %d)\n", temp, len);
		}
	}
}

static void p3_constants(json_t* json)
{
	char* str;
	int32_t len;
	json_t dest = {0};
	uint8_t flags;
	while((flags = pj_next_pair(&str, &len, &dest, json)))
	{
		if (strncmp("name", str, len) == 0)
		{
			pj_str(&str, &len, &dest);
			char* temp = scratch_alloc(len + 1);
			copy_str(temp, str, len);
			fprintf(file, "\tCACHE_CONSTANT(\"%s\", %d)\n", temp, len);
		}
	}
}

static void p3_signals(json_t* json)
{
	char* str;
	int32_t len;
	json_t dest = {0};
	uint8_t flags;
	fprintf(file, "\tBEGIN_SIGNAL\n");
	while((flags = pj_next_pair(&str, &len, &dest, json)))
	{
		if (strncmp("name", str, len) == 0)
		{
			pj_str(&str, &len, &dest);
			char* temp = scratch_alloc(len + 1);
			copy_str(temp, str, len);
			fprintf(file, "\t\tCACHE_SIGNAL(\"%s\", %d)\n", temp, len);
		}
		else if (strncmp("arguments", str, len) == 0)
		{
			num_param = 0;
			pj_mapc_array(p3_params, &dest);
			if (num_param)
			{
				fprintf(file, "\t\tNUM_PARAMS(%d)\n", num_param);
			}
		}

	}
	fprintf(file, "\tEND\n");
}

static void p3_singletons(json_t* json)
{
	char* str;
	int32_t len;
	json_t dest = {0};
	uint8_t flags;
	while((flags = pj_next_pair(&str, &len, &dest, json)))
	{
		if (strncmp("type", str, len) == 0)
		{
			pj_str(&str, &len, &dest);
			char* temp = scratch_alloc(len + 1);
			copy_str(temp, str, len);
			fprintf(file, "SINGLETON(\"%s\", %d)\n", temp, len);
		}
	}
}


/* PHASE FOUR -- type hookups */

static void p4_funcs(json_t* json);
static void p4_types(json_t* json);
static void p4_type_constructors(json_t* json);
static void p4_classes(json_t* json);
static void p4_class_constructors(json_t* json);
static void p4_params(json_t* json);
static void p4_operators(json_t* json);
static void p4_members(json_t* json);
static void p4_constants(json_t* json);
static void p4_signals(json_t* json);


static void p4_funcs(json_t* json)
{
	char* str;
	int32_t len;
	json_t dest = {0};
	uint8_t flags;
	while((flags = pj_next_pair(&str, &len, &dest, json)))
	{
		if (strncmp("return_type", str, len) == 0)
		{
			pj_str(&str, &len, &dest);
			char* temp = scratch_alloc(len + 1);
			copy_str(temp, str, len);
			fprintf(file, "FUNC_RETURN(%d, \"%s\", %d)\n", num_func, temp, len);
		}
		else if (strncmp("arguments", str, len) == 0)
		{
			pj_mapc_array(p4_params, &dest);
		}
	}
	num_func++;
}

static void p4_params(json_t* json)
{
	char* str;
	int32_t len;
	json_t dest = {0};
	uint8_t flags;
	while((flags = pj_next_pair(&str, &len, &dest, json)))
	{
		if (strncmp("type", str, len) == 0)
		{
			pj_str(&str, &len, &dest);
			char* temp = scratch_alloc(len + 1);
			copy_str(temp, str, len);
			fprintf(file, "PARAM_TYPE(%d, \"%s\", %d)\n", num_func, temp, len);
		}
	}
	num_param++;
}

static void p4_types(json_t* json)
{
	char* str;
	int32_t len;
	json_t dest = {0};
	uint8_t flags;
	while((flags = pj_next_pair(&str, &len, &dest, json)))
	{
		if (strncmp("methods", str, len) == 0)
		{
			pj_mapc_array(p4_funcs, &dest);
		}
		else if (strncmp("operators", str, len) == 0)
		{
			pj_mapc_array(p4_operators, &dest);
		}
		if (strncmp("constructors", str, len) == 0)
		{
			pj_mapc_array(p4_type_constructors, &dest);
		}
		else if (strncmp("members", str, len) == 0)
		{
			pj_mapc_array(p4_members, &dest);
		}
		else if (strncmp("constants", str, len) == 0)
		{
			pj_mapc_array(p4_constants, &dest);
		}
	}
	++num_type;
}

static void p4_operators(json_t* json)
{
	char* str;
	int32_t len;
	json_t dest = {0};
	uint8_t flags;
	while((flags = pj_next_pair(&str, &len, &dest, json)))
	{
		if (strncmp("right_type", str, len) == 0)
		{
			pj_str(&str, &len, &dest);
			char* temp = scratch_alloc(len + 1);
			copy_str(temp, str, len);
			fprintf(file, "OPERATOR_RIGHT(%d, \"%s\", %d)\n", num_operator, temp, len);
		}
		else if (strncmp("return_type", str, len) == 0)
		{
			pj_str(&str, &len, &dest);
			char* temp = scratch_alloc(len + 1);
			copy_str(temp, str, len);
			fprintf(file, "OPERATOR_RETURN(%d, \"%s\", %d)\n", num_operator, temp, len);
		}
	}
	num_operator++;
}

static void p4_members(json_t* json)
{
	char* str;
	int32_t len;
	json_t dest = {0};
	uint8_t flags;
	while((flags = pj_next_pair(&str, &len, &dest, json)))
	{
		if (strncmp("type", str, len) == 0)
		{
			pj_str(&str, &len, &dest);
			char* temp = scratch_alloc(len + 1);
			copy_str(temp, str, len);
			fprintf(file, "MEMBER_TYPE(%d, \"%s\", %d)\n", num_member, temp, len);
		}
	}
	num_member++;
}

static void p4_constants(json_t* json)
{
	char* str;
	int32_t len;
	json_t dest = {0};
	uint8_t flags;
	while((flags = pj_next_pair(&str, &len, &dest, json)))
	{
		if (strncmp("type", str, len) == 0)
		{
			pj_str(&str, &len, &dest);
			char* temp = scratch_alloc(len + 1);
			copy_str(temp, str, len);
			fprintf(file, "CONSTANT_TYPE(%d, \"%s\", %d)\n", num_constant, temp, len);
		}
	}
	num_constant++;
}

static void p4_type_constructors(json_t* json)
{
	char* str;
	int32_t len;
	json_t dest = {0};
	uint8_t flags;
	fprintf(file, "TYPE_CONSTRUCTOR(%d, %d)\n", num_func, num_type);
	while((flags = pj_next_pair(&str, &len, &dest, json)))
	{
		if (strncmp("arguments", str, len) == 0)
		{
			pj_mapc_array(p4_params, &dest);
		}
	}
	num_func++;
}


static void p4_classes(json_t* json)
{
	char* str;
	int32_t len;
	json_t dest = {0};
	uint8_t flags;
	while((flags = pj_next_pair(&str, &len, &dest, json)))
	{
		if (strncmp("methods", str, len) == 0)
		{
			pj_mapc_array(p4_funcs, &dest);
		}
		else if (strncmp("operators", str, len) == 0)
		{
			pj_mapc_array(p4_operators, &dest);
		}
		if (strncmp("constructors", str, len) == 0)
		{
			pj_mapc_array(p4_class_constructors, &dest);
		}
		else if (strncmp("members", str, len) == 0 || strncmp("properties", str, len) == 0)
		{
			pj_mapc_array(p4_members, &dest);
		}
		else if (strncmp("constants", str, len) == 0)
		{
			pj_mapc_array(p4_constants, &dest);
		}
		else if (strncmp("signals", str, len) == 0)
		{
			pj_mapc_array(p4_signals, &dest);
		}
	}
	++num_class;
}

static void p4_class_constructors(json_t* json)
{
	char* str;
	int32_t len;
	json_t dest = {0};
	uint8_t flags;
	fprintf(file, "CLASS_CONSTRUCTOR(%d, %d)\n", num_func, num_type);
	while((flags = pj_next_pair(&str, &len, &dest, json)))
	{
		if (strncmp("arguments", str, len) == 0)
		{
			pj_mapc_array(p4_params, &dest);
		}
	}
	num_func++;
}

static void p4_signals(json_t* json)
{
	char* str;
	int32_t len;
	json_t dest = {0};
	uint8_t flags;
	while((flags = pj_next_pair(&str, &len, &dest, json)))
	{
		if (strncmp("arguments", str, len) == 0)
		{
			pj_mapc_array(p4_params, &dest);
		}
	}
}


/* MAIN */

int main(int argc, char** argv)
{
	ms_handle_t handle;
	int32_t json_len;
	char* json_str = ms_map_entire_file("./extension_api.json", &json_len, &handle);
	arena_t* parse_arena = workpad_checkout(0);
	json_t json = { 0x00 };
#define X(A) printf("sizeof(%s_t): %d\n", #A, sizeof(A##_t));
	NUMBERS
#undef X
	if (pj_parse(&json, parse_arena, json_str, json_len) ^ JSON_OBJECT)
	{
		fprintf(stderr, "FAILED TO PARSE extension_api.json!\n");
		return 1;
	}
	file = fopen("./src/builtin.gen", "w");
	fprintf(file, "/* this file is generated in builtin_generator.c & relies upon extension_api.json */\n\n");
	fprintf(file, "#include <stdlib.h>\n"
			"#include <stdint.h>\n"
			"#include <assert.h>\n"
			"#include \"allocators/arena.h\"\n"
			"#include \"hashmaps/ptr_map.h\"\n"
			"#include \"hashmaps/op_map.h\"\n"
			"#include \"builtin_macros.h\"\n\n");

	/* PHASE ONE */

	json_t dest = { 0x00 };
	char* key;
	int32_t len;
	uint8_t flags;
	int32_t body_idx;
	/* while((res = pj_next_key(&key, &len, &json))) */
	while ((flags = pj_next_pair(&key, &len, &dest, &json)))
	{
		if (strncmp("header", key, len) == 0 && (flags & 0x0f) == JSON_OBJECT)
		{
			printf("\nsupported version: ");
			json_t header = {0};
			int32_t val = 0;
			while((flags = pj_next_pair(&key, &len, &header, &dest)))
			{
				if (strncmp("version_major", key, len) == 0 && (flags & 0x0f) == JSON_INTEGER)
				{
					pj_int(&val, &header);
					printf("%d.", val);
					fprintf(file, "SUPPORTED_VERSION(major, %d)\n", val);
				}
				else if (strncmp(key, "version_minor", len) == 0 && (flags & 0x0f) == JSON_INTEGER)
				{
					pj_int(&val, &header);
					printf("%d.", val);
					fprintf(file, "SUPPORTED_VERSION(minor, %d)\n", val);
				}
				else if (strncmp(key, "version_patch", len) == 0 && (flags & 0x0f) == JSON_INTEGER)
				{
					pj_int(&val, &header);
					printf("%d\n", val);
					fprintf(file, "SUPPORTED_VERSION(patch, %d)\n", val);
				}
			}
			break;
		}
	}

	/* PHASE ONE */

	body_idx = json.index;
	while ((flags = pj_next_pair(&key, &len, &dest, &json)))
	{
		if (strncmp("global_enums", key, len) == 0)
		{
			pj_mapc_array(p1_enums, &dest);
		}
		else if (strncmp("utility_functions", key, len) == 0)
		{
			pj_mapc_array(p1_funcs, &dest);
		}
		else if (strncmp("builtin_classes", key, len) == 0)
		{
			pj_mapc_array(p1_types, &dest);
		}
		else if (strncmp("classes", key, len) == 0)
		{
			pj_mapc_array(p1_classes, &dest);
		}
	}
	fprintf(file,
#define X(A) "const int32_t " #A "_size = %d * sizeof(" #A "_t);\n"
			NUMBERS
#undef X
			"const int32_t api_size = "
#define X(A) #A "_size +\n\t"
			NUMBERS
#undef X
			"%d;\n\n",
#define X(A) num_##A,
			NUMBERS
#undef X
			num_chars);
#define X(A) const int32_t total_##A = num_##A;
	NUMBERS
#undef X

	/* PHASE DEUCE -- allocation for hashmaps */

	json.index = body_idx;
	zero_nums();
	while ((flags = pj_next_pair(&key, &len, &dest, &json)))
	{
		if (strncmp("global_enums", key, len) == 0)
		{
			pj_mapc_array(p2_sum_enum, &dest);
		}
		else if (strncmp("utility_functions", key, len) == 0)
		{
			pj_mapc_array(p2_sum_func, &dest);
		}
		else if (strncmp("builtin_classes", key, len) == 0)
		{
			pj_mapc_array(p2_sum_type, &dest);
		}
		else if (strncmp("classes", key, len) == 0)
		{
			pj_mapc_array(p2_sum_class, &dest);
		}
	}
	fprintf(file, "int32_t ptr_map_count = hashmap_size(%d); /* Global */\n", num_enum + num_func + total_type + total_class);
	fprintf(file, "int32_t op_map_count = 0; /* for builtin_types operators */\n");

	json.index = body_idx;
	zero_nums();
	while ((flags = pj_next_pair(&key, &len, &dest, &json)))
	{
		if (strncmp("builtin_classes", key, len) == 0)
		{
			pj_mapc_array(p2_types, &dest);
		}
		else if (strncmp("classes", key, len) == 0)
		{
			fprintf(file, "\n");
			pj_mapc_array(p2_classes, &dest);
		}
	}
	fprintf(file, "const int32_t ptr_map_scale = sizeof(void*) + sizeof(char*) + sizeof(int8_t);\n");
	fprintf(file, "const int32_t op_map_scale = sizeof(void*) + sizeof(gdscript_id_t);\n");
	fprintf(file, "\nconst int32_t ptr_map_size = ptr_map_scale * ptr_map_count;\n");
	fprintf(file, "const int32_t op_map_size = op_map_scale * op_map_count;\n");
	fprintf(file, "const int32_t cumulative_size = ptr_map_size + op_map_size + api_size;\n"
			"uint8_t* buffer = malloc(cumulative_size);\n"
			"assert(buffer && \"failed to allocate sufficient memory during builtin api generation!\");\n\n");
	fprintf(file, "arena_t ptr_map_arena = { .allocation = buffer, .cap = ptr_map_size };\n"
		"arena_t op_map_arena = { .allocation = buffer + ptr_map_size, .cap = op_map_size };\n"
		"buffer += ptr_map_size + op_map_size;\n");
	fprintf(file,
#define X(A) "arena_t " #A "_arena = { .allocation = buffer, .cap = " #A "_size };\nbuffer += " #A "_size;\n"
			NUMBERS
#undef X
		"arena_t str_arena = { .allocation = buffer, .cap = %d };\n", num_chars);

	/* PHASE DEUCE (B) */

	json.index = body_idx;
	zero_nums();
	while ((flags = pj_next_pair(&key, &len, &dest, &json)))
	{
		if (strncmp("global_enums", key, len) == 0)
		{
			pj_mapc_array(p2_sum_enum, &dest);
		}
		else if (strncmp("utility_functions", key, len) == 0)
		{
			pj_mapc_array(p2_sum_func, &dest);
		}
		else if (strncmp("builtin_classes", key, len) == 0)
		{
			pj_mapc_array(p2_sum_type, &dest);
		}
		else if (strncmp("classes", key, len) == 0)
		{
			pj_mapc_array(p2_sum_class, &dest);
		}
	}
	fprintf(file, "builtin_api.tagged_map.cap = hashmap_size(%d);\n"
			"ptr_map_init(&builtin_api.tagged_map, arena_alloc(&ptr_map_arena, ptr_map_scale * builtin_api.tagged_map.cap), builtin_api.tagged_map.cap);\n",
			num_enum + num_func + total_type + total_class);
	json.index = body_idx;
	while ((flags = pj_next_pair(&key, &len, &dest, &json)))
	{
		if (strncmp("builtin_classes", key, len) == 0)
		{
			fprintf(file, "{\n\ttype_t* types = type_arena.allocation;\tint32_t i = 0;\n");
			pj_mapc_array(p2_type_maps, &dest);
			fprintf(file, "}\n");
		}
		else if (strncmp("classes", key, len) == 0)
		{
			fprintf(file, "{\n\tclass_t* classes = class_arena.allocation;\tint32_t i = 0;\n");
			pj_mapc_array(p2_class_maps, &dest);
			fprintf(file, "}\n");
		}
	}

	/* PHASE THREE */

	json.index = body_idx;
	while ((flags = pj_next_pair(&key, &len, &dest, &json)))
	{
		if (strncmp("global_enums", key, len) == 0)
		{
			pj_mapc_array(p3_global_enums, &dest);
		}
		else if (strncmp("utility_functions", key, len) == 0)
		{
			pj_mapc_array(p3_global_funcs, &dest);
		}
		else if (strncmp("builtin_classes", key, len) == 0)
		{
			pj_mapc_array(p3_types, &dest);
		}
		else if (strncmp("classes", key, len) == 0)
		{
			pj_mapc_array(p3_classes, &dest);
		}
	}

	/* PHASE THREE (B) FLAG SINGLETONS */

	json.index = body_idx;
	while ((flags = pj_next_pair(&key, &len, &dest, &json)))
	{
		if (strncmp("singletons", key, len) == 0)
		{
			pj_mapc_array(p3_singletons, &dest);
		}
	}

	/* PHASE FOUR */

	json.index = body_idx;
	zero_nums();
	fprintf(file, "\n/* TYPE HOOKUPS */\n"
			"func_t* funcs = func_arena.allocation;\n"
			"param_t* params = param_arena.allocation;\n"
			"member_t* members = member_arena.allocation;\n"
			"constant_t* constants = constant_arena.allocation;\n"
			"operator_t* operators = operator_arena.allocation;\n"
			"type_t* types = type_arena.allocation;\n"
			"class_t* classes = class_arena.allocation;\n\n");
	while ((flags = pj_next_pair(&key, &len, &dest, &json)))
	{
		if (strncmp("utility_functions", key, len) == 0)
		{
			pj_mapc_array(p4_funcs, &dest);
		}
		else if (strncmp("builtin_classes", key, len) == 0)
		{
			pj_mapc_array(p4_types, &dest);
		}
		else if (strncmp("classes", key, len) == 0)
		{
			pj_mapc_array(p4_classes, &dest);
		}
	}
	return 0;
}
