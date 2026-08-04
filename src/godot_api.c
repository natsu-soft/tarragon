#include "godot_api.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "api_structs.h"
#include "allocators/scratch.h"
#include "allocators/workpad.h"
#include "hashmaps/api_map.h"
#include "godot_api.gen.h"
#include "gdscript/token_ids.h"
IMPL_UNARY_KEY
IMPL_BINARY_KEY


#define PASSERT(A, MSG, ...) if (!(A)) { fprintf(stderr, MSG, ##__VA_ARGS__); assert(A); }

#define VERBOSE_FETCH
#ifdef VERBOSE_FETCH
	#define FETCH_PRINT(MSG, ...) printf(MSG, ##__VA_ARGS__)
#else
	#define FETCH_PRINT(...)
#endif

const api_ptr_t godot_api_fetch(const string* key)
{
	api_map_t* map = api_map;
	api_ptr_t res = {0};
	if (key->chars[0] != ' ')
	{
		int32_t i0 = 0;
		FETCH_PRINT("fetch: ");
		for (int32_t i = i0; i < key->length; ++i)
		{
			if (key->chars[i] != '.') { continue; }
			else
			{
				string temp = STRINGN(key->chars + i0, i - i0);
				api_ptr_t p = api_map_fetch(map, &temp);
				/* cpystr(temp, key + i0, i - i0); */
				FETCH_PRINT("%.*s -> ", temp.length, temp.chars);
				PASSERT(api_tag(p) == API_CLASS, "'%.*s' should have fetched a class!\n", temp.length, temp.chars);
				map = api_ptr(p);
				i0 = i + 1;
			}
		}
		/* char* temp = alloca(len + 1); */
		/* cpystr(temp, key + i0, len - i0); */
		string temp = STRINGN(key->chars + i0, key->length - i0);
		FETCH_PRINT("%.*s (%d:", temp.length, temp.chars, temp.length);
		res = api_map_fetch(map, key);
		switch(res.tag)
		{
		case API_MEMBER:
			/* res.ptr = abs_diff(&res.diff_ptr, members); */
			FETCH_PRINT("member)\n");
			break;
		case API_ENUM:
			/* res.ptr = abs_diff(&res.diff_ptr, enums); */
			FETCH_PRINT("enum)\n");
			break;
		case API_FUNC:
			/* res.ptr = abs_diff(&res.diff_ptr, funcs); */
			FETCH_PRINT("function)\n");
			break;
		case API_CLASS:
			/* res.ptr = abs_diff(&res.diff_ptr, classes); */
			FETCH_PRINT("class)\n");
			break;
		case API_SIGNAL:
			/* res.ptr = abs_diff(&res.diff_ptr, signals); */
			FETCH_PRINT("signal)\n");
			break;
		case API_CONSTANT:
			/* res.ptr = abs_diff(&res.diff_ptr, constants); */
			FETCH_PRINT("constant)\n");
			break;
		case API_NATIVE_STRUCT:
			/* res.ptr = abs_diff(&res.diff_ptr, native_structs); */
			FETCH_PRINT("struct)\n");
			break;
		default:
			FETCH_PRINT("ERR) -- DELTA: %#016lx\n", res.bits);
			#ifdef VERBOSE_FETCH
			fflush(stdout);
			assert(0);
			#else
			assert(0 && "catastrophic fetch error -- enable VERBOSE_FETCH");
			#endif
		}
		fflush(stdout);
	}
	else
	{
		res = api_map_fetch(map, key);
		if (!(res.tag == API_CLASS || res.tag == API_ENUM || res.tag == API_NATIVE_STRUCT))
		{
			assert(0 && "operation keys SHOULD return either an enum or a class!");
		}
	}
	return res;
}

void debug_key(const string* key)
{
	// TODO
}


// this works
#define arg_type(A) _Generic((A),								\
							 api_ptr_t: #A " -> api_ptr_t\n",	\
							 string: #A " -> string\n",			\
							 default: #A " -> pointer\n")

