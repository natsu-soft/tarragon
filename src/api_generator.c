#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "gdscript/gdscript_ids.h"
#include "perfect_json.h"
#include "mem_sys.h"
#include "api_structs.h"
#include "allocators/arena.h"
#include "allocators/scratch.h"
#include "allocators/workpad.h"
#include "hashmaps/api_map.h"
#include "gdscript/hashmaps.h"
#include "relative_ptrs.h"

#define PASSERT(A, MSG, ...) if (!(A)) { printf(MSG, ##__VA_ARGS__); fflush(stdout); assert(A); }

#define SCRUB_STR(STR, LEN)												\
	if (len == 18 && strncmp("enum::Variant.Type", str, 18) == 0)		\
	{																	\
		str = "Variant";												\
		len = 7;														\
	}																	\
	else if (len == 22 && strncmp("enum::Variant.Operator", str, 22) == 0) \
	{																	\
		str = "Variant:Operator";										\
		len = 16;														\
	}																	\
	else if (len == 12 && strncmp("Variant.Type", str, 12) == 0) \
	{																	\
		str = "Variant";												\
		len = 7;														\
	}																	\
	else if (len == 16 && strncmp("Variant.Operator", str, 16) == 0) \
	{																	\
		str = "Variant:Operator";										\
		len = 16;														\
	}																	\
	else if (len > 6 && strncmp("enum::", str, 6) == 0)					\
	{																	\
		str += 6;														\
		len += 6;														\
	}																	\
	else if (len > 10 && strncmp("bitfield::", str, 10) == 0)			\
	{																	\
		str += 10;														\
		len += 10;														\
	}

#ifdef _WIN32
typedef struct _iobuf FILE;
#else
typedef struct _IO_FILE FILE;
#endif

FILE* file;

static api_map_t* api_map = NULL;

static inline void cpystr(char* restrict dest, const char* restrict src, const int len)
{
	memcpy(dest, src, len);
	dest[len] = '\0';
}

static int32_t hashmap_size(int32_t num_entries)
{
	/* if (num_entries == 0) { return 0; } */
	num_entries <<= 2;
	for(int8_t i = 30; i >= 0; --i)
	{
		const int32_t mask = 1u << i;
		if (num_entries & mask)
		{
			return mask << (num_entries == mask ? 0 : 1);
			/* return ((num_entries & mask) & ~(mask - 1)) << 1; */
		}
	}
	assert(0 && "failed to find a good size for hashmap");
	return -1;
}

// NOTE: reorder larg to smol
#define NUMBERS									\
	X(class, classes)							\
		X(func, funcs)							\
		X(param, params)						\
		X(enum, enums)							\
		X(enum_value, enum_values)				\
		X(constant, constants)					\
		X(signal, signals)						\
		X(member, members)

#define X(A, B) A##_t* B = NULL;
	NUMBERS
#undef X

typedef struct fetch_res_t
{
	void* ptr;
	diff_ptr_t diff_ptr;
	api_tag_t tag;
} fetch_res_t;

static const fetch_res_t api_fetch(char* key, int16_t len)
{
	api_map_t* map = api_map;
	fetch_res_t res = {0};
	if (key[0] != ' ')
	{
		int32_t i0 = 0;
		for (int32_t i = i0; i < len; ++i)
		{
			if (key[i] != '.') { continue; }
			else
			{
				diff_ptr_t diff_ptr = api_map_fetch(map, key + i0, i - i0);
				char* temp = alloca(len + 1);
				cpystr(temp, key + i0, i - i0);
				/* printf("%s -> ", temp); */
				PASSERT((diff_ptr.delta & 0x7) == API_CLASS, "'%s' should have fetched a class!\n", temp);
				diff_ptr.delta &= ~0x7;
				map = abs_diff(&diff_ptr, classes);
				i0 = i + 1;
			}
		}
		/* char* temp = alloca(len + 1); */
		/* cpystr(temp, key + i0, len - i0); */
		/* printf("%s\n", temp); */
		/* fflush(stdout); */
		res.diff_ptr = api_map_fetch(map, key + i0, len - i0);
		res.tag = res.diff_ptr.delta & 0x07;
		res.diff_ptr.delta &= ~0x07;
		switch(res.tag)
		{
		case API_MEMBER: res.ptr = abs_diff(&res.diff_ptr, members); break;
		case API_ENUM: res.ptr = abs_diff(&res.diff_ptr, enums); break;
		case API_FUNC: res.ptr = abs_diff(&res.diff_ptr, funcs); break;
		case API_CLASS: res.ptr = abs_diff(&res.diff_ptr, classes); break;
		case API_SIGNAL: res.ptr = abs_diff(&res.diff_ptr, signals); break;
		case API_CONSTANT: res.ptr = abs_diff(&res.diff_ptr, constants); break;
		}
	}
	else
	{
		res.diff_ptr = api_map_fetch(map, key, len);
		res.tag = res.diff_ptr.delta & 0x07;
		res.diff_ptr.delta &= ~0x07;
		switch(res.tag)
		{
		case API_ENUM: res.ptr = abs_diff(&res.diff_ptr, enums); break;
		case API_CLASS: res.ptr = abs_diff(&res.diff_ptr, classes); break;
		default:
			assert(0 && "operation keys SHOULD return either an enum or a class!");
		}
	}
	res.diff_ptr.delta |= res.tag;
	return res;
}


static arena_t map_arena = {0};
static arena_t str_arena = {0};

#define X(_, A) static int32_t num_##A = 0;
	NUMBERS
#undef X
static int32_t num_operators = 0; // NOTE: operators get cached in the base api map
static int32_t str_size = 0;
static int32_t map_entries = 0;

#define X(_, A) static void accumulate_##A(json_t* json) { ++num_##A; }
static void accumulate_classes(json_t *json) { ++num_classes; }
static void accumulate_funcs(json_t *json) { ++num_funcs; }
static void accumulate_params(json_t *json) { ++num_params; }
static void accumulate_enums(json_t *json) { ++num_enums; }
static void accumulate_enum_values(json_t *json) { ++num_enum_values; }
static void accumulate_members(json_t *json) { ++num_members; }
static void accumulate_constants(json_t *json) { ++num_constants; }
static void accumulate_signals(json_t *json) { ++num_signals; }
#undef X

