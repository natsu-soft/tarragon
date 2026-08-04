#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "gdscript/token_ids.h"
#include "perfect_json.h"
#include "mem_sys.h"
#include "api_structs.h"
#include "strings.h"
#include "allocators/arena.h"
#include "allocators/scratch.h"
#include "hashmaps/api_map.h"
IMPL_UNARY_KEY
IMPL_BINARY_KEY

/* #define VERBOSE_FETCH */

#ifdef VERBOSE_FETCH
	#define FETCH_PRINT(MSG, ...) printf(MSG, ##__VA_ARGS__)
#else
	#define FETCH_PRINT(...)
#endif

#define PASSERT(A, MSG, ...) if (!(A)) { printf(MSG, ##__VA_ARGS__); fflush(stdout); assert(A); }

#define SCRUB_STR(STR, LEN)												\
	if (LEN > 12 && strncmp("typedarray::", STR, 12) == 0)				\
	{																	\
		STR = "Array";													\
		LEN = 5;														\
	}																	\
	else if (STR[LEN - 1] != '*')										\
	{																	\
		for (int32_t __i = LEN - 1; __i >= 0; --__i)					\
		{																\
			if (STR[__i] == ':')										\
			{															\
				STR += __i + 1;											\
				LEN -= __i + 1;											\
				break;													\
			}															\
		}																\
		/* else */ if (LEN == 12 && strncmp("Variant.Type", STR, 12) == 0) \
		{																\
			STR = "Variant";											\
			LEN = 7;													\
		}																\
		else if (LEN == 16 && strncmp("Variant.Operator", STR, 16) == 0) \
		{																\
			STR = "Variant:Operator";									\
			LEN = 16;													\
		}																\
		else if (LEN == 0)												\
		{																\
			STR = "Variant";											\
			LEN = 7;													\
		}																\
		for (int32_t __i = 0; __i < LEN; ++__i)							\
		{																\
			if (STR[__i] == ',')										\
			{															\
				STR = "Variant";										\
				LEN = 7;												\
				break;													\
			}															\
		}																\
	}																	\
	else																\
	{																	\
		if (LEN > 6 && strncmp("const ", STR, 6) == 0)					\
		{																\
			STR += 6;													\
			LEN -= 6;													\
		}																\
		if (LEN > 6 && strncmp("uint8_t", STR, 6) == 0)					\
		{																\
			STR = "C Buffer";											\
			LEN = 8;													\
		}																\
		else															\
		{																\
			STR = "C Pointer";											\
			LEN = 9;													\
		}																\
	}

#ifdef _WIN32
typedef struct _iobuf FILE;
#else
typedef struct _IO_FILE FILE;
#endif

FILE* src;
FILE* header;

static api_map_t* api_map = NULL;

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

#define X(_, A, B) A##_t* B = NULL;
	API_TAGS
#undef X


static const api_ptr_t api_fetch(const string* str)
{
	api_map_t* map = api_map;
	api_ptr_t res = {0};
	if (str->chars[0] != ' ')
	{
		int32_t i0 = 0;
		FETCH_PRINT("fetch: ");
		for (int32_t i = i0; i < str->length; ++i)
		{
			if (str->chars[i] != '.') { continue; }
			else
			{
				string temp = { .chars = str->chars + i0, .length = i - i0 };
				api_ptr_t p = api_map_fetch(map, &temp);
				FETCH_PRINT("%.*s -> ", temp.length , temp.value);
				PASSERT(api_tag(p) == API_CLASS, "'%.*s' should have fetched a class!\n", temp.length, temp.chars);
				map = api_ptr(p);
				i0 = i + 1;
			}
		}
		string temp = { .chars = str->chars + i0, .length = str->length - i0 };
		FETCH_PRINT("%.*s (%d:", temp.length, temp.value, temp.length);
		fflush(stdout);
		res = api_map_fetch(map, &temp);
		switch(res.tag)
		{
#define X(A, B, __) case API_##A: FETCH_PRINT(#B")\n"); break;
			API_TAGS
			#undef X
		default:
			FETCH_PRINT("ERR) -- PTR: %#018x\n", res.bits);
			#ifdef VERBOSE_FETCH
			fflush(stdout);
			assert(0);
			#else
			assert(0 && "catastrophic fetch error -- enable VERBOSE_FETCH in src/api_generator.c");
			#endif
		}
	}
	else // else it's an operator (since I prepend a space to the operator keys)
	{ // NOTE: operators map to the resultant type--either an enum or a class
		res = api_map_fetch(map, str);
		if (!(res.tag == API_CLASS || res.tag == API_ENUM || res.tag == API_NATIVE_STRUCT))
		{
			assert(0 && "operation keys SHOULD return either an enum or a class!");
		}
	}
	return res;
}


static arena_t map_arena = {0};
static arena_t str_arena = {0};

#define X(_, __, A) static int32_t num_##A = 0;
	API_TAGS
#undef X
#define X(_,__,A) static int32_t idx_##A = 0;
	API_TAGS
#undef X


static int32_t num_operators = 0; // NOTE: operators get cached in the base api map
static int32_t idx_operators = 0; // NOTE: operators get cached in the base api map
static int32_t str_size = 0;
static int32_t map_entries = 0;

#define X(_, __, A) static void accumulate_##A(json_t* json) { ++idx_##A; }
API_TAGS
#undef X

#define JSON_KEYS \
	X(NAME, name, 4) \
	X(TYPE, type, 4) \
	X(VALUE, value, 5) \
	X(VALUES, values, 6) \
	X(ARGUMENTS, arguments, 9) \
	X(DEFAULT_VALUE, default_value, 13) \
	X(METHODS, methods, 7) \
	X(MEMBERS, members, 7) \
	X(PROPERTIES, properties, 10) \
	X(CONSTANTS, constants, 9) \
	X(ENUMS, enums, 5) \
	X(SIGNALS, signals, 7) \
	X(CONSTRUCTORS, constructors, 12) \
	X(OPERATORS, operators, 9) \
	X(DESCRIPTION, description, 9) \
	X(BRIEF_DESCRIPTION, brief_description, 9) \
	X(RIGHT_TYPE, right_type, 10) \
	X(FORMAT, format, 6) \
	X(IS_BITFIELD, is_bitfield, 11) \
	X(RETURN_TYPE, return_type, 11) \
	X(RETURN_VALUE, return_value, 12) \
	X(IS_CONST, is_const, 8) \
	X(IS_VARARG, is_vararg, 9) \
	X(IS_STATIC, is_static, 9) \
	X(IS_VIRTUAL, is_virtual, 10) \
	X(INHERITS, inherits, 8) \
	X(IS_KEYED, is_keyed, 8) \
	X(IS_INSTANTIABLE, is_instantiable, 16)		 \
	X(HAS_DESTRUCTOR, has_destructor, 14) \
	/* X(UNARY, unary, 5) */ \
	X(INT, int, 3) \
	X(FLOAT, float, 5) \
	X(BOOL, bool, 4) \
	X(NIL, Nil, 3) \


enum
{
#define X(A, ...) A,
	JSON_KEYS
#undef X
};

static string strings[] =
{
#define X(_, A, N) STRINGN(#A, N),
	JSON_KEYS
	#undef X
};

static void accumulate_class_cache(json_t* json)
{
	string key;
	json_t dest = {0};
	uint8_t flags;
	while((flags = pj_next_pair(&key, &dest, json)))
	{
		if (stringeq(strings[METHODS], key))
		{
			pj_mapc_array(accumulate_funcs, &dest);
		}
		else if (stringeq(strings[MEMBERS], key) || stringeq(strings[PROPERTIES], key))
		{
			pj_mapc_array(accumulate_members, &dest);
		}
		else if(stringeq(strings[CONSTANTS], key))
		{
			pj_mapc_array(accumulate_constants, &dest);
		}
		else if(stringeq(strings[ENUMS], key))
		{
			pj_mapc_array(accumulate_enumerators, &dest);
		}
		else if(stringeq(strings[SIGNALS], key))
		{
			pj_mapc_array(accumulate_signals, &dest);
		}
	}
}

static class_t* hot_class = NULL;

// wtf is this used for again? AUDITING--DO NOT DELETE!
// as you can see it adds a .
static string hot_append_scratch(const string* str)
{
	const int32_t name_len = hot_class->name.length;
	const int total_len = name_len + str->length + 1;
	string res = STRINGN(scratch_alloc(total_len), total_len);
	memcpy(res.chars, hot_class->name.chars, name_len);
	res.chars[name_len] = '.';
	memcpy(res.chars + name_len + 1, str->chars, str->length);
	return res;
}

/* PHASE ONE */

static void p1_enum_values(json_t* json);