#define stable_ptr(A) _Generic((A),								\
							   api_ptr_t: stable_api_ptr,		\
							   string: stable_string,			\
							   default: stable_pointer)(&(A), base)


static inline void stable_pointer(void** ptr, const size_t base)
{
	*ptr = *ptr ? (void*)((size_t)*ptr + base) : NULL;
}

static inline void stable_string(string* s, const size_t base)
{
	s->chars = s->length ? (void*)((size_t)s->chars + base) : NULL;
}

static inline void stable_api_ptr(api_ptr_t* ptr, const size_t base)
{
	/* printf("api_ptr: %#lx -> ", ptr->ptr); */
	ptr->ptr = ptr->ptr ? (void*)((size_t)ptr->ptr + base) : NULL;
	/* printf("%#lx\n", ptr->ptr); */
}

static void stabilize_api()
{
	const size_t base  = (size_t)api_map;
	for (int i = 0; i < num_classes; ++i)
	{
		class_t *class = classes + i;
		stable_ptr(class->name);
		stable_ptr(class->inherits);
		stable_ptr(class->constructors);
	}
	fflush(stdout);
	for (int i = 0; i < num_funcs; ++i)
	{
		func_t *func = funcs + i;
		stable_ptr(func->name);
		stable_ptr(func->params);
		stable_ptr(func->return_type);
		void* ret_ptr = api_ptr(func->return_type);
		/* switch(api_tag(func->return_type)) */
		/* { */
		/* case API_NONE: */
		/* 	printf("%.*s -> void\n", func->name.length, func->name.value); */
		/* 	break; */
		/* case API_ENUM: */
		/* 	printf("%.*s -> enum@%#018x\n", func->name.length, func->name.value, ret_ptr); */
		/* 	break; */
		/* case API_CLASS: */
		/* { */
		/* 	class_t* class = ret_ptr; */
		/* 	printf("%.*s -> %.*s\n", func->name.length, func->name.value, */
		/* 		   class->name.length, class->name.value); */
		/* } */
		/* break; */
		/* } */
	}
	for (int i = 0; i < num_params; ++i) {
		param_t *param = params + i;
		stable_ptr(param->name);
		stable_ptr(param->default_value);
		stable_ptr(param->type);
	}
	for (int i = 0; i < num_enum_values; ++i) {
		struct enum_value_t *enum_value = enum_values + i;
		stable_ptr(enum_value->name);
	}
	for (int i = 0; i < num_constants; ++i) {
		struct constant_t *constant = constants + i;
		stable_ptr(constant->type);
		stable_ptr(constant->value);
	}
	for (int i = 0; i < num_enumerators; ++i) {
		struct enumerator_t *enumerator = enumerators + i;
		stable_ptr(enumerator->values);
	}
	for (int i = 0; i < num_signals; ++i) {
		struct signal_t *signal = signals + i;
		stable_ptr(signal->params);
	}
	for (int i = 0; i < num_native_structs; ++i) {
		struct native_struct_t *native_struct = native_structs + i;
		stable_ptr(native_struct->format);
	}
	for (int i = 0; i < num_members; ++i) {
		struct member_t *member = members + i;
		stable_ptr(member->type);
	}
	stable_ptr(api_map->values);
	stable_ptr(api_map->keys);
	stable_ptr(api_map->lengths);
	api_ptr_t* values = api_map->values;
	char** keys = api_map->keys;
	int32_t* lens = api_map->lengths;
	for (int i = 0; i < api_map->cap; ++i)
	{
		if (lens[i] > 0)
		{
			stable_ptr(values[i]);
			stable_ptr(keys[i]);
			if (keys[i][0] == ' ')
			{
				if (lens[i] == 18)
				{
					union binary_key key0;
					memcpy(&key0, keys[i], sizeof key0);
					/* printf("relative operator key: %02x%016lx%016lx", key0.operator, key0.lhs, key0.rhs); */
					key0.lhs.bits += base;
					key0.rhs.bits += base;
					/* printf(" -> %02x%016lx%016lx\n", key0.operator, key0.lhs, key0.rhs); */
					memcpy(keys[i], &key0, sizeof key0);
				}
				else if (lens[i] == 10)
				{{
					union unary_key key0;
					memcpy(&key0, keys[i], sizeof key0);
					/* printf("old: %02x%016lx\n", key0.operator, key0.operand); */
					key0.operand.bits += base;
					/* printf("new: %02x%016lx\n", key0.operator, key0.operand); */
					memcpy(keys[i], &key0, sizeof key0);
				}
				}
			}
		}
	}
	for (int i = 0; i < num_classes; ++i)
	{
		api_map_t* map = (api_map_t*)(classes + i);
		stable_ptr(map->values);
		stable_ptr(map->keys);
		stable_ptr(map->lengths);
		values = map->values;
		keys = map->keys;
		lens = map->lengths;
		for (int j = 0; j < map->cap; ++j)
		{
			if (lens[j] > 0)
			{
				stable_ptr(values[j]);
				stable_ptr(keys[j]);
				if (keys[j][0] == ' ')
				{
					assert(0 && "classes do NOT have operator keys!");
				}
			}
		}
	}
}