static void accumulate_class_cache(json_t* json)
{
	char* str;
	int32_t len;
	json_t dest = {0};
	uint8_t flags;
	while((flags = pj_next_pair(&str, &len, &dest, json)))
	{
		if (strncmp("methods", str, len) == 0)
		{
			pj_mapc_array(accumulate_funcs, &dest);
		}
		else if (strncmp("members", str, len) == 0 || strncmp("properties", str, len) == 0)
		{
			pj_mapc_array(accumulate_members, &dest);
		}
		else if(strncmp("constants", str, len) == 0)
		{
			pj_mapc_array(accumulate_constants, &dest);
		}
		else if(strncmp("enums", str, len) == 0)
		{
			pj_mapc_array(accumulate_enums, &dest);
		}
		else if(strncmp("signals", str, len) == 0)
		{
			pj_mapc_array(accumulate_signals, &dest);
		}
	}
}

class_t* hot_class;
diff_ptr_t hot_diff;

/* PHASE ONE */

static void p1_enums(json_t* json);
static void p1_enum_values(json_t* json);
static void p1_funcs(json_t* json);
static void p1_params(json_t* json);
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
	++num_enums;
	uint8_t flags;
	while((flags = pj_next_pair(&str, &len, &dest, json)))
	{
		if (strncmp("name", str, len) == 0)
		{
			pj_str(&str, &len, &dest);
			str_size += len;
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
	++num_enum_values;
	uint8_t flags;
	while((flags = pj_next_pair(&str, &len, &dest, json)))
	{
		if (strncmp("name", str, len) == 0 /* || strncmp("description", str, len) == 0 */)
		{
			pj_str(&str, &len, &dest);
			str_size += len + 1;
			return;
		}
	}
}

static void p1_funcs(json_t* json)
{
	char* str;
	int32_t len;
	json_t dest = {0};
	++num_funcs;
	uint8_t flags;
	while((flags = pj_next_pair(&str, &len, &dest, json)))
	{
		if (strncmp("name", str, len) == 0 /* || strncmp("description", str, len) == 0 */)
		{
			pj_str(&str, &len, &dest);
			str_size += len;
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
	++num_params;
	uint8_t flags;
	while((flags = pj_next_pair(&str, &len, &dest, json)))
	{
		if (strncmp("name", str, len) == 0)
		{
			pj_str(&str, &len, &dest);
			str_size += len + 1;
		}
		else if (strncmp("default_value", str, len) == 0)
		{
			pj_str(&str, &len, &dest);
			str_size += len + 1;
		}
	}
}

static void p1_classes(json_t* json)
{
	char* str;
	int32_t len;
	json_t dest = {0};
	++num_classes;
	uint8_t flags;
	while((flags = pj_next_pair(&str, &len, &dest, json)))
	{
		if (strncmp("name", str, len) == 0)
		{
			pj_str(&str, &len, &dest);
			str_size += len;
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
		/* /\* 	pj_str(&str, &len, &dest); *\/ */
		/* /\* 	num_chars += len; *\/ */
		/* } */
		else if(strncmp("operators", str, len) == 0)
		{
			/* pj_mapc_array(p1_operators, &dest); */
			pj_mapc_array(p1_operators, &dest);
		}
		/* else */
		/* { */
		/* 	char* key = scratch_alloc(len  + 1); */
		/* 	copy_str(key, str, len); */
		/* 	printf("\t- %s\n", key); */
		/* } */
	}
}

// NOTE: see api_struct.h regarding caching operators
static void p1_operators(json_t* json)
{
	char* str;
	int32_t len;
	json_t dest = {0};
	uint8_t flags;
	++num_operators;
	str_size += sizeof(uint8_t) +  sizeof(gdscript_id_t) + sizeof(diff_ptr_t);
	while((flags = pj_next_pair(&str, &len, &dest, json)))
	{
		if (strncmp("right_type", str, len) == 0)
		{
			str_size += sizeof(diff_ptr_t);
			return;
		}
	}
}

// TODO: const value parsing
static void p1_constants(json_t* json)
{
	char* str;
	int32_t len;
	json_t dest = {0};
	++num_constants;
	uint8_t flags;
	while((flags = pj_next_pair(&str, &len, &dest, json)))
	{
		if (strncmp("name", str, len) == 0 /* || strncmp("description", str, len) == 0 */)
		{
			pj_str(&str, &len, &dest);
			str_size += len;
			return;
		}
		/* else */
		/* { */
		/* 	char* key = scratch_alloc(len  + 1); */
		/* 	copy_str(key, str, len); */
		/* 	/\* printf("members:%s\n", key); *\/ */
		/* } */
	}
}

static void p1_members(json_t* json)
{
	char* str;
	int32_t len;
	json_t dest = {0};
	++num_members;
	uint8_t flags;
	while((flags = pj_next_pair(&str, &len, &dest, json)))
	{
		if (strncmp("name", str, len) == 0 /* || strncmp("description", str, len) == 0 */)
		{
			pj_str(&str, &len, &dest);
			str_size += len;
			return;
		}
		/* else */
		/* { */
		/* 	char* key = scratch_alloc(len  + 1); */
		/* 	copy_str(key, str, len); */
		/* 	/\* printf("members:%s\n", key); *\/ */
		/* } */
	}
}

static void p1_signals(json_t* json)
{
	char* str;
	int32_t len;
	json_t dest = {0};
	++num_signals;
	uint8_t flags;
	while((flags = pj_next_pair(&str, &len, &dest, json)))
	{
		if (strncmp("name", str, len) == 0 /* || strncmp("description", str, len) == 0 */)
		{
			pj_str(&str, &len, &dest);
			str_size += len;
		}
		else if (strncmp("arguments", str, len) == 0)
		{
			pj_mapc_array(p1_params, &dest);
		}
	}
}

/* PHASE DEUCE */

static void p2_classes(json_t* json)
{
#define X(_, A) num_##A = 0;
	NUMBERS
#undef X
	accumulate_class_cache(json);
	if (num_enums || num_funcs || num_constants || num_members || num_signals)
	{
		map_entries += hashmap_size(num_enums + num_funcs + num_constants + num_members + num_signals);
	}
}

/* PHASE DEUCE(B) */

static void p2b_classes(json_t* json)
{
	num_funcs = 0;
	num_enums = 0;
	num_members = 0;
	num_constants = 0;
	num_signals = 0;
	accumulate_class_cache(json);
	if (num_enums || num_funcs || num_constants || num_members || num_signals)
	{
		const int32_t map_cap = hashmap_size(num_enums + num_funcs + num_constants + num_members + num_signals);
		/* class->class_map.cap = map_cap; */
		/* const int32_t map_size = map_cap * (2 * sizeof(self_ptr_t) + sizeof(int32_t)); */
		/* api_map_back(&class->class_map, arena_alloc(&map_arena, map_size)); */
		class_t* class = classes + num_classes;
		api_map_back(class, &map_arena, map_cap);
	}
	++num_classes;
}


/* PHASE THREE: CACHE EVERYTHING NOW! */

static void p3_global_enums(json_t* json)
{
	char* str;
	int32_t len;
	json_t dest = {0};
	uint8_t flags;
	enum_t* e = enums + num_enums;
	while((flags = pj_next_pair(&str, &len, &dest, json)))
	{
		if (strncmp("name", str, len) == 0)
		{
			pj_str(&str, &len, &dest);
			if (strncmp("Variant.Type", str, len) == 0)
			{
				str = "Variant";
				len = 7;
			}
			else if (strncmp("Variant.Operator", str, len) == 0)
			{
				str = "Variant:Operator";
			}
			char* name = arena_push(&str_arena, str, len);
			diff_ptr_t* dptr = api_map_cache(api_map, name, len, diff_relate(e, enums));
			dptr->delta |= API_ENUM;
		}
		else if (strncmp("values", str, len) == 0)
		{
			e->num_values = pj_array_size(&dest);
		}
	}
	++num_enums;
}

static void p3_global_funcs(json_t* json)
{
	char* str;
	int32_t len;
	json_t dest = {0};
	uint8_t flags;
	func_t* func = funcs + num_funcs;
	while((flags = pj_next_pair(&str, &len, &dest, json)))
	{
		if (strncmp("name", str, len) == 0)
		{
			pj_str(&str, &len, &dest);
			char* name = arena_push(&str_arena, str, len);
			diff_ptr_t* dtr = api_map_cache(api_map, name, len, diff_relate(func, funcs));
			dtr->delta |= API_FUNC;
		}
		else if (strncmp("arguments", str, len) == 0)
		{
			num_params = 0;
			pj_mapc_array(accumulate_params, &dest);
			func->num_params = num_params;
		}
	}
	++num_funcs;
}

static void p3_enums(json_t* json);
static void p3_funcs(json_t* json);
static void p3_constructors(json_t* json);
static void p3_members(json_t* json);
static void p3_constants(json_t* json);
static void p3_signals(json_t* json);

static void p3_classes(json_t* json)
{
	char* str;
	int32_t len;
	json_t dest = {0};
	uint8_t flags;
	class_t* class = classes + num_classes;
	hot_class = class;
	while((flags = pj_next_pair(&str, &len, &dest, json)))
	{
		if (strncmp("name", str, len) == 0)
		{
			pj_str(&str, &len, &dest);
			char* name = arena_push(&str_arena, str, len);
			diff_ptr_t* dtr = api_map_cache(api_map, name, len, diff_relate(class, classes));
			dtr->delta |= API_CLASS;
			/* char* temp = scratch_alloc(len + 1); */
			/* cpystr(temp, str, len); */
			self_relate(&class->name_selfptr, name);
			/* printf("p3-class[%d]: %s +%x: %x(%x) -> %#x\n", num_classes, temp, class->name_selfptr.delta, dtr->delta, dtr->delta & 0x7, class); */
			class->name_len = len;
		}
		else if (strncmp("enums", str, len) == 0)
		{
			pj_mapc_array(p3_enums, &dest);
		}
		else if (strncmp("methods", str, len) == 0)
		{
			pj_mapc_array(p3_funcs, &dest);
		}
		else if (strncmp("constructors", str, len) == 0)
		{
			class->num_constructors = pj_array_size(&dest);
			dest.index = 0;
			pj_mapc_array(p3_constructors, &dest);
		}
		else if (strncmp("members", str, len) == 0 || strncmp("properties", str, len) == 0)
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
	}
	++num_classes;
}


static void p3_enums(json_t* json)
{
	char* str;
	int32_t len;
	json_t dest = {0};
	uint8_t flags;
	enum_t* e = enums + num_enums;
	while((flags = pj_next_pair(&str, &len, &dest, json)))
	{
		if (strncmp("name", str, len) == 0)
		{
			pj_str(&str, &len, &dest);
			char* name = arena_push(&str_arena, str, len);
			diff_ptr_t* dtr = api_map_cache(hot_class, name, len, diff_relate(e, enums));
			dtr->delta |= API_ENUM;
		}
		else if (strncmp("values", str, len) == 0)
		{
			num_enum_values = 0;
			pj_mapc_array(accumulate_enum_values, &dest);
			e->num_values = num_enum_values;
		}
	}
	++num_enums;
}

static void p3_funcs(json_t* json)
{
	char* str;
	int32_t len;
	json_t dest = {0};
	uint8_t flags;
	func_t* func= funcs + num_funcs;
	while((flags = pj_next_pair(&str, &len, &dest, json)))
	{
		if (strncmp("name", str, len) == 0)
		{
			pj_str(&str, &len, &dest);
			char* name = arena_push(&str_arena, str, len);
			diff_ptr_t* dtr = api_map_cache(hot_class, name, len, diff_relate(func, funcs));
			dtr->delta |= API_FUNC;
		}
		else if (strncmp("arguments", str, len) == 0)
		{
			num_params = 0;
			pj_mapc_array(accumulate_params, &dest);
			func->num_params = num_params;
		}
	}
	++num_funcs;
}

static void p3_constructors(json_t* json)
{
	char* str;
	int32_t len;
	json_t dest = {0};
	uint8_t flags;
	func_t* func= funcs + num_funcs;
	while((flags = pj_next_pair(&str, &len, &dest, json)))
	{
		if (strncmp("arguments", str, len) == 0)
		{
			num_params = 0;
			pj_mapc_array(accumulate_params, &dest);
			func->num_params = num_params;
		}
	}
	++num_funcs;
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
			char* name = arena_push(&str_arena, str, len);
			diff_ptr_t* dtr = api_map_cache(hot_class, name, len, diff_relate(members + num_members, members));
			dtr->delta |= API_MEMBER;
			++num_members;
			return;
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
			diff_ptr_t* dtr = api_map_cache(hot_class, str, len, diff_relate(constants + num_constants, constants));
			dtr->delta |= API_CONSTANT;
			++num_constants;
			return;
		}
	}
}

static void p3_signals(json_t* json)
{
	char* str;
	int32_t len;
	json_t dest = {0};
	uint8_t flags;
	signal_t* signal = signals + num_signals;
	while((flags = pj_next_pair(&str, &len, &dest, json)))
	{
		if (strncmp("name", str, len) == 0)
		{
			pj_str(&str, &len, &dest);
			char* name = arena_push(&str_arena, str, len);
			diff_ptr_t* dtr = api_map_cache(hot_class, str, len, diff_relate(signal, signals));
			dtr->delta |= API_SIGNAL;
		}
		else if (strncmp("arguments", str, len) == 0)
		{
			num_params = 0;
			pj_mapc_array(accumulate_params, &dest);
			signal->num_params = num_params;
		}
	}
	++num_signals;
}


/* AUDIT */

static void print_name(json_t* json)
{
	json_t dest = pj_find_key("name", json);
	if (dest.num_tokens > 0 && (dest.bitmasks[0] & 0x0f) == JSON_STRING)
	{
		char* str;
		int32_t len;
		pj_str(&str, &len, &dest);
		char* temp = scratch_alloc(len + 1);
		cpystr(temp, str, len);
		printf("> %s\n", temp);
	}
}


/* PHASE FOUR */

static void p4_enum_values(json_t* json);

static void p4_enums(json_t* json)
{
	char* str;
	int32_t len;
	json_t dest = {0};
	uint8_t flags;
	while((flags = pj_next_pair(&str, &len, &dest, json)))
	{
		if (strncmp("values", str, len) == 0)
		{
			pj_mapc_array(p4_enum_values, &dest);
		}
		else if (strncmp("is_bitfield", str, len) == 0 && (flags & 0x0f) == JSON_TRUE)
		{
			enums[num_enums].is_bitfield = 0xff;
		}
	}
	++num_enums;
}

static void p4_enum_values(json_t* json)
{
	char* str;
	int32_t len;
	json_t dest = {0};
	uint8_t flags;
	enum_value_t* val = enum_values + num_enum_values;
	while((flags = pj_next_pair(&str, &len, &dest, json)))
	{
		if (strncmp("names", str, len) == 0)
		{
			pj_str(&str, &len, &dest);
			char* name = arena_alloc(&str_arena, len + 1);
			cpystr(name, str, len);
			self_relate(&val->name, name);
		}
		else if (strncmp("value", str, len) == 0)
		{
			pj_int(&val->value, &dest);
		}
	}
	++num_enum_values;
}

static void p4_params(json_t* json);

static void p4_funcs(json_t* json)
{
	char* str;
	int32_t len;
	json_t dest = {0};
	uint8_t flags;
	func_t* func = funcs + num_funcs;
	while((flags = pj_next_pair(&str, &len, &dest, json)))
	{
		if (strncmp("return_type", str, len) == 0)
		{
			pj_str(&str, &len, &dest);
			SCRUB_STR(str, len)
			func->return_type = api_fetch(str, len).diff_ptr;
		}
		else if (strncmp("return_value", str, len) == 0)
		{
			dest = pj_find_key("type", &dest);
			pj_str(&str, &len, &dest);
			SCRUB_STR(str, len)
			func->return_type = api_fetch(str, len).diff_ptr;
		}
		else if (strncmp("arguments", str, len) == 0)
		{
			pj_mapc_array(p4_params, &dest);
		}
		else if (strncmp("is_const", str, len) == 0 && (flags & 0x0f) == JSON_TRUE)
		{
			func->bitflags |= FUNC_CONST;
		}
		else if (strncmp("is_vararg", str, len) == 0 && (flags & 0x0f) == JSON_TRUE)
		{
			func->bitflags |= FUNC_VARIADIC;
		}
		else if (strncmp("is_static", str, len) == 0 && (flags & 0x0f) == JSON_TRUE)
		{
			func->bitflags |= FUNC_STATIC;
		}
		else if (strncmp("is_virtual", str, len) == 0 && (flags & 0x0f) == JSON_TRUE)
		{
			func->bitflags |= FUNC_VIRTUAL;
		}
	}
	++num_funcs;
}

static void p4_params(json_t* json)
{
	char* str;
	int32_t len;
	json_t dest = {0};
	uint8_t flags;
	param_t* param = params + num_params;
	while((flags = pj_next_pair(&str, &len, &dest, json)))
	{
		if (strncmp("name", str, len) == 0)
		{
			pj_str(&str, &len, &dest);
			char* name = arena_alloc(&str_arena, len + 1);
			cpystr(name, str, len);
			self_relate(&param->name, name);
		}
		else if (strncmp("type", str, len) == 0)
		{
			pj_str(&str, &len, &dest);
			SCRUB_STR(str, len)
			param->type = api_fetch(str, len).diff_ptr;
		}
		else if (strncmp("default_value", str, len) == 0)
		{
			pj_str(&str, &len, &dest);
			char* value = arena_alloc(&str_arena, len + 1);
			cpystr(value, str, len);
			self_relate(&param->default_value, value);
		}
	}
	++num_params;
}

static void p4_members(json_t* json);
static void p4_constructors(json_t* json);
static void p4_constants(json_t* json);
static void p4_signals(json_t* json);
static void p4_operators(json_t* json);

static void p4_classes(json_t* json)
{
	char* str;
	int32_t len;
	json_t dest = {0};
	uint8_t flags;
	class_t* class = classes + num_classes;
	hot_class = class;
	hot_diff = diff_relate(class, classes);
	hot_diff.delta |= API_CLASS;
	while((flags = pj_next_pair(&str, &len, &dest, json)))
	{
		if (strncmp("inherits", str, len) == 0)
		{
			pj_str(&str, &len, &dest);
			char* temp = scratch_alloc(len + 1);
			cpystr(temp, str, len);
			fetch_res_t fetch = api_fetch(str, len);
			PASSERT(fetch.tag == API_CLASS, "fetched value for '%s' is not a class!\n", temp);
			class->inherits = fetch.diff_ptr;
		}
		else if (strncmp("methods", str, len) == 0)
		{
			pj_mapc_array(p4_funcs, &dest);
		}
		else if (strncmp("constructors", str, len) == 0)
		{
			pj_mapc_array(p4_constructors, &dest);
		}
		else if (strncmp("members", str, len) == 0 || strncmp("properties", str, len) == 0)
		{
			pj_mapc_array(p4_members, &dest);
		}
		else if(strncmp("constants", str, len) == 0)
		{
			pj_mapc_array(p4_constants, &dest);
		}
		else if(strncmp("enums", str, len) == 0)
		{
			pj_mapc_array(p4_enums, &dest);
		}
		else if(strncmp("signals", str, len) == 0)
		{
			pj_mapc_array(p4_signals, &dest);
		}
		else if (strncmp("is_keyed", str, len) == 0 && (flags & 0x0f) == JSON_TRUE)
		{
			class->bitflags |= CLASS_IS_KEYED;
		}
		else if (strncmp("is_instantiable", str, len) == 0 && (flags & 0x0f) == JSON_TRUE)
		{
			class->bitflags |= CLASS_IS_INSTANTIABLE;
		}
		else if (strncmp("has_destructor", str, len) == 0 && (flags & 0x0f) == JSON_TRUE)
		{
			class->bitflags |= CLASS_HAS_DESTRUCTOR;
		}
		else if(strncmp("operators", str, len) == 0)
		{
			/* char* temp = scratch_alloc(class->name_len + 1); */
			/* cpystr(temp, abs_self(&class->name_selfptr), class->name_len); */
			/* printf("> %s\n", temp); */
			pj_mapc_array(p4_operators, &dest);
		}
		/* else */
		/* { */
		/* 	char* key = scratch_alloc(len  + 1); */
		/* 	cpystr(key, str, len); */
		/* 	printf("  class.%s\n", key); */
		/* } */
	}
	++num_classes;
}

static void p4_members(json_t* json)
{
	json_t dest = pj_find_key("type", json);
	char* str;
	int32_t len;
	pj_str(&str, &len, &dest);
	SCRUB_STR(str, len)
	fetch_res_t fetch = api_fetch(str, len);
	members[num_members].type = fetch.diff_ptr;
	num_members++;
}

static void p4_constructors(json_t* json)
{
	funcs[num_funcs].return_type = hot_diff;
	json_t dest = pj_find_key("arguments", json);
	if (dest.num_tokens > 0 && (dest.bitmasks[0] & 0x0f) == JSON_ARRAY)
	{
		pj_mapc_array(p4_params, &dest);
	}
	++num_funcs;
}

static void p4_constants(json_t* json)
{
	json_t dest = pj_find_key("type", json);
	if (dest.num_tokens == 1 && (dest.bitmasks[0] & 0x0f) == JSON_STRING)
	{
		char* str;
		int32_t len;
		pj_str(&str, &len, &dest);
		SCRUB_STR(str, len)
		fetch_res_t fetch = api_fetch(str, len);
		constants[num_constants].type = fetch.diff_ptr;
	}
	else
	{
		json_t dest = pj_find_key("value", json);
		if (dest.num_tokens == 1)
		{
			switch (dest.bitmasks[0] & 0x0f)
			{
			case JSON_INTEGER:
				constants[num_constants].type = api_fetch("int", 3).diff_ptr;
			case JSON_FLOAT:
				constants[num_constants].type = api_fetch("float", 5).diff_ptr;
			case JSON_TRUE: __attribute__((fallthrough));
			case JSON_FALSE:
				constants[num_constants].type = api_fetch("bool", 4).diff_ptr;
			case JSON_NULL:
				constants[num_constants].type = api_fetch("Nil", 3).diff_ptr;
			}
		}
		else
		{
			printf("\n");
			pj_allman(json);
			fflush(stdout);
			assert(0 && "what fucking constant is this?");
		}
	}
	++num_constants;
}

static void p4_signals(json_t* json)
{
	json_t dest = pj_find_key("arguments", json);
	if (dest.num_tokens > 0 && (dest.bitmasks[0] & 0x0f) == JSON_ARRAY)
	{
		pj_mapc_array(p4_params, &dest);
	}
	++num_signals;
}

static void p4_operators(json_t* json)
{
	char* str;
	int32_t len;
	json_t dest = {0};
	uint8_t flags;
	gdscript_id_t operator = GDSCRIPT_NONE;
	diff_ptr_t lhs = hot_diff;
	diff_ptr_t rhs = {0};
	diff_ptr_t ret = {0};
	while((flags = pj_next_pair(&str, &len, &dest, json)))
	{
		if (strncmp("name", str, len) == 0)
		{
			pj_str(&str, &len, &dest);
			char* temp = scratch_alloc(len + 1);
			cpystr(temp, str, len);
			/* printf("\t%s ", temp); */
			operator = builtin_operators(str, len);
			if (operator != GDSCRIPT_NONE) { /* just for clarity lmao */ }
			else if (strncmp("unary", str, 5) == 0)
			{
				operator = str[5] == '+' ? GDSCRIPT_OP_IDENTITY : GDSCRIPT_OP_NEGATION;
			}
		}
		else if (strncmp("return_type", str, len) == 0)
		{
			pj_str(&str, &len, &dest);
			/* char* temp = scratch_alloc(len + 1); */
			/* cpystr(temp, str, len); */
			/* printf("ret: %s ", temp); */
			SCRUB_STR(str, len);
			const fetch_res_t res = api_fetch(str, len);
			assert(res.tag != API_NONE);
			ret = api_fetch(str, len).diff_ptr;
			/* printf("(OK) "); */
		}
		else if (strncmp("right_type", str, len) == 0)
		{
			pj_str(&str, &len, &dest);
			/* char* temp = scratch_alloc(len + 1); */
			/* cpystr(temp, str, len); */
			/* printf("rhs: %s ", temp); */
			SCRUB_STR(str, len);
			const fetch_res_t res = api_fetch(str, len);
			assert(res.tag != API_NONE);
			rhs = res.diff_ptr;
			/* printf("(OK) "); */
			/* char* temp = scratch_alloc(len + 1); */
			/* cpystr(temp, str, len); */
			/* printf(" rhs: %s(%#x)", temp, rhs.delta); */
		}
	}
	/* printf("\n"); */
	/* fflush(stdout); */
	assert(operator != GDSCRIPT_NONE && "failed to determine operator gdscript token id!");
	assert(ret.delta && "failed to determine operator return type!");
	char* key_bytes = arena_alloc(&str_arena, sizeof(uint8_t));
	*key_bytes = ' ';
	arena_push(&str_arena, &operator, sizeof(gdscript_id_t));
	arena_push(&str_arena, &lhs, sizeof(diff_ptr_t));
	len = sizeof(uint8_t) + sizeof(gdscript_id_t) + sizeof(diff_ptr_t);
	if (rhs.delta)
	{
		arena_push(&str_arena, &rhs, sizeof(diff_ptr_t));
		len += sizeof(diff_ptr_t);
	}
	/* printf("\n\t\t"); */
	/* for (int32_t i = 0; i < len; ++i) */
	/* { */
	/* 	printf("%#x ", key_bytes[i]); */
	/* } */
	fflush(stdout);
	api_map_cache(api_map, key_bytes, len, ret);
}

static void p4_singletons(json_t* json)
{
	json_t dest = pj_find_key("type", json);
	char* str;
	int32_t len;
	pj_str(&str, &len, &dest);
	const fetch_res_t fetch = api_fetch(str, len);
	assert(fetch.tag == API_CLASS && "api_fetch() did not return a valid class in p4_singletons()!");
	class_t* class = fetch.ptr;
	char* temp = scratch_alloc(class->name_len + 1);
	cpystr(temp, abs_self(&class->name_selfptr), class->name_len);
	/* printf("class->name (%d): %s\n", class->name_len, temp); */
	class->bitflags |= CLASS_IS_SINGLETON;
}

#ifndef DO_NOT_AUDIT

static void audit_enum_values(json_t* json);

static void audit_global_enums(json_t* json)
{
	char* str;
	int32_t len;
	json_t dest = {0};
	uint8_t flags;
	enum_t* e = enums + num_enums;
	while ((flags = pj_next_pair(&str, &len, &dest, json)))
	{
		if (strncmp("name", str, len) == 0)
		{
			pj_str(&str, &len, &dest);
			SCRUB_STR(str, len)
			fetch_res_t res = api_fetch(str, len);
			assert(res.tag == API_ENUM);
			assert(res.ptr == e);
			e = res.ptr;
		}
		else if (strncmp("values", str, len) == 0)
		{
			pj_mapc_array(audit_enum_values, &dest);
		}
		else if (strncmp("is_bitfield", str, len) == 0)
		{
			uint8_t is_bitfield = (flags & 0x0f) == JSON_TRUE ? 0xff : 0x00;
			PASSERT(e->is_bitfield == is_bitfield, "saved: %#x, expected: %#x\n", e->is_bitfield, is_bitfield);
		}
	}
	num_enums++;
}

static void audit_enum_values(json_t* json)
{
	char* str;
	int32_t len;
	json_t dest = {0};
	uint8_t flags;
	enum_value_t* val = enum_values + num_enum_values;
	while((flags = pj_next_pair(&str, &len, &dest, json)))
	{
		if (strncmp("name", str, len) == 0)
		{
			pj_str(&str, &len, &dest);
			assert(strncmp(str, abs_self(&val->name), len) == 0);
		}
		else if (strncmp("value", str, len) == 0)
		{
			int ground_truth;
			pj_int(&ground_truth, &dest);
			assert(ground_truth == val->value);
		}
	}
	++num_enum_values;
}

static void audit_params(json_t* json);

static void audit_global_funcs(json_t* json)
{
	char* str;
	int32_t len;
	json_t dest = {0};
	uint8_t flags;
	func_t* func = funcs + num_funcs;
	while((flags = pj_next_pair(&str, &len, &dest, json)))
	{
		if (strncmp("name", str, len) == 0)
		{
			pj_str(&str, &len, &dest);
			SCRUB_STR(str, len)
			fetch_res_t res = api_fetch(str, len);
			assert(res.tag == API_FUNC);
			assert(res.ptr == func);
		}
		else if (strncmp("return_type", str, len) == 0)
		{
			pj_str(&str, &len, &dest);
			SCRUB_STR(str, len)
			assert(func->return_type.delta == api_fetch(str, len).diff_ptr.delta);
		}
		else if (strncmp("return_value", str, len) == 0)
		{
			dest = pj_find_key("type", &dest);
			pj_str(&str, &len, &dest);
			SCRUB_STR(str, len)
			assert(func->return_type.delta == api_fetch(str, len).diff_ptr.delta);
		}
		else if (strncmp("arguments", str, len) == 0)
		{
			pj_mapc_array(audit_params, &dest);
		}
		else if (strncmp("is_const", str, len) == 0)
		{
			const uint8_t mask = (flags & 0x0f) == JSON_TRUE ? FUNC_CONST : 0x00;
			assert((func->bitflags & FUNC_CONST) == mask);
		}
		else if (strncmp("is_vararg", str, len) == 0)
		{
			const uint8_t mask = (flags & 0x0f) == JSON_TRUE ? FUNC_VARIADIC : 0x00;
			assert((func->bitflags & FUNC_VARIADIC) == mask);
		}
		else if (strncmp("is_static", str, len) == 0)
		{
			const uint8_t mask = (flags & 0x0f) == JSON_TRUE ? FUNC_STATIC : 0x00;
			assert((func->bitflags & FUNC_STATIC) == mask);
		}
		else if (strncmp("is_virtual", str, len) == 0)
		{
			const uint8_t mask = (flags & 0x0f) == JSON_TRUE ? FUNC_VIRTUAL : 0x00;
			assert((func->bitflags & FUNC_VIRTUAL) == mask);
		}
	}
	++num_funcs;
}

static void audit_params(json_t* json)
{
	char* str;
	int32_t len;
	json_t dest = {0};
	uint8_t flags;
	param_t* param = params + num_params;
	while((flags = pj_next_pair(&str, &len, &dest, json)))
	{
		if (strncmp("name", str, len) == 0)
		{
			pj_str(&str, &len, &dest);
			assert(strncmp(abs_self(&param->name), str, len) == 0);
		}
		else if (strncmp("type", str, len) == 0)
		{
			pj_str(&str, &len, &dest);
			SCRUB_STR(str, len)
			assert(param->type.delta == api_fetch(str, len).diff_ptr.delta);
		}
		else if (strncmp("default_value", str, len) == 0)
		{
			pj_str(&str, &len, &dest);
			assert(strncmp(abs_self(&param->default_value), str, len) == 0);
		}
	}
	++num_params;
}

static void audit_methods(json_t* json);

static void audit_classes(json_t* json)
{
	char* str;
	int32_t len;
	json_t dest = {0};
	uint8_t flags;
	class_t* class = classes + num_classes;
	hot_class = class;
	hot_diff = diff_relate(class, classes);
	hot_diff.delta |= API_CLASS;
	while((flags = pj_next_pair(&str, &len, &dest, json)))
	{
		if (strncmp("name", str, len) == 0)
		{
			pj_str(&str, &len, &dest);
			assert(strncmp(abs_self(&hot_class->name_selfptr), str, len) == 0);
			fetch_res_t fetch = api_fetch(str, len);
			assert(fetch.tag == API_CLASS);
			assert(class == fetch.ptr);
		}
		else if (strncmp("inherits", str, len) == 0)
		{
			pj_str(&str, &len, &dest);
			fetch_res_t fetch = api_fetch(str, len);
			assert(fetch.tag == API_CLASS);
			assert(class->inherits.delta == fetch.diff_ptr.delta);
		}
		else if (strncmp("methods", str, len) == 0)
		{
			pj_mapc_array(audit_methods, &dest);
		}
		else if (strncmp("constructors", str, len) == 0)
		{
			/* pj_mapc_array(p4_constructors, &dest); */
		}
		else if (strncmp("members", str, len) == 0 || strncmp("properties", str, len) == 0)
		{
			/* pj_mapc_array(p4_members, &dest); */
		}
		else if(strncmp("constants", str, len) == 0)
		{
			/* pj_mapc_array(p4_constants, &dest); */
		}
		else if(strncmp("enums", str, len) == 0)
		{
			/* pj_mapc_array(p4_enums, &dest); */
		}
		else if(strncmp("signals", str, len) == 0)
		{
			/* pj_mapc_array(p4_signals, &dest); */
		}
		else if (strncmp("is_keyed", str, len) == 0)
		{
			const uint8_t mask = (flags & 0x0f) == JSON_TRUE ? CLASS_IS_KEYED : 0x00;
			assert((class->bitflags & CLASS_IS_KEYED) == mask);
		}
		else if (strncmp("is_instantiable", str, len) == 0)
		{
			const uint8_t mask = (flags & 0x0f) == JSON_TRUE ? CLASS_IS_INSTANTIABLE : 0x00;
			assert((class->bitflags & CLASS_IS_INSTANTIABLE) == mask);
		}
		else if (strncmp("has_destructor", str, len) == 0)
		{
			const uint8_t mask = (flags & 0x0f) == JSON_TRUE ? CLASS_HAS_DESTRUCTOR : 0x00;
			assert((class->bitflags & CLASS_HAS_DESTRUCTOR) == mask);
		}
	}
	++num_classes;
}

static void audit_methods(json_t* json)
{
	char* str;
	int32_t len;
	json_t dest = {0};
	uint8_t flags;
	func_t* func = funcs + num_funcs;
	while((flags = pj_next_pair(&str, &len, &dest, json)))
	{
		if (strncmp("name", str, len) == 0)
		{
			pj_str(&str, &len, &dest);
			const int32_t total_len = hot_class->name_len + len + 1;
			char* temp = scratch_alloc(total_len + 2);
			memcpy(temp, abs_self(&hot_class->name_selfptr), hot_class->name_len);
			temp[hot_class->name_len] = '.';
			cpystr(temp + hot_class->name_len + 1, str, len);
			const fetch_res_t res = api_fetch(temp, total_len);
			printf("auditing: %s\n", temp);
			PASSERT(res.tag == API_FUNC, "key: '%s', tag: %x, ptr: %#x\n", temp, res.tag, res.ptr);
		}
		else if (strncmp("return_type", str, len) == 0)
		{
			pj_str(&str, &len, &dest);
			pj_allman(&dest);
			SCRUB_STR(str, len)
			fflush(stdout);
			const fetch_res_t res = api_fetch(str, len);
			PASSERT(res.tag != API_NONE, "bad fetch: %#x: %x\n", res.ptr, res.tag);
			PASSERT(func->return_type.delta == res.diff_ptr.delta, "%#x != %#x\n", func->return_type.delta, res.diff_ptr.delta);
		}
		else if (strncmp("return_value", str, len) == 0)
		{
			dest = pj_find_key("type", &dest);
			pj_str(&str, &len, &dest);
			SCRUB_STR(str, len)
			assert(func->return_type.delta == api_fetch(str, len).diff_ptr.delta);
		}
		else if (strncmp("arguments", str, len) == 0)
		{
			pj_mapc_array(audit_params, &dest);
		}
		else if (strncmp("is_const", str, len) == 0)
		{
			const uint8_t mask = (flags & 0x0f) == JSON_TRUE ? FUNC_CONST : 0x00;
			assert((func->bitflags & FUNC_CONST) == mask);
		}
		else if (strncmp("is_vararg", str, len) == 0)
		{
			const uint8_t mask = (flags & 0x0f) == JSON_TRUE ? FUNC_VARIADIC : 0x00;
			assert((func->bitflags & FUNC_VARIADIC) == mask);
		}
		else if (strncmp("is_static", str, len) == 0)
		{
			const uint8_t mask = (flags & 0x0f) == JSON_TRUE ? FUNC_STATIC : 0x00;
			assert((func->bitflags & FUNC_STATIC) == mask);
		}
		else if (strncmp("is_virtual", str, len) == 0)
		{
			const uint8_t mask = (flags & 0x0f) == JSON_TRUE ? FUNC_VIRTUAL : 0x00;
			assert((func->bitflags & FUNC_VIRTUAL) == mask);
		}
	}
	++num_funcs;
}


#endif


int main(int argc, char** argv)
{
	printf("NOTE: these should be ordered largest to smallest & be divisible by 8!\n");
#define X(A,...) printf("sizeof(" #A "_t): %d\n", sizeof(A##_t));
	NUMBERS
#undef X
	printf("sizeof(api_map_t): %d\n\n", sizeof(api_map_t));
	ms_handle_t handle;
	int32_t json_len;
	char* json_str = ms_map_entire_file("./extension_api.json", &json_len, &handle);
	arena_t* parse_arena = workpad_checkout(0);
	json_t json = { 0x00 };
	if (pj_parse(&json, parse_arena, json_str, json_len) ^ JSON_OBJECT)
	{
		fprintf(stderr, "FAILED TO PARSE extension_api.json!\n");
		return 1;
	}
	file = fopen("./src/godot_api.gen", "w");
	fprintf(file, "/* this file is generated by api_generator.c & relies upon extension_api.json */\n\n");
			/* "#include \"godot_api.h\"\n" */
			/* "#include \"api_utils.h\"\n\n"); */
	json_t dest = { 0x00 };
	char* key;
	int32_t len;
	uint8_t flags;
	int32_t body_idx;
	{
		int major;
		int minor;
		int patch;
		while ((flags = pj_next_pair(&key, &len, &dest, &json)))
		{
			if (strncmp("header", key, len) == 0 && (flags & 0x0f) == JSON_OBJECT)
			{
				json_t header = {0};
				while((flags = pj_next_pair(&key, &len, &header, &dest)))
				{
					if (strncmp("version_major", key, len) == 0 && (flags & 0x0f) == JSON_INTEGER)
					{
						pj_int(&major, &header);
					}
					else if (strncmp(key, "version_minor", len) == 0 && (flags & 0x0f) == JSON_INTEGER)
					{
						pj_int(&minor, &header);
					}
					else if (strncmp(key, "version_patch", len) == 0 && (flags & 0x0f) == JSON_INTEGER)
					{
						pj_int(&patch, &header);
					}
				}
				goto write_supported_version;
			}
		}
		perror("FAILED TO EXTRACT SUPPORTED VERSION FROM extension_api.json!\n");
		write_supported_version:
		body_idx = json.index;
		printf("supported version: %d.%d.%d\n\n", major, minor, patch);
		fprintf(file, "void print_supported_version(void) { printf(\"Supported verison: %d.%d.%d\\n\"); }\n", major, minor, patch);
	}
	// PHASE ONE: API SIZE WITHOUT HASHMAPS
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
		else if (strncmp("builtin_classes", key, len) == 0 || strncmp("classes", key, len) == 0)
		{
			pj_mapc_array(p1_classes, &dest);
		}
	}
#define X(A, B) sizeof(A##_t) * num_##B +
	const int32_t api_size = NUMBERS str_size;
#undef X
	printf("str size: %dKB\n", str_size / 1024);
	printf("api size: %dKB\n", api_size / 1024);

	// PHASE DEUCE: CUMULATIVE SIZE OF ALL HASHMAPS
	// recount enums for the global api
#define X(_, A) const int32_t cache_##A = num_##A; num_##A = 0;
	NUMBERS
#undef X
	json.index = body_idx;
	while ((flags = pj_next_pair(&key, &len, &dest, &json)))
	{
		if (strncmp("global_enums", key, len) == 0)
		{
			pj_mapc_array(accumulate_enums, &dest);
		}
	}
	const int32_t map_scale = sizeof(self_ptr_t) + sizeof(diff_ptr_t) + sizeof(int16_t);
	const int32_t base_map_entries = hashmap_size(num_operators + num_enums + num_classes);
	map_entries = base_map_entries;
	json.index = body_idx;
	while ((flags = pj_next_pair(&key, &len, &dest, &json)))
	{
		if (strncmp("builtin_classes", key, len) == 0 || strncmp("classes", key, len) == 0)
		{
			pj_mapc_array(p2_classes, &dest);
		}
	}
#define X(_, A) num_##A =  cache_##A;
	NUMBERS
#undef X
	const int32_t map_size = map_scale * map_entries;
	printf("cumulative map size: %dKB\n", map_size / 1024);
	const int32_t total_size = sizeof *api_map + map_size + api_size + 512;
	printf("total size: %dKB\n", total_size / 1024);
	/* uint8_t* buffer = malloc(total_size); */
	arena_init(&str_arena, total_size);
	/* memset(buffer, 0x00, total_size); // worth */
	api_map = arena_alloc(&str_arena, sizeof *api_map);
	printf("api_map: %#x\n", api_map);
	api_map->cap = base_map_entries;
	map_arena = (arena_t)
	{
		.allocation = arena_alloc_aligned(&str_arena, map_size, 64u),
		.cap = map_size
	};
	/* api_map_back(api_map, arena_alloc(&map_arena, map_scale * base_map_entries)); */
	api_map_back(api_map, &map_arena, base_map_entries);
#define X(_, A) \
	A = arena_alloc_aligned(&str_arena, num_##A * sizeof *A, 64u); \
	memset(A, 0x00, num_##A & sizeof *A); \
	printf(#A ": [%#x, %#x)\n", (size_t)A, (size_t)A + num_##A * sizeof *A);
	NUMBERS
#undef X
	/* str_arena = (arena_t) */
	/* { */
	/* 	.allocation = arena_alloc(&str_arena, str_size), */
	/* 	.cap = str_size */
	/* }; */

	/* PHASE DEUCE(B): INITIALIZE CLASS MAPS */

	json.index = body_idx;
	num_classes = 0;
	while ((flags = pj_next_pair(&key, &len, &dest, &json)))
	{
		if (strncmp("builtin_classes", key, len) == 0 || strncmp("classes", key, len) == 0)
		{
			pj_mapc_array(p2b_classes, &dest);
		}
	}

	/* PHASE THREE: CACHE EVERYTHING NOW! */

#define X(_, A) num_##A = 0;
	NUMBERS
#undef X
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
		else if (strncmp("builtin_classes", key, len) == 0 || strncmp("classes", key, len) == 0)
		{
			pj_mapc_array(p3_classes, &dest);
		}
	}

/* #define DO_NOT_AUDIT */
#ifndef DO_NOT_AUDIT
	api_map_audit(api_map);
	for (int32_t i = 0; i < cache_classes; ++i)
	{
		char* name = abs_self(&classes[i].name_selfptr);
		const int32_t len = classes[i].name_len;
		char* temp = scratch_alloc(len + 1);
		cpystr(temp, name, len);
		fflush(stdout);
		api_map_audit(classes + i);
	}
	printf("OK: SIMPLE AUDIT\n");
#endif

	/* PHASE 4: EVERYTHING ELSE */

#define X(_, A) num_##A = 0;
	NUMBERS
#undef X
	json.index = body_idx;
	while ((flags = pj_next_pair(&key, &len, &dest, &json)))
	{
		if (strncmp("global_enums", key, len) == 0)
		{
			pj_mapc_array(p4_enums, &dest);
		}
		else if (strncmp("utility_functions", key, len) == 0)
		{
			pj_mapc_array(p4_funcs, &dest);
		}
		else if (strncmp("builtin_classes", key, len) == 0 || strncmp("classes", key, len) == 0)
		{
			pj_mapc_array(p4_classes, &dest);
		}
		else if (strncmp("singletons", key, len) == 0)
		{
			pj_mapc_array(p4_singletons, &dest);
		}
	}
	printf("API binary fully generated!\n");

#ifndef DO_NOT_AUDIT
	api_map_audit(api_map);
	for (int32_t i = 0; i < cache_classes; ++i)
	{
		api_map_audit(classes + i);
	}
#define X(_, A) num_##A = 0;
	NUMBERS
#undef X
	json.index = body_idx;
	while ((flags = pj_next_pair(&key, &len, &dest, &json)))
	{
		if (strncmp("global_enums", key, len) == 0)
		{
			pj_mapc_array(audit_global_enums, &dest);
		}
		else if (strncmp("utility_functions", key, len) == 0)
		{
			pj_mapc_array(audit_global_funcs, &dest);
		}
		else if (strncmp("builtin_classes", key, len) == 0 || strncmp("classes", key, len) == 0)
		{
			pj_mapc_array(audit_classes, &dest);
		}
		else if (strncmp("singletons", key, len) == 0)
		{
			/* pj_mapc_array(audit_singletons, &dest); */
		}
	}
	printf("OK: FULL AUDIT\n");
#endif
	return 69;

	fprintf(file, "\nconst static uint8_t api_binary[%d] =\n{\n\t", total_size);
	uint8_t* bin = str_arena.allocation;
	for (int32_t i = 0; i < total_size; ++i)
	{
		if (bin[i])
		{
			fprintf(file, "%#04x, ", bin[i]);
		}
		else
		{
			fprintf(file, "0x00, ");
		}
	}
	fprintf(file, "\n};\n\n");
	fprintf(file,
#define X(A, B) "static " #A "_t* " #B " = api_binary + %ld;\n"
	NUMBERS
#undef X
#define X(A, B) , (size_t)B - (size_t)bin
	NUMBERS
#undef X
	);
	fprintf(file,
#define X(_, A) "const static int32_t num_" #A " = %d;\n"
		NUMBERS
#undef X
#define X(_, A) , num_##A
		NUMBERS
#undef X
		);
	fprintf(file, "const static uint64_t api_hash = %#x;\n", api_map_hash(api_map, total_size));
	fclose(file);
	printf("'godot_api.gen' generated!\n");
	return 0;
}