static void p1_enums(json_t* json)
{
	string str;
	json_t dest = {0};
	++num_enumerators;
	uint8_t flags;
	while((flags = pj_next_pair(&str, &dest, json)))
	{
		if (stringeq(strings[NAME], str))
		{
			pj_str(&str, &dest);
			str_size += str.length;
		}
		else if (stringeq(strings[VALUES], str))
		{
			pj_mapc_array(p1_enum_values, &dest);
		}
	}
}

static void p1_enum_values(json_t* json)
{
	string str;
	json_t dest = {0};
	++num_enum_values;
	uint8_t flags;
	while((flags = pj_next_pair(&str, &dest, json)))
	{
		if (stringeq(strings[NAME], str) /* || stringeq(STRING("description"), str) */)
		{
			pj_str(&str, &dest);
			str_size += str.length;
			return;
		}
	}
}

static void p1_params(json_t* json);

static void p1_funcs(json_t* json)
{
	string str;
	json_t dest = {0};
	++num_funcs;
	uint8_t flags;
	while((flags = pj_next_pair(&str, &dest, json)))
	{
		if (stringeq(strings[NAME], str) /* || stringeq("description", str, len) == 0 */)
		{
			pj_str(&str, &dest);
			str_size += str.length;
		}
		else if (stringeq(strings[ARGUMENTS], str))
		{
			pj_mapc_array(p1_params, &dest);
		}
	}
}

static void p1_params(json_t* json)
{
	string str;
	json_t dest = {0};
	++num_params;
	uint8_t flags;
	while((flags = pj_next_pair(&str, &dest, json)))
	{
		if (stringeq(strings[NAME], str))
		{
			pj_str(&str, &dest);
			str_size += str.length;
		}
		else if (stringeq(strings[DEFAULT_VALUE], str))
		{
			pj_str(&str, &dest);
			str_size += str.length;
		}
	}
}

static void p1_operators(json_t* json);
static void p1_constants(json_t* json);
static void p1_members(json_t* json);
static void p1_signals(json_t* json);