// just make sure all the pointers & values are within range
static void check_bounds_api_map(api_map_t* api_map)
{
	char** keys = api_map->keys;
	int32_t* lens = api_map->lengths;
	api_ptr_t* values = api_map->values;
	for (int i = 0; i < api_map->cap; ++i)
	{
		if (lens[i] > 0)
		{
			assert(builtin_api_chars <= keys[i]);
			assert(keys[i] + lens[i] < builtin_api_chars + num_builtin_api_chars);
			assert(api_metadata(values[i]) == GODOT_API);
			switch(api_tag(values[i]))
			{
#define X(ENUM, A, B) case API_##ENUM: assert(B <= (A##_t*)api_ptr(values[i])); assert((A##_t*)api_ptr(values[i]) < B + num_##B); break;
				API_TAGS
#undef X
			default:
				assert(0 && "unhandled API_TAG in check_bounds_api_map\n");
			}
			if (keys[i][0] == ' ')
			{
				token_id_t operator;
				api_ptr_t operands[2] = {0};
				int num_operands = 1;
				if (lens[i] == 18)
				{
					union binary_key k;
					memcpy(&k, keys[i], sizeof k);
					operands[0] = k.lhs;
					operands[1] = k.rhs;
					++num_operands;
				}
				else if (lens[i] == 10)
				{
					union unary_key k;
					memcpy(&k, keys[i], sizeof k);
					operands[0] = k.operand;
				}
				else
				{
					assert(0 && "unexpected length for operator key");
				}
				switch (operator)
				{
#define X(A, STR, LEN) case GDSCRIPT_##A: assert(0 && "unexpected operator" STR); break;
					GDSCRIPT_NOT_OPERATORS
#undef X
				}
				for (int j = 0; j < num_operands; ++j)
				{
					assert(api_metadata(operands[j]) == GODOT_API);
					switch(api_tag(operands[j]))
					{
#define X(ENUM, A, B) case API_##ENUM: assert(B <= (A##_t*)api_ptr(operands[j])); assert((A##_t*)api_ptr(operands[j]) < B + num_##B); break;
						API_TAGS
#undef X
					default:
						assert(0 && "unhandled API_TAG in check_bounds_api_map\n");
					}
				}
			}
		}
	}
}

static void check_bounds_string(const string str)
{
	assert(str.length > 0 ? builtin_api_chars <= str.chars : 1);
	assert(str.length > 0 ? str.chars + str.length - 1 < builtin_api_chars + num_builtin_api_chars : 1);
}