static void p1_classes(json_t* json)
{
	string s;
	json_t dest = {0};
	++num_classes;
	uint8_t flags;
	while((flags = pj_next_pair(&s, &dest, json)))
	{
		if (stringeq(strings[NAME], s))
		{
			pj_str(&s, &dest);
			str_size += s.length;
			/* char* name = scratch_alloc(len + 1); */
			/* copy_str(name, str, len); */
			/* printf("%s\n", name); */
		}
		else if (stringeq(strings[METHODS], s) || stringeq(strings[CONSTRUCTORS], s))
		{
			pj_mapc_array(p1_funcs, &dest);
		}
		else if (stringeq(strings[MEMBERS], s) || stringeq(strings[PROPERTIES], s))
		{
			pj_mapc_array(p1_members, &dest);
		}
		else if(stringeq(strings[CONSTANTS], s))
		{
			pj_mapc_array(p1_constants, &dest);
		}
		else if(stringeq(strings[ENUMS], s))
		{
			pj_mapc_array(p1_enums, &dest);
		}
		else if(stringeq(strings[SIGNALS], s))
		{
			pj_mapc_array(p1_signals, &dest);
		}
		/* else if(stringeq("description", str, len) == 0 || stringeq("brief_description", str, len) == 0) */
		/* { */
		/* /\* 	pj_str(&str, &len, &dest); *\/ */
		/* /\* 	num_chars += len; *\/ */
		/* } */
		else if(stringeq(strings[OPERATORS], s))
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
	string s;
	json_t dest = {0};
	uint8_t flags;
	++num_operators;
	str_size += sizeof(char) + sizeof(token_id_t) + sizeof(api_ptr_t);
	while((flags = pj_next_pair(&s, &dest, json)))
	{
		if (stringeq(strings[RIGHT_TYPE], s))
		{
			str_size += sizeof(api_ptr_t);
			return;
		}
	}
}

// TODO: const value parsing
static void p1_constants(json_t* json)
{
	string s;
	json_t dest = {0};
	++num_constants;
	uint8_t flags;
	while((flags = pj_next_pair(&s, &dest, json)))
	{
		if (stringeq(strings[NAME], s) /* || stringeq("description", str, len) == 0 */)
		{
			pj_str(&s, &dest);
			str_size += s.length;
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
	string s;
	json_t dest = {0};
	++num_members;
	uint8_t flags;
	while((flags = pj_next_pair(&s, &dest, json)))
	{
		if (stringeq(strings[NAME], s) /* || stringeq("description", str, len) == 0 */)
		{
			pj_str(&s, &dest);
			str_size += s.length;
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
	string s;
	json_t dest = {0};
	uint8_t flags;
	while((flags = pj_next_pair(&s, &dest, json)))
	{
		if (stringeq(strings[NAME], s) /* || stringeq("description", str, len) == 0 */)
		{
			pj_str(&s, &dest);
			str_size += s.length;
		}
		else if (stringeq(strings[ARGUMENTS], s))
		{
			pj_mapc_array(p1_params, &dest);
		}
	}
	++num_signals;
}

static void p1_native_structs(json_t* json)
{
	string s;
	json_t dest = {0};
	uint8_t flags;
	while((flags = pj_next_pair(&s, &dest, json)))
	{
		if (stringeq(strings[NAME], s) /* || stringeq("description", str, len) == 0 */)
		{
			pj_str(&s, &dest);
			str_size += s.length;
		}
		else if (stringeq(strings[FORMAT], s))
		{
			pj_str(&s, &dest);
			str_size += s.length;
		}
	}
	++num_native_structs;
}

/* PHASE DEUCE */

static void p2_classes(json_t* json)
{
#define X(_, __, A) idx_##A = 0;
	API_TAGS
#undef X
	accumulate_class_cache(json);
	if (idx_enumerators || idx_funcs || idx_constants || idx_members || idx_signals)
	{
		map_entries += hashmap_size(idx_enumerators + idx_funcs + idx_constants + idx_members + idx_signals);
	}
}

/* PHASE DEUCE(B) */

static void p2b_classes(json_t* json)
{
	idx_funcs = 0;
	idx_enumerators = 0;
	idx_members = 0;
	idx_constants = 0;
	idx_signals = 0;
	accumulate_class_cache(json);
	if (idx_enumerators || idx_funcs || idx_constants || idx_members || idx_signals)
	{
		const int32_t map_cap = hashmap_size(idx_enumerators + idx_funcs + idx_constants + idx_members + idx_signals);
		/* class->class_map.cap = map_cap; */
		/* const int32_t map_size = map_cap * (2 * sizeof(self_ptr_t) + sizeof(int32_t)); */
		/* api_map_back(&class->class_map, arena_alloc(&map_arena, map_size)); */
		/* printf("map_arena: %d/%d -> ", map_arena.size, map_arena.cap); */
		class_t* class = classes + idx_classes;
		assert(class && "wtf this is null\n");
		api_map_back(class, &map_arena, map_cap);
		/* printf("%d/%d\n", map_arena.size, map_arena.cap); */
	}
	++idx_classes;
}


/* PHASE THREE: CACHE EVERYTHING NOW! */

static void p3_global_enums(json_t* json)
{
	string s;
	json_t dest = {0};
	uint8_t flags;
	enumerator_t* e = enumerators + idx_enumerators;
	while((flags = pj_next_pair(&s, &dest, json)))
	{
		if (stringeq(strings[NAME], s))
		{
			pj_str(&s, &dest);
			SCRUB_STR(s.chars, s.length);
			string name = { .chars=arena_push(&str_arena, s.chars, s.length), .length=s.length };
			api_ptr_t ptr = api_map_cache(api_map, &name, flag_api_ptr(e, API_ENUM | GODOT_API));
			PASSERT(api_ptr(ptr) == e, "%lx != %lx\n\tRAW: %lx\n", (uint64_t)api_ptr(ptr), (uint64_t)e, ptr.bits);
		}
		else if (stringeq(strings[VALUES], s))
		{
			e->num_values = pj_array_size(&dest);
		}
	}
	++idx_enumerators;
}

static void p3_global_funcs(json_t* json)
{
	string s;
	json_t dest = {0};
	uint8_t flags;
	func_t* func = funcs + idx_funcs;
	while((flags = pj_next_pair(&s, &dest, json)))
	{
		if (stringeq(strings[NAME], s))
		{
			pj_str(&s, &dest);
			func->name = STRINGN(arena_push(&str_arena, s.chars, s.length), s.length);
			api_ptr_t ptr = api_map_cache(api_map, &(func->name), flag_api_ptr(func, API_FUNC | GODOT_API));
			assert(api_fetch(&(func->name)).func == ptr.func);
		}
		else if (stringeq(strings[ARGUMENTS], s))
		{
			func->num_params = pj_array_size(&dest);
		}
	}
	++idx_funcs;
}

static void p3_enums(json_t* json);
static void p3_funcs(json_t* json);
static void p3_constructors(json_t* json);
static void p3_members(json_t* json);
static void p3_constants(json_t* json);
static void p3_signals(json_t* json);

static void p3_classes(json_t* json)
{
	string s;
	json_t dest = {0};
	uint8_t flags;
	class_t* class = classes + idx_classes;
	hot_class = class;
	while((flags = pj_next_pair(&s, &dest, json)))
	{
		if (stringeq(strings[NAME], s))
		{
			pj_str(&s, &dest);
			string name = STRINGN(arena_push(&str_arena, s.chars, s.length), s.length);
			class->name = name;
			api_map_cache(api_map, &name, flag_api_ptr(class, API_CLASS | GODOT_API));
			api_ptr_t p = api_fetch(&name);
			enum api_tag tag = api_tag(p);
			assert(class == api_ptr(p));
			assert(tag == API_CLASS);
		}
		else if (stringeq(strings[ENUMS], s))
		{
			pj_mapc_array(p3_enums, &dest);
		}
		else if (stringeq(strings[METHODS], s))
		{
			pj_mapc_array(p3_funcs, &dest);
		}
		else if (stringeq(strings[CONSTRUCTORS], s))
		{
			class->num_constructors = pj_array_size(&dest);
			dest.index = 0;
			class->constructors = funcs + idx_funcs;
			pj_mapc_array(p3_constructors, &dest);
		}
		else if (stringeq(strings[MEMBERS], s) || stringeq(strings[PROPERTIES], s))
		{
			pj_mapc_array(p3_members, &dest);
		}
		else if (stringeq(strings[CONSTANTS], s))
		{
			pj_mapc_array(p3_constants, &dest);
		}
		else if (stringeq(strings[SIGNALS], s))
		{
			pj_mapc_array(p3_signals, &dest);
		}
	}
	++idx_classes;
}


static void p3_enums(json_t* json)
{
	string s;
	json_t dest = {0};
	uint8_t flags;
	enumerator_t* e = enumerators + idx_enumerators;
	while((flags = pj_next_pair(&s, &dest, json)))
	{
		if (stringeq(strings[NAME], s))
		{
			pj_str(&s, &dest);
			string name = STRINGN(arena_push(&str_arena, s.chars, s.length), s.length);
			/* printf("cached: %.*s.%.*s\n", */
			/* 	   hot_class->name.length, hot_class->name.value, */
			/* 	   name.length, name.value); */
			api_ptr_t p = api_map_cache(hot_class, &name, flag_api_ptr(e, API_ENUM | GODOT_API));
			/* printf("\t%#018lx -> %#018lx\n", (uint64_t)e, p.bits); */
			assert(api_tag(p) == API_ENUM);
			assert(api_ptr(p) == e);
		}
		else if (stringeq(strings[VALUES], s))
		{
			idx_enum_values = 0;
			pj_mapc_array(accumulate_enum_values, &dest);
			e->num_values = idx_enum_values;
		}
	}
	++idx_enumerators;
}

static void p3_funcs(json_t* json)
{
	string s;
	json_t dest = {0};
	uint8_t flags;
	func_t* func = funcs + idx_funcs;
	while((flags = pj_next_pair(&s, &dest, json)))
	{
		if (stringeq(strings[NAME], s))
		{
			pj_str(&s, &dest);
			string name = STRINGN(arena_push(&str_arena, s.chars, s.length), s.length);
			api_ptr_t ptr = api_map_cache((api_map_t*)hot_class, &name, flag_api_ptr(func, API_FUNC | GODOT_API));

			/* printf("%x: (%02x,%02x)@%016lx\n", API_FUNC | GODOT_API, ptr.metadata, ptr.tag, ptr); */
			assert(api_metadata(ptr) == GODOT_API);
			func->name = name;
		}
		else if (stringeq(strings[ARGUMENTS], s))
		{
			func->num_params = pj_array_size(&dest);
			/* idx_params = 0; */
			/* pj_mapc_array(accumulate_params, &dest); */
			/* func->idx_params = idx_params; */
		}
	}
	++idx_funcs;
}

static void p3_constructors(json_t* json)
{
	string s;
	json_t dest = {0};
	uint8_t flags;
	func_t* func = funcs + idx_funcs;
	while((flags = pj_next_pair(&s, &dest, json)))
	{
		if (stringeq(strings[ARGUMENTS], s))
		{
			func->num_params = pj_array_size(&dest);
			/* idx_params = 0; */
			/* pj_mapc_array(accumulate_params, &dest); */
			/* func->idx_params = idx_params; */
		}
	}
	++idx_funcs;
}

static void p3_members(json_t* json)
{
	string s;
	json_t dest = {0};
	uint8_t flags;
	while((flags = pj_next_pair(&s, &dest, json)))
	{
		if (stringeq(strings[NAME], s))
		{
			pj_str(&s, &dest);
			string name = STRINGN(arena_push(&str_arena, s.chars, s.length), s.length);
			api_ptr_t ptr = api_map_cache((api_map_t*)hot_class, &name, flag_api_ptr(members + idx_members, API_MEMBER | GODOT_API));
			++idx_members;
			return;
		}
	}
}

static void p3_constants(json_t* json)
{
	string s;
	json_t dest = {0};
	uint8_t flags;
	constant_t* constant = constants + idx_constants;
	while((flags = pj_next_pair(&s, &dest, json)))
	{
		if (stringeq(strings[NAME], s))
		{
			pj_str(&s, &dest);
			string name = STRINGN(arena_push(&str_arena, s.chars, s.length), s.length);
			api_map_cache((api_map_t*)hot_class, &name, flag_api_ptr(constants + idx_constants, API_CONSTANT | GODOT_API));
			++idx_constants;
			return;
		}
	}
}

static void p3_signals(json_t* json)
{
	string s;
	json_t dest = {0};
	uint8_t flags;
	signal_t* signal = signals + idx_signals;
	while((flags = pj_next_pair(&s, &dest, json)))
	{
		if (stringeq(strings[NAME], s))
		{
			pj_str(&s, &dest);
			string name = STRINGN(arena_push(&str_arena, s.chars, s.length), s.length);
			api_map_cache((api_map_t*)hot_class, &name, flag_api_ptr(signal, API_SIGNAL | GODOT_API));
		}
		else if (stringeq(strings[ARGUMENTS], s))
		{
			idx_params = 0;
			pj_mapc_array(accumulate_params, &dest);
			signal->num_params = idx_params;
		}
	}
	++idx_signals;
}

static void p3_native_structs(json_t* json)
{
	string s;
	json_t dest = {0};
	uint8_t flags;
	native_struct_t* native = native_structs + idx_native_structs;
	while((flags = pj_next_pair(&s, &dest, json)))
	{
		if (stringeq(strings[NAME], s))
		{
			pj_str(&s, &dest);
			/* string name = { .chars=arena_push(&str_arena, str, len), .length=len }; */
			string name = STRINGN(arena_push(&str_arena, s.chars, s.length), s.length);
			api_ptr_t ptr = api_map_cache(api_map, &name, flag_api_ptr(native, API_NATIVE_STRUCT | GODOT_API));
			assert(api_ptr(api_fetch(&name)) == native);
		}
		else if (stringeq(strings[FORMAT], s))
		{
			pj_str(&s, &dest);
			native->format = STRINGN(arena_push(&str_arena, s.chars, s.length), s.length);
		}
	}
	++idx_native_structs;
}

static void print_name(json_t* json)
{
	json_t dest = pj_find_key(&strings[NAME], json);
	if (dest.num_tokens > 0 && (dest.bitmasks[0] & 0x0f) == JSON_STRING)
	{
		string name;
		pj_str(&name, &dest);
		printf("> %.*s\n", name.length, name.chars);
	}
}


/* PHASE FOUR */

static void p4_enum_values(json_t* json);

static void p4_enums(json_t* json)
{
	string s;
	json_t dest = {0};
	uint8_t flags;
	while((flags = pj_next_pair(&s, &dest, json)))
	{
		if (stringeq(strings[VALUES], s))
		{
			enumerators[idx_enumerators].values = enum_values + idx_enum_values;
			pj_mapc_array(p4_enum_values, &dest);
		}
		else if (stringeq(strings[IS_BITFIELD], s) && (flags & 0x0f) == JSON_TRUE)
		{
			enumerators[idx_enumerators].is_bitfield = 0xff;
		}
	}
	++idx_enumerators;
}

static void p4_enum_values(json_t* json)
{
	string s;
	json_t dest = {0};
	uint8_t flags;
	enum_value_t* val = enum_values + idx_enum_values;
	while((flags = pj_next_pair(&s, &dest, json)))
	{
		if (stringeq(strings[NAME], s))
		{
			pj_str(&s, &dest);
			val->name = STRINGN(arena_push(&str_arena, s.chars, s.length), s.length);
		}
		else if (stringeq(strings[VALUE], s))
		{
			pj_int(&val->value, &dest);
		}
	}
	++idx_enum_values;
}

static void p4_params(json_t* json);

static void p4_funcs(json_t* json)
{
	string s;
	json_t dest = {0};
	uint8_t flags;
	func_t* func = funcs + idx_funcs;
	while((flags = pj_next_pair(&s, &dest, json)))
	{
		if (stringeq(strings[RETURN_TYPE], s) ||
			(stringeq(strings[RETURN_VALUE], s) && (dest = pj_find_key(&STRING("type"), &dest), dest.src)))
		{
			pj_str(&s, &dest);
			SCRUB_STR(s.chars, s.length)
			/* func->return_type = api_fetch(&STRINGN(str, len)); */
			func->return_type = api_fetch(&s);
			assert(func->return_type.ptr);
		}
		/* else if (stringeq(strings[RETURN_VALUE], s)) */
		/* { */
		/* 	dest = pj_find_key(&STRING("type"), &dest); */
		/* 	pj_str(&s, &dest); */
		/* 	SCRUB_STR(s.chars, s.length) */
		/* 	/\* char* temp = scratch_alloc(len + 1); *\/ */
		/* 	/\* cpystr(temp, str, len); *\/ */
		/* 	func->return_type = api_fetch(&s); */
		/* 	assert(func->return_type.ptr); */
		/* } */
		else if (stringeq(strings[ARGUMENTS], s))
		{
			func->params = params + idx_params;
			pj_mapc_array(p4_params, &dest);
		}
		else if (stringeq(strings[IS_CONST], s) && (flags & 0x0f) == JSON_TRUE)
		{
			func->bitflags |= FUNC_CONST;
		}
		else if (stringeq(strings[IS_VARARG], s) && (flags & 0x0f) == JSON_TRUE)
		{
			func->bitflags |= FUNC_VARIADIC;
		}
		else if (stringeq(strings[IS_STATIC], s) && (flags & 0x0f) == JSON_TRUE)
		{
			func->bitflags |= FUNC_STATIC;
		}
		else if (stringeq(strings[IS_VIRTUAL], s) && (flags & 0x0f) == JSON_TRUE)
		{
			func->bitflags |= FUNC_VIRTUAL;
		}
	}
	++idx_funcs;
}

static void p4_params(json_t* json)
{
	string s;
	json_t dest = {0};
	uint8_t flags;
	param_t* param = params + idx_params;
	while((flags = pj_next_pair(&s, &dest, json)))
	{
		if (stringeq(strings[NAME], s))
		{
			pj_str(&s, &dest);
			string name = STRINGN(arena_push(&str_arena, s.chars, s.length), s.length);
			param->name = name;
			/* printf("param: %.*s\n", name.length, name.value); */
		}
		else if (stringeq(strings[TYPE], s))
		{
			pj_str(&s, &dest);
			/* printf("param type: '%.*s' ->", len, str); */
			SCRUB_STR(s.chars, s.length);
			/* printf("'%.*s'\n", len, str); */
			string query = STRINGN(s.chars, s.length);
			/* fflush(stdout); */
			param->type = api_fetch(&query);
			assert(param->type.bits);
		}
		else if (stringeq(strings[DEFAULT_VALUE], s))
		{
			pj_str(&s, &dest);
			string value = STRINGN(arena_push(&str_arena, s.chars, s.length), s.length);
			param->default_value = value;
		}
	}
	++idx_params;
}

static void p4_members(json_t* json);
static void p4_constructors(json_t* json);
static void p4_constants(json_t* json);
static void p4_signals(json_t* json);
static void p4_operators(json_t* json);

static void p4_classes(json_t* json)
{
	string s;
	json_t dest = {0};
	uint8_t flags;
	class_t* class = classes + idx_classes;
	hot_class = class;
	class->inherits = NULL;
	/* hot_diff = diff_relate(class, classes); */
	/* hot_diff.delta |= API_CLASS; */
	while((flags = pj_next_pair(&s, &dest, json)))
	{
		if (stringeq(strings[INHERITS], s))
		{
			pj_str(&s, &dest);
			string parent = s;
			api_ptr_t ptr = api_fetch(&parent);
			PASSERT(api_tag(ptr) == API_CLASS, "fetched value for '%.*s' is not a class!\n", parent.length, parent.chars);
			class->inherits = api_ptr(ptr);
		}
		else if (stringeq(strings[METHODS], s))
		{
			pj_mapc_array(p4_funcs, &dest);
		}
		else if (stringeq(strings[CONSTRUCTORS], s))
		{
			pj_mapc_array(p4_constructors, &dest);
		}
		else if (stringeq(strings[MEMBERS], s) || stringeq(strings[PROPERTIES], s))
		{
			pj_mapc_array(p4_members, &dest);
		}
		else if(stringeq(strings[CONSTANTS], s))
		{
			pj_mapc_array(p4_constants, &dest);
		}
		else if(stringeq(strings[ENUMS], s))
		{
			pj_mapc_array(p4_enums, &dest);
		}
		else if(stringeq(strings[SIGNALS], s))
		{
			pj_mapc_array(p4_signals, &dest);
		}
		else if (stringeq(strings[IS_KEYED], s) && (flags & 0x0f) == JSON_TRUE)
		{
			class->bitflags |= CLASS_IS_KEYED;
		}
		else if (stringeq(strings[IS_INSTANTIABLE], s) && (flags & 0x0f) == JSON_TRUE)
		{
			class->bitflags |= CLASS_IS_INSTANTIABLE;
		}
		else if (stringeq(strings[HAS_DESTRUCTOR], s) && (flags & 0x0f) == JSON_TRUE)
		{
			class->bitflags |= CLASS_HAS_DESTRUCTOR;
		}
		else if(stringeq(strings[OPERATORS], s))
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
	++idx_classes;
}

static void p4_members(json_t* json)
{
	json_t dest = pj_find_key(&strings[TYPE], json);
	string s;
	pj_str(&s, &dest);
	SCRUB_STR(s.chars, s.length)
	api_ptr_t ptr = api_fetch(&s);
	assert(ptr.ptr);
	members[idx_members].type = ptr;
	idx_members++;
}

static void p4_constructors(json_t* json)
{
	funcs[idx_funcs].return_type = flag_api_ptr(hot_class, API_CLASS | GODOT_API);
	json_t dest = pj_find_key(&strings[ARGUMENTS], json);
	if (dest.num_tokens > 0 && (dest.bitmasks[0] & 0x0f) == JSON_ARRAY)
	{
		funcs[idx_funcs].params = params + idx_params;
		pj_mapc_array(p4_params, &dest);
	}
	++idx_funcs;
}

static void p4_constants(json_t* json)
{
	string s;
	json_t dest = {0};
	uint8_t flags;
	constant_t* constant = constants + idx_constants;
	while ((flags = pj_next_pair(&s, &dest, json)))
	{
		if (stringeq(strings[TYPE], s))
		{
			pj_str(&s, &dest);
			SCRUB_STR(s.chars, s.length)
			constant->type = api_fetch(&s);
			assert(constant->type.ptr);
			/* assert(api_fetch(&s).ptr == constant->type.ptr); */
			break;
		}
		else if (stringeq(strings[VALUE], s))
		{
			assert(dest.num_tokens == 1 && "p4_constants(): wtf is this???");
			switch (flags & 0x0f)
			{
			case JSON_INTEGER:
				constant->type = api_fetch(&STRINGN("int", 3));
				break;
			case JSON_FLOAT:
				constant->type = api_fetch(&STRINGN("float", 5));
				break;
			case JSON_TRUE: __attribute__((fallthrough));
			case JSON_FALSE:
				constant->type = api_fetch(&STRINGN("bool", 4));
				break;
			case JSON_NULL:
				constant->type = api_fetch(&STRINGN("Nil", 3));
				break;
			default:
				PASSERT(0, "unprocessed value in p4_constant: %#x\n", flags & 0x0f);
				break;
			}
		}
	}
	++idx_constants;
	return;
}

static void p4_signals(json_t* json)
{
	json_t dest = pj_find_key(&strings[ARGUMENTS], json);
	if (dest.num_tokens > 0 && (dest.bitmasks[0] & 0x0f) == JSON_ARRAY)
	{
		signals[idx_signals].params = params + idx_params;
		pj_mapc_array(p4_params, &dest);
	}
	++idx_signals;
}

static void p4_operators(json_t* json)
{
	string s;
	json_t dest = {0};
	uint8_t flags;
	token_id_t operator = GDSCRIPT_NONE;
	api_ptr_t lhs = flag_api_ptr(hot_class, API_CLASS | GODOT_API);
	api_ptr_t rhs = {0};
	api_ptr_t ret = {0};
	while((flags = pj_next_pair(&s, &dest, json)))
	{
		if (stringeq(strings[NAME], s))
		{
			pj_str(&s, &dest);
			/* printf("\t%s ", temp); */
			/* if (stringeq(strings[UNARY], s)) */
			if (strncmp("unary", s.chars, 5) == 0) // you ACTUALLY want strncmp here
			{
				operator = s.chars[5] == '+' ? GDSCRIPT_OP_IDENTITY : GDSCRIPT_OP_NEGATION;
			}
			else
			{
				operator = builtin_operators(s.chars, s.length);
				PASSERT(operator != GDSCRIPT_NONE, "invalid operator: '%.*s'\n", s.length, s.chars);
			}
		}
		else if (stringeq(strings[RETURN_TYPE], s))
		{
			pj_str(&s, &dest);
			SCRUB_STR(s.chars, s.length);
			const api_ptr_t res = api_fetch(&s);
			assert(api_tag(res) != API_NONE);
			ret = res;
			assert(ret.ptr && "failed to determine operator return type!");
		}
		else if (stringeq(strings[RIGHT_TYPE], s))
		{
			pj_str(&s, &dest);
			SCRUB_STR(s.chars, s.length);
			const api_ptr_t res = api_fetch(&s);
			assert(api_tag(res) != API_NONE);
			rhs = res;
		}
	}
	string key;
	if (rhs.ptr)
	{
		union binary_key binary =
		{
			.operator = operator,
			.lhs = lhs,
			.rhs = rhs
		};
		/* printf("operator: %02x%016lx%016lx -> %016lx\n", operator, lhs, rhs, ret); */
		key = STRINGN(arena_push(&str_arena, binary.bits, sizeof binary), sizeof binary);
	}
	else
	{
		union unary_key unary =
		{
			.operator = operator,
			.operand = lhs
		};
		/* printf("operator: %02x%016lx -> %016lx\n", operator, lhs, ret); */
		key = STRINGN(arena_push(&str_arena, unary.bits, sizeof unary), sizeof unary);
	}
	key.chars[0] = ' ';
	api_map_cache(api_map, &key, ret);
}

static void p4_singletons(json_t* json)
{
	json_t dest = pj_find_key(&strings[TYPE], json);
	string s;
	pj_str(&s, &dest);
	const api_ptr_t ptr = api_fetch(&s);
	assert(api_tag(ptr) == API_CLASS && "api_fetch() did not return a valid class in p4_singletons()!");
	class_t* class = api_ptr(ptr);
	/* char* temp = scratch_alloc(class->name_len + 1); */
	/* cpystr(temp, abs_self(&class->name_selfptr), class->name_len); */
	/* printf("class->name (%d): %s\n", class->name_len, temp); */
	class->bitflags |= CLASS_IS_SINGLETON;
}

#ifndef DO_NOT_AUDIT

static void audit_enum_values(json_t* json);

static void audit_global_enums(json_t* json)
{
	string s;
	json_t dest = {0};
	uint8_t flags;
	enumerator_t* e = enumerators + idx_enumerators;
	while ((flags = pj_next_pair(&s, &dest, json)))
	{
		if (stringeq(strings[NAME], s))
		{
			pj_str(&s, &dest);
			SCRUB_STR(s.chars, s.length)
			api_ptr_t temp = api_fetch(&s);
			assert(api_tag(temp) == API_ENUM);
			assert(api_ptr(temp) == e);
		}
		else if (stringeq(strings[VALUES], s))
		{
			pj_mapc_array(audit_enum_values, &dest);
		}
		else if (stringeq(strings[IS_BITFIELD], s))
		{
			uint8_t is_bitfield = (flags & 0x0f) == JSON_TRUE ? 0xff : 0x00;
			PASSERT(e->is_bitfield == is_bitfield, "saved: %#x, expected: %#x\n", e->is_bitfield, is_bitfield);
		}
	}
	idx_enumerators++;
}

static void audit_enum_values(json_t* json)
{
	string s;
	json_t dest = {0};
	uint8_t flags;
	enum_value_t* val = enum_values + idx_enum_values;
	while((flags = pj_next_pair(&s, &dest, json)))
	{
		if (stringeq(strings[NAME], s))
		{
			pj_str(&s, &dest);
			assert(stringeq(s, val->name));
		}
		else if (stringeq(strings[VALUE], s))
		{
			int ground_truth;
			pj_int(&ground_truth, &dest);
			assert(ground_truth == val->value);
		}
	}
	++idx_enum_values;
}

static void audit_params(json_t* json);

static void audit_global_funcs(json_t* json)
{
	string s;
	json_t dest = {0};
	uint8_t flags;
	func_t* func = funcs + idx_funcs;
	while((flags = pj_next_pair(&s, &dest, json)))
	{
		if (stringeq(strings[NAME], s))
		{
			pj_str(&s, &dest);
			SCRUB_STR(s.chars, s.length)
			api_ptr_t temp = api_fetch(&s);
			assert(api_tag(temp) == API_FUNC);
			assert(api_ptr(temp) == func);
		}
		else if (stringeq(strings[RETURN_TYPE], s) ||
				 (stringeq(strings[RETURN_VALUE], s) &&
				  (dest = pj_find_key(&strings[TYPE], &dest), dest.src)))
		{
			pj_str(&s, &dest);
			SCRUB_STR(s.chars, s.length)
			api_ptr_t temp = api_fetch(&s);
			assert(func->return_type.bits == temp.bits);
		}
		/* else if (stringeq("return_value", str, len) == 0) */
		/* { */
		/* 	dest = pj_find_key("type", &dest); */
		/* 	pj_str(&str, &len, &dest); */
		/* 	SCRUB_STR(str, len) */
		/* 	api_ptr_t temp = api_fetch(&STRINGN(str, len)); */
		/* 	assert(func->return_type.bits == temp.bits); */
		/* } */
		else if (stringeq(strings[ARGUMENTS], s))
		{
			pj_mapc_array(audit_params, &dest);
		}
		else if (stringeq(strings[IS_CONST], s))
		{
			const uint8_t mask = (flags & 0x0f) == JSON_TRUE ? FUNC_CONST : 0x00;
			assert((func->bitflags & FUNC_CONST) == mask);
		}
		else if (stringeq(strings[IS_VARARG], s))
		{
			const uint8_t mask = (flags & 0x0f) == JSON_TRUE ? FUNC_VARIADIC : 0x00;
			assert((func->bitflags & FUNC_VARIADIC) == mask);
		}
		else if (stringeq(strings[IS_STATIC], s))
		{
			const uint8_t mask = (flags & 0x0f) == JSON_TRUE ? FUNC_STATIC : 0x00;
			assert((func->bitflags & FUNC_STATIC) == mask);
		}
		else if (stringeq(strings[IS_VIRTUAL], s))
		{
			const uint8_t mask = (flags & 0x0f) == JSON_TRUE ? FUNC_VIRTUAL : 0x00;
			assert((func->bitflags & FUNC_VIRTUAL) == mask);
		}
	}
	++idx_funcs;
}

static void audit_params(json_t* json)
{
	string s;
	json_t dest = {0};
	uint8_t flags;
	param_t* param = params + idx_params;
	while((flags = pj_next_pair(&s, &dest, json)))
	{
		if (stringeq(strings[NAME], s))
		{
			pj_str(&s, &dest);
			assert(stringeq(param->name, s));
		}
		else if (stringeq(strings[TYPE], s))
		{
			pj_str(&s, &dest);
			SCRUB_STR(s.chars, s.length)
			api_ptr_t temp = api_fetch(&s);
			assert(param->type.bits == temp.bits);
		}
		else if (stringeq(strings[DEFAULT_VALUE], s))
		{
			pj_str(&s, &dest);
			PASSERT(stringeq(param->default_value, s), "default_value: '%.*s' != '%.*s'",
					param->default_value.length, param->default_value.chars,
					s.length, s.chars);
		}
	}
	++idx_params;
}

static void audit_methods(json_t* json);
static void audit_constructors(json_t* json);
static void audit_members(json_t* json);
static void audit_enums(json_t* json);
static void audit_signals(json_t* json);
static void audit_constants(json_t* json);

static void audit_classes(json_t* json)
{
	string s;
	json_t dest = {0};
	uint8_t flags;
	class_t* class = classes + idx_classes;
	hot_class = class;
	while((flags = pj_next_pair(&s, &dest, json)))
	{
		if (stringeq(strings[NAME], s))
		{
			pj_str(&s, &dest);
			assert(stringeq(hot_class->name, s));
			api_ptr_t temp = api_fetch(&s);
			assert(api_tag(temp) == API_CLASS);
			assert(class == api_ptr(temp));
		}
		else if (stringeq(strings[INHERITS], s))
		{
			pj_str(&s, &dest);
			api_ptr_t temp = api_fetch(&s);
			assert(api_tag(temp) == API_CLASS);
			class->inherits = api_ptr(temp);
			assert(classes <= class->inherits);
			assert(class->inherits < classes + num_classes);
		}
		else if (stringeq(strings[METHODS], s))
		{
			pj_mapc_array(audit_methods, &dest);
		}
		else if (stringeq(strings[CONSTRUCTORS], s))
		{
			pj_mapc_array(audit_constructors, &dest);
		}
		else if (stringeq(strings[MEMBERS], s) || stringeq(strings[PROPERTIES], s))
		{
			pj_mapc_array(audit_members, &dest);
		}
		else if(stringeq(strings[CONSTANTS], s))
		{
			pj_mapc_array(audit_constants, &dest);
		}
		else if(stringeq(strings[ENUMS], s))
		{
			pj_mapc_array(audit_enums, &dest);
		}
		else if(stringeq(strings[SIGNALS], s))
		{
			pj_mapc_array(audit_signals, &dest);
		}
		else if (stringeq(strings[IS_KEYED], s))
		{
			const uint8_t mask = (flags & 0x0f) == JSON_TRUE ? CLASS_IS_KEYED : 0x00;
			assert((class->bitflags & CLASS_IS_KEYED) == mask);
		}
		else if (stringeq(strings[IS_INSTANTIABLE], s))
		{
			const uint8_t mask = (flags & 0x0f) == JSON_TRUE ? CLASS_IS_INSTANTIABLE : 0x00;
			assert((class->bitflags & CLASS_IS_INSTANTIABLE) == mask);
		}
		else if (stringeq(strings[HAS_DESTRUCTOR], s))
		{
			const uint8_t mask = (flags & 0x0f) == JSON_TRUE ? CLASS_HAS_DESTRUCTOR : 0x00;
			assert((class->bitflags & CLASS_HAS_DESTRUCTOR) == mask);
		}
	}
	++idx_classes;
}

// TODO: SHITS FUCKED, YO.
static void audit_methods(json_t* json)
{
	string s;
	json_t dest = {0};
	uint8_t flags;
	func_t* func = funcs + idx_funcs;
	while((flags = pj_next_pair(&s, &dest, json)))
	{
		if (stringeq(strings[NAME], s))
		{
			pj_str(&s, &dest);
			SCRUB_STR(s.chars, s.length);
			string query = hot_append_scratch(&s);
			assert(func == api_ptr(api_fetch(&query)));
		}
		else if (stringeq(strings[RETURN_TYPE], s) ||
				 (stringeq(strings[RETURN_VALUE], s) && (dest = pj_find_key(&strings[TYPE], &dest), dest.src)))
		{
			pj_str(&s, &dest);
			SCRUB_STR(s.chars, s.length)
			const api_ptr_t temp = api_fetch(&s);
			PASSERT(api_tag(temp) != API_NONE, "bad fetch: %#x: %x\n", api_ptr(temp), api_tag(temp));
			PASSERT(func->return_type.bits == temp.bits, "%#x != %#x\n", func->return_type.bits, temp.bits);
		}
		/* else if (stringeq("return_value", str, len)) */
		/* { */
		/* 	dest = pj_find_key("type", &dest); */
		/* 	pj_str(&str, &len, &dest); */
		/* 	SCRUB_STR(str, len) */
		/* 	const api_ptr_t temp = api_fetch(&STRINGN(str, len)); */
		/* 	PASSERT(api_tag(temp) != API_NONE, "bad fetch: %#x: %x\n", api_ptr(temp), api_tag(temp)); */
		/* 	PASSERT(func->return_type.bits == temp.bits, "%#x != %#x\n", func->return_type.bits, temp.bits); */
		/* } */
		else if (stringeq(strings[ARGUMENTS], s))
		{
			pj_mapc_array(audit_params, &dest);
		}
		else if (stringeq(strings[IS_CONST], s))
		{
			const uint8_t mask = (flags & 0x0f) == JSON_TRUE ? FUNC_CONST : 0x00;
			assert((func->bitflags & FUNC_CONST) == mask);
		}
		else if (stringeq(strings[IS_VARARG], s))
		{
			const uint8_t mask = (flags & 0x0f) == JSON_TRUE ? FUNC_VARIADIC : 0x00;
			assert((func->bitflags & FUNC_VARIADIC) == mask);
		}
		else if (stringeq(strings[IS_STATIC], s))
		{
			const uint8_t mask = (flags & 0x0f) == JSON_TRUE ? FUNC_STATIC : 0x00;
			assert((func->bitflags & FUNC_STATIC) == mask);
		}
		else if (stringeq(strings[IS_VIRTUAL], s))
		{
			const uint8_t mask = (flags & 0x0f) == JSON_TRUE ? FUNC_VIRTUAL : 0x00;
			assert((func->bitflags & FUNC_VIRTUAL) == mask);
		}
	}
	++idx_funcs;
}


// GOOD
static void audit_constructors(json_t* json)
{
	assert(api_ptr(funcs[idx_funcs].return_type) == hot_class);
	json_t dest = pj_find_key(&strings[ARGUMENTS], json);
	if (dest.num_tokens > 0 && (dest.bitmasks[0] & 0x0f) == JSON_ARRAY)
	{
		pj_mapc_array(audit_params, &dest);
	}
	++idx_funcs;
}

// SOLID
static void audit_members(json_t* json)
{
	string s;
	json_t dest = {0};
	uint8_t flags;
	member_t* member = members +  idx_members;
	while((flags = pj_next_pair(&s, &dest, json)))
	{
		if (stringeq(strings[NAME], s))
		{
			pj_str(&s, &dest);
			string query = hot_append_scratch(&s);
			PASSERT(member == api_ptr(api_fetch(&query)), "MEMBER MISMATCH: '%.*s'\n", query.length, query.chars);
		}
		else if (stringeq(strings[TYPE], s))
		{
			pj_str(&s, &dest);
			SCRUB_STR(s.chars, s.length)
			assert(member->type.bits == api_fetch(&s).bits);
		}
	}
	++idx_members;
}


static void audit_enums(json_t* json)
{
	string s;
	json_t dest = {0};
	uint8_t flags;
	enumerator_t* e = enumerators + idx_enumerators;
	while((flags = pj_next_pair(&s, &dest, json)))
	{
		if (stringeq(strings[NAME], s))
		{
			pj_str(&s, &dest);
			string query = hot_append_scratch(&s);
			assert(e == api_ptr(api_fetch(&query)));
		}
		else if (stringeq(strings[VALUES], s))
		{
			pj_mapc_array(audit_enum_values, &dest);
		}
		else if (stringeq(strings[IS_BITFIELD], s))
		{
			const uint8_t mask = (flags & 0x0f) == JSON_TRUE ? 0xff : 0x00;
			assert(e->is_bitfield == mask);
		}
	}
	++idx_enumerators;
}

// GOOD
static void audit_signals(json_t* json)
{
	string s;
	json_t dest = {0};
	uint8_t flags;
	signal_t* signal = signals + idx_signals;
	while((flags = pj_next_pair(&s, &dest, json)))
	{
		if (stringeq(strings[NAME], s))
		{
			pj_str(&s, &dest);
			string query = hot_append_scratch(&s);
			assert(signal == api_ptr(api_fetch(&query)));
		}
		else if (stringeq(strings[ARGUMENTS], s))
		{
			pj_mapc_array(audit_params, &dest);
		}
	}
	++idx_signals;
}

static void audit_constants(json_t* json)
{
	string s;
	json_t dest = {0};
	uint8_t flags;
	string name;
	constant_t* constant = constants + idx_constants;
	while((flags = pj_next_pair(&s, &dest, json)))
	{
		if (stringeq(strings[NAME], s))
		{
			pj_str(&s, &dest);
			name = hot_append_scratch(&s);
			assert(constant == api_ptr(api_fetch(&name)));
		}
		else if (stringeq(strings[TYPE], s))
		{
			if ((flags & 0x0f) != JSON_STRING)
			{
				pj_allman(&dest);
				fflush(stdout);
				assert(0);
			}
			pj_str(&s, &dest);
			SCRUB_STR(s.chars, s.length)
			const api_ptr_t temp = api_fetch(&s);
			PASSERT(constant->type.bits == temp.bits, "audit_constants() '%.*s': type mismatch\n"
					"target: %#x; fetched: %#x\n", name.length, name.chars, constant->type.bits, temp.bits);
			break;
			/* assert(target_delta == api_fetch(str, len).diff_ptr.delta); */
		}
		else if (stringeq(strings[VALUE], s))
		{
			switch(flags & 0x0f)
			{
			case JSON_INTEGER:
				assert(constant->type.bits == api_fetch(&strings[INT]).bits);
				break;
			case JSON_FLOAT:
				assert(constant->type.bits == api_fetch(&strings[FLOAT]).bits);
				break;
			case JSON_TRUE: __attribute__((fallthrough));
			case JSON_FALSE:
				assert(constant->type.bits == api_fetch(&strings[BOOL]).bits);
				break;
			case JSON_NULL:
				assert(constant->type.bits == api_fetch(&strings[NIL]).bits);
				break;
			default:
				assert(0 && "u fuccn wot");
			}
		}
	}
	++idx_constants;
}
#endif

#define relative_ptr(A) _Generic((A),									\
								 api_ptr_t: relative_api_ptr,			\
								 string: relative_string,				\
								 default: relative_pointer)(&(A), base)

static inline void relative_api_ptr(api_ptr_t* api_ptr,  const size_t base)
{
	api_ptr->ptr = api_ptr->ptr ? (void*)((size_t)api_ptr->ptr - base) : NULL;
}

static inline void relative_string(string* s, const size_t base)
{
	s->chars = s->length ? (char*)((size_t)s->chars - base) : NULL;
}

static inline void relative_pointer(void** ptr, const size_t base)
{
	*ptr = *ptr ? (void*)((size_t)*ptr - base) : NULL;
}

static void relative_api_map(api_map_t* api_map, const size_t base)
{
	char** keys = api_map->keys;
	int32_t* lens = api_map->lengths;
	api_ptr_t* values = api_map->values;
	for (int i = 0; i < api_map->cap; ++i)
	{
		if (lens[i] > 0)
		{
			relative_ptr(values[i]);
			if (keys[i][0] == ' ')
			{
				if (lens[i] == 18)
				{
					union binary_key key0;
					memcpy(&key0, keys[i], sizeof key0);
					/* printf("relative operator key: %02x%016lx%016lx", key0.operator, key0.lhs, key0.rhs); */
					relative_ptr(key0.lhs);
					relative_ptr(key0.rhs);
					/* printf(" -> %02x%016lx%016lx\n", key0.operator, key0.lhs, key0.rhs); */
					memcpy(keys[i], &key0, sizeof key0);
				}
				else if (lens[i] == 10)
				{
					union unary_key key0;
					memcpy(&key0, keys[i], sizeof key0);
					/* printf("old: %02x%016lx\n", key0.operator, key0.operand); */
					relative_ptr(key0.operand);
					/* printf("new: %02x%016lx\n", key0.operator, key0.operand); */
					memcpy(keys[i], &key0, sizeof key0);
				}
			}
			relative_ptr(keys[i]);
		}
	}
	relative_ptr(api_map->keys);
	relative_ptr(api_map->values);
	relative_ptr(api_map->lengths);
}

static void relative_pointers(const size_t base)
{
	relative_api_map(api_map, base);
#define X(_, A, B) printf("num " #B ": %d\n", num_##B); A##_t* A;
	API_TAGS
	#undef X
	for (int i = 0; i < num_classes; ++i)
	{
		class = classes + i;
		relative_api_map(class, base);
		relative_ptr(class->constructors);
		relative_ptr(class->inherits);
		relative_ptr(class->name);
	}
	for (int i = 0; i < num_funcs; ++i)
	{
		func = funcs + i;
		relative_ptr(func->name);
		if (func->num_params)
		relative_ptr(func->params);
		relative_ptr(func->return_type);
	}
	for (int i = 0; i < num_params; ++i)
	{
		param = params + i;
		relative_ptr(param->name);
		relative_ptr(param->default_value);
		relative_ptr(param->type);
	}
	for (int i = 0; i < num_enum_values; ++i)
	{
		enum_value = enum_values + i;
		relative_ptr(enum_value->name);
	}
	for (int i = 0; i < num_constants; ++i)
	{
		constant = constants + i;
		relative_ptr(constant->type);
		relative_ptr(constant->value);
	}
	for (int i = 0; i < num_enumerators; ++i)
	{
		enumerator = enumerators + i;
		relative_ptr(enumerator->values);
	}
	for (int i = 0; i < num_signals; ++i)
	{
		signal = signals + i;
		relative_ptr(signal->params);
	}
	for (int i = 0; i < num_native_structs; ++i)
	{
		native_struct = native_structs + i;
		relative_ptr(native_struct->format);
	}
	for (int i = 0; i < num_members; ++i)
	{
		member = members + i;
		relative_ptr(member->type);
	}
}


int main(int argc, char** argv)
{
	assert(sizeof(api_ptr_t) == 8 && "8 && api_ptr_t must be 8 bytes!\n");
	/* printf("NOTE: these should be ordered largest to smallest & be divisible by 8!\n"); */
#define X(_, A,...) printf("sizeof(" #A "_t): %d\n", sizeof(A##_t));
	API_TAGS
#undef X
	printf("sizeof(api_map_t): %d\n\n", sizeof(api_map_t));
	printf("sizeof(api_ptr_t): %d\n", sizeof(api_ptr_t));
	printf("sizeof(union unary_key): %d\n", sizeof(union unary_key));
	printf("sizeof(union binary_key): %d\n\n", sizeof(union binary_key));
	ms_handle_t handle;
	int32_t json_len;
	char* json_str = ms_read_file("./extension_api.json", &json_len, &handle);
	arena_t* parse_arena = alloca(sizeof *parse_arena);
	*parse_arena = (arena_t){0};
	json_t json = { 0x00 };
	if (pj_parse(&json, parse_arena, json_str, json_len) ^ JSON_OBJECT)
	{
		fprintf(stderr, "FAILED TO PARSE extension_api.json!\n");
		return 1;
	}
	src = fopen("./src/godot_api.gen.c", "w");
	fprintf(src, "/* this file is generated by api_generator.c & relies upon extension_api.json */\n\n");
	fprintf(src, "#define GODOT_API_GEN_C // dont really need this but w/e\n"
			"#include \"stdio.h\"\n"
			"#include \"stdint.h\"\n"
			"#include \"godot_api.h\"\n");
			/* "#include \"api_utils.h\"\n\n"); */
	json_t dest = { 0x00 };
	string s;
	uint8_t flags;
	int32_t body_idx;
	{
		int major;
		int minor;
		int patch;
		while ((flags = pj_next_pair(&s, &dest, &json)))
		{
			printf("key: %.*s\n", s.length, s.chars);
			if (stringeq(STRING("header"), s) && (flags & 0x0f) == JSON_OBJECT)
			{
				json_t header = {0};
				while((flags = pj_next_pair(&s, &header, &dest)))
				{
					if (stringeq(STRING("version_major"), s) && (flags & 0x0f) == JSON_INTEGER)
					{
						pj_int(&major, &header);
					}
					else if (stringeq(STRING("version_minor"), s) && (flags & 0x0f) == JSON_INTEGER)
					{
						pj_int(&minor, &header);
					}
					else if (stringeq(STRING("version_patch"), s) && (flags & 0x0f) == JSON_INTEGER)
					{
						pj_int(&patch, &header);
					}
				}
				goto write_supported_version;
			}
		}
		perror("FAILED TO EXTRACT SUPPORTED VERSION FROM extension_api.json!\n");
		fflush(stderr);
		return 1;
		write_supported_version:
		body_idx = json.index;
		printf("supported version: %d.%d.%d\n\n", major, minor, patch);
		fprintf(src, "void print_supported_version(void) { printf(\"Supported verison: %d.%d.%d\\n\"); }\n", major, minor, patch);
	}
	// PHASE ONE: API SIZE WITHOUT HASHMAPS

	while ((flags = pj_next_pair(&s, &dest, &json)))
	{
		if (stringeq(STRING("global_enums"), s))
		{
			pj_mapc_array(p1_enums, &dest);
			printf("num enums: %d\n", num_enumerators);
		}
		else if (stringeq(STRING("utility_functions"), s))
		{
			pj_mapc_array(p1_funcs, &dest);
		}
		else if (stringeq(STRING("builtin_classes"), s) || stringeq(STRING("classes"), s))
		{
			/* #error builtin classes are fucky wrt operators */
			pj_mapc_array(p1_classes, &dest);
			/* printf("num_classes: %d\n", num_classes); */
		}
		else if (stringeq(STRING("native_structures"), s))
		{
			pj_mapc_array(p1_native_structs, &dest);
			num_native_structs += 2; // NOTE: I just add in TWO of my own BESPOKE native struct to handle all pointer types
			// C-Pointer for ALL pointers
			// C-Buffer for buffers
			str_size += strlen("C Pointer");
			str_size += strlen("void*");
			str_size += strlen("C Buffer");
			str_size += strlen("uint8_t*");
		}
	}
#define X(_, A, B) (sizeof(A##_t) * num_##B) +
	const int32_t api_size = API_TAGS str_size;
#undef X
	printf("str size: %dKB\n", str_size / 1024);
	printf("api size: %dKB\n", api_size / 1024);

	// PHASE DEUCE: CUMULATIVE SIZE OF ALL HASHMAPS
	// recount enums for the global api
#define X(_, __, A) const int32_t cache_##A = num_##A; num_##A = 0;
	API_TAGS
#undef X
	json.index = body_idx;
	while ((flags = pj_next_pair(&s, &dest, &json)))
	{
		if (stringeq(STRING("global_enums"), s))
		{
			pj_mapc_array(accumulate_enumerators, &dest);
		}
	}
	const int32_t base_map_entries = hashmap_size(num_operators + num_enumerators + num_classes + num_native_structs);
	map_entries = base_map_entries;
	json.index = body_idx;
	while ((flags = pj_next_pair(&s, &dest, &json)))
	{
		if (stringeq(STRING("builtin_classes"), s) || stringeq(STRING("classes"), s))
		{
			pj_mapc_array(p2_classes, &dest);
		}
	}
#define X(_, __, A) num_##A =  cache_##A;
	API_TAGS
#undef X
	const int32_t map_scale = sizeof(string) + sizeof(api_ptr_t);
	const int32_t map_size = map_scale * map_entries;
	printf("cumulative map size: %dKB\n", map_size / 1024);
	const int32_t total_size = sizeof *api_map + map_size + api_size + 512;
	printf("total size: %dKB\n", total_size / 1024);
	/* uint8_t* buffer = malloc(total_size); */
	/* total_size += (8 - (total_size & 0x07)); */
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
#define X(_, __, A)												   \
	A = arena_alloc_aligned(&str_arena, num_##A * sizeof *A, 64u); \
	memset(A, 0x00, num_##A * sizeof *A); \
	printf(#A "(%d): [%#018x, %#018x)\n", num_##A, (size_t)A, (size_t)A + num_##A * sizeof *A);
	API_TAGS
#undef X
	/* str_arena = (arena_t) */
	/* { */
	/* 	.allocation = arena_alloc(&str_arena, str_size), */
	/* 	.cap = str_size */
	/* }; */

	/* PHASE DEUCE(B): INITIALIZE CLASS MAPS */

	json.index = body_idx;
	idx_classes = 0;
	while ((flags = pj_next_pair(&s, &dest, &json)))
	{
		if (stringeq(STRING("builtin_classes"), s) || stringeq(STRING("classes"), s))
		{
			pj_mapc_array(p2b_classes, &dest);
		}
	}

	/* PHASE THREE: CACHE EVERYTHING NOW! */

#define X(_, __, A) idx_##A = 0;
	API_TAGS
#undef X
	json.index = body_idx;
	while ((flags = pj_next_pair(&s, &dest, &json)))
	{
		if (stringeq(STRING("global_enums"), s))
		{
			pj_mapc_array(p3_global_enums, &dest);
		}
		else if (stringeq(STRING("utility_functions"), s))
		{
			pj_mapc_array(p3_global_funcs, &dest);
		}
		else if (stringeq(STRING("builtin_classes"), s) || stringeq(STRING("classes"), s))
		{
			pj_mapc_array(p3_classes, &dest);
		}
		else if (stringeq(STRING("native_structures"), s))
		{
			pj_mapc_array(p3_native_structs, &dest);
			// add in C Pointer & C Buffer
			native_struct_t* native = native_structs + idx_native_structs;
			string key = STRINGN(arena_push(&str_arena, "C Pointer", 9), 9);
			native->format = STRINGN(arena_push(&str_arena, "void*", 5), 5);
			api_map_cache(api_map, &key, flag_api_ptr(native, API_NATIVE_STRUCT | GODOT_API));
			native++;
			key = STRINGN(arena_push(&str_arena, "C Buffer", 8), 8);
			native->format = STRINGN(arena_push(&str_arena, "uint8_t*", 8), 8);
			api_map_cache(api_map, &key, flag_api_ptr(native, API_NATIVE_STRUCT | GODOT_API));
		}
	}

#ifndef DO_NOT_AUDIT
	/* api_map_audit(api_map); */
	/* for (int32_t i = 0; i < cache_classes; ++i) */
	/* { */
	/* 	api_map_audit(classes + i); */
	/* } */
	/* printf("OK: SIMPLE AUDIT\n"); */
#endif

	/* PHASE 4: EVERYTHING ELSE */

#define X(_, __, A) idx_##A = 0;
	API_TAGS
#undef X
	json.index = body_idx;
	while ((flags = pj_next_pair(&s, &dest, &json)))
	{
		if (stringeq(STRING("global_enums"), s))
		{
			pj_mapc_array(p4_enums, &dest);
		}
		else if (stringeq(STRING("utility_functions"), s))
		{
			pj_mapc_array(p4_funcs, &dest);
		}
		else if (stringeq(STRING("builtin_classes"), s) || stringeq(STRING("classes"), s))
		{
			pj_mapc_array(p4_classes, &dest);
		}
		else if (stringeq(STRING("singletons"), s))
		{
			pj_mapc_array(p4_singletons, &dest);
		}
	}
	printf("API binary fully generated!\n");

#ifndef DO_NOT_AUDIT
	/* api_map_audit(api_map); */
	/* for (int32_t i = 0; i < cache_classes; ++i) */
	/* { */
	/* 	api_map_audit(classes + i, 0); */
	/* } */
#define X(_, __, A) idx_##A = 0;
	API_TAGS
#undef X
	json.index = body_idx;
	while ((flags = pj_next_pair(&s, &dest, &json)))
	{
		if (stringeq(STRING("global_enums"), s))
		{
			pj_mapc_array(audit_global_enums, &dest);
		}
		else if (stringeq(STRING("utility_functions"), s))
		{
			pj_mapc_array(audit_global_funcs, &dest);
		}
		else if (stringeq(STRING("builtin_classes"), s) || stringeq(STRING("classes"), s))
		{
			pj_mapc_array(audit_classes, &dest);
		}
		else if (stringeq(STRING("singletons"), s))
		{
			/* pj_mapc_array(audit_singletons, &dest); */
		}
	}

	printf("OK: FULL AUDIT\n");
	printf("Nil@%016lx\n", api_fetch(&STRINGN("Nil", 3)).bits);
#endif
	api_ptr_t nil = api_fetch(&STRINGN("Nil", 3));
	printf("NIL(%016lx): %x @ %016lx\n", nil.bits, api_tag(nil), api_ptr(nil));
	FILE* ppm = fopen("godot_api.ppm", "wb");
	api_map_ppm(ppm, 64, api_map);
	fclose(ppm);
	relative_pointers(str_arena.allocation);
	// just make all pointers self-relative YOLO
	fprintf(src, "\nconst int32_t api_size = %d;\n", total_size);
	fprintf(src, "uint8_t api_binary[%d] =\n{\n\t", total_size);
	uint8_t* bin = (void*)str_arena.allocation;
	for (int32_t i = 0; i < total_size; ++i)
	{
		fprintf(src, "0x%02x, ", bin[i]);
		/* if (bin[i]) */
		/* { */
		/* } */
		/* else */
		/* { */
		/* 	fprintf(src, "0x00, "); */
		/* } */
	}
	fprintf(src, "\n};\n\n");
	fprintf(src, "api_map_t* api_map = api_binary;\n");
	fprintf(src,
#define X(_, A, B) #A "_t* " #B " = api_binary + %ld;\n"
	API_TAGS
#undef X
#define X(_, A, B) , (size_t)B - (size_t)bin
	API_TAGS
#undef X
	);
	fprintf(src,
#define X(_, __, A) "const int32_t num_" #A " = %d;\n"
	API_TAGS
#undef X
#define X(_, __, A) , num_##A
	API_TAGS
#undef X
		);
	fprintf(src, "char* builtin_api_chars = api_binary + %ld;\n"
			"const int32_t num_builtin_api_chars = %d;\n",
			(size_t)str_arena.allocation - (size_t)bin,
			str_arena.size);
	const uint64_t hash = api_map_hash(api_map, total_size);
	fprintf(src, "uint64_t api_hash = %lu; /* NOTE: DO NOT SAVE AS HEX */\n", hash);
	printf("DO NOT WRITE AS HEX: api hash: %#x\n", hash);
	fclose(src);
	printf("'godot_api.gen.c' generated!\n");
	// FORWARD DECLARATIONS
	header = fopen("./src/godot_api.gen.h", "w");
	fprintf(header, "#ifndef GODOT_API_GEN_H\n"
			"#define GODOT_API_GEN_H\n"
			"#include <stdint.h>\n"
			"#include \"api_structs.h\"\n"
			"\n"
			"void print_supported_version(void);\n"
			"#ifndef GODOT_API_GEN_C\n"
			"extern int32_t api_size;\n"
			"extern uint8_t api_binary[%d];\n", total_size);
	fprintf(header, "extern api_map_t* api_map;\n");
	fprintf(header,
#define X(_, A, B) "extern " #A "_t* " #B";\n"
	API_TAGS
#undef X
#define X(_, __, A) "extern int32_t num_" #A ";\n"
	API_TAGS
#undef X
/* #define X(_, A) , num_##Anum */
/* 		NUMBERS */
/* #undef X */
		);
	fprintf(header, "extern char* builtin_api_chars;\n"
			"extern int32_t num_builtin_api_chars;\n"
			"extern uint64_t api_hash;\n"
			"#endif /* GODOT_API_GEN_C */\n"
			"#endif /* End GODOT_API_GEN_H */");
	fclose(header);
	printf("godot_api.gen.h' generated!\n");
	return 0;
}