static void check_bounds_api_ptr(const api_ptr_t ptr)
{
	if (ptr.ptr)
	{
		/* printf("ptr: %016lx\n", ptr.ptr); */
		/* fflush(stdout); */
		assert(api_metadata(ptr) == GODOT_API);
		switch(api_tag(ptr))
		{
	#define X(ENUM, A, B) case API_##ENUM: \
			assert(B <= (A##_t*)api_ptr(ptr)); \
			assert((A##_t*)api_ptr(ptr) < B + num_##B); \
			break;
			API_TAGS
	#undef X
		default:
			assert(0 && "brother wtf????");
		}
	}
}

static void check_bounds_class(class_t* class)
{
	check_bounds_api_map(class);
	check_bounds_string(class->name);
	assert(class->num_constructors > 0 ? funcs <= class->constructors : 1);
	assert(class->num_constructors > 0 ? class->constructors + class->num_constructors - 1 < funcs + num_funcs : 1);
	assert(class->inherits != NULL ? classes <= class->inherits : 1);
	assert(class->inherits != NULL ? class->inherits < classes + num_classes : 1);
}

static void check_bounds_func(func_t* func)
{
	check_bounds_string(func->name);
	check_bounds_api_ptr(func->return_type);
	if (func->num_params > 0)
	{
		PASSERT(params <= func->params, "%.*s()->params@%016lx < %016lx", func->name.length, func->name.chars, func->params, params);
		assert(func->params + func->num_params - 1 < params + num_params);
	}
}

static void check_bounds_param(param_t* param)
{
	check_bounds_string(param->name);
	check_bounds_string(param->default_value);
	check_bounds_api_ptr(param->type);
}

static void check_bounds_enum_value(enum_value_t* enum_value)
{
	check_bounds_string(enum_value->name);
}

static void check_bounds_constant(constant_t* constant)
{
	check_bounds_api_ptr(constant->type);
	check_bounds_string(constant->value);
}

static void check_bounds_enumerator(enumerator_t* enumerator)
{
	assert(enumerator->num_values == 0 ? 1 : enum_values <= enumerator->values);
	assert(enumerator->num_values == 0 ? 1 : enumerator->values + enumerator->num_values - 1 < enum_values + num_enum_values);
}

static void check_bounds_signal(signal_t* signal)
{
	assert(signal->num_params == 0 ? 1 : params <= signal->params);
	assert(signal->num_params == 0 ? 1 : signal->params + signal->num_params - 1 < params + num_params);
}

static void check_bounds_native_struct(native_struct_t* native_struct)
{
	check_bounds_string(native_struct->format);
}

static void check_bounds_member(member_t* member)
{
	check_bounds_api_ptr(member->type);
}

int godot_api_test(int argc, char** argv)
{
	printf("testing api!\n");
	assert(api_map != NULL && "api_map is NULL!\n");
	printf("api_map is not NULL!\n");
	printf("api_binary: %0x\n", api_binary);
	printf("api_map: %0x\n", api_map);
	printf("api_size: %d\n", api_size);
	const uint64_t fresh_hash = api_map_hash(api_map, api_size);
	PASSERT((api_hash ^ fresh_hash) == 0x00, "hash mismatch! %#lx != %#lx\n", api_hash, fresh_hash);
	printf("serial hash: %#lx, new hash: %#lx\n", api_hash, fresh_hash);
	stabilize_api();
#define X(ENUM, A, B) printf(#B ": [%016lx, %016lx)\n", B, B + num_##B);
	API_TAGS
#undef X
	/* printf("\tglobal api:\n"); */
	/* api_map_audit(api_map); */
	check_bounds_api_map(api_map);
#define X(_, A, B) \
	printf("checking: "  #A " bounds\n"); \
	fflush(stdout); \
	for(int i = 0; i < num_##B; ++i)			\
	{ \
		check_bounds_##A(B + i); \
	}
	API_TAGS
#undef X
	return 0;
}
