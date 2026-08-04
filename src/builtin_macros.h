#ifndef BUILTIN_MACROS_H
#define BUILTIN_MACROS_H

// TODO: delete this

#define SUPPORTED_VERSION(NUM, VAL) builtin_api.supported_version.NUM = VAL;

#define END }

#define CACHE_GLOBAL_ENUM(NAME, LEN) \
	ptr_map_cache(&builtin_api.tagged_map, arena_push(&str_arena, NAME, LEN), LEN, (size_t)e | API_ENUM);

#define CACHE_GLOBAL_TYPE(NAME, LEN) \
	ptr_map_cache(&builtin_api.tagged_map, arena_push(&str_arena, NAME, LEN), LEN, (size_t)hot | API_TYPE);

#define CACHE_GLOBAL_CLASS(NAME, LEN) \
	ptr_map_cache(&builtin_api.tagged_map, arena_push(&str_arena, NAME, LEN), LEN, (size_t)hot | API_CLASS);

/* ENUMS */

#define BEGIN_ENUM {										\
	enum_t* e = arena_calloc(&enum_arena, sizeof(enum_t));


#define ENUM_NAME(NAME, LEN) \
	char* name = arena_push(&str_arena, NAME, LEN);	\
	const int32_t name_len = LEN;

#define BEGIN_ENUM_VALUE { enum_value_t* value = arena_calloc(&enum_value_arena, sizeof(enum_value_t));

#define BEGIN_VALUES e->values = enum_value_arena.allocation + enum_value_arena.size;

#define END_VALUES(NUM) e->num_values = NUM;

#define ENUM_VALUE_NAME(NAME, LEN) value->name = arena_push(&str_arena, NAME, LEN + 1);

#define ENUM_VALUE(VALUE) value->value = VALUE;

#define ENUM_IS_BITFIELD e->is_bitfield = 0xff;

/* FUNCS */

#define BEGIN_FUNC { func_t* f = arena_calloc(&func_arena, sizeof(func_t));

#define CACHE_GLOBAL_FUNC(NAME, LEN) \
	{ \
		func_t* head = ptr_map_fetch(&builtin_api.tagged_map, NAME, LEN); \
		if (head == NULL) \
		{ \
			ptr_map_cache(&builtin_api.tagged_map, arena_push(&str_arena, NAME, LEN), LEN, (size_t)f | API_FUNC); \
		} \
		else									\
		{ \
			assert(((size_t)head & 0xf) == API_FUNC && "in CACHE_GLOBAL_FUNC: ptr_map_fetch fetched a non-func_t address"); \
			for(head = (func_t*)((size_t)head & ~0xf); head->next != NULL; head = head->next); \
			head->next = f; \
		} \
	}

#define CACHE_FUNC(NAME, LEN) \
	{ \
		func_t* head = ptr_map_fetch(&hot->tagged_map, NAME, LEN); \
		if (head == NULL) \
		{ \
			ptr_map_cache(&hot->tagged_map, arena_push(&str_arena, NAME, LEN), LEN, (size_t)f | API_FUNC); \
		} \
		else									\
		{ \
			assert(((size_t)head & 0xf) == API_FUNC && "in CACHE_FUNC: ptr_map_fetch fetched a non-func_t address"); \
			for(head = (func_t*)((size_t)head & ~0xf); head->next != NULL; head = head->next); \
			head->next = f; \
		} \
	}

#define NUM_PARAMS(NUM) \
	f->params = arena_calloc(&param_arena, NUM * sizeof(param_t)); \
	f->num_params = NUM;

#define VARIADIC f->bitflags |= FUNC_VARARG;

#define CONSTANT f->bitflags |= FUNC_CONST;

#define STATIC f->bitflags |= FUNC_STATIC;

#define VIRTUAL f->bitflags |= FUNC_VIRTUAL;

#define TYPE_ADDR(TYPE, LEN) (ptr_map_fetch(&builtin_api.tagged_map, TYPE, LEN))

#define FUNC_RETURN(IDX, TYPE, LEN) \
	funcs[IDX].return_type = TYPE_ADDR(TYPE, LEN);

#define PARAM_TYPE(IDX, TYPE, LEN) \
	params[IDX].type = TYPE_ADDR(TYPE, LEN);

#define CONSTANT_TYPE(IDX, TYPE, LEN) \
	constants[IDX].type = TYPE_ADDR(TYPE, LEN);

// OPERATORS

#define OPERATOR(ID)											\
	{																	\
		operator_t* op = arena_calloc(&operator_arena, sizeof(operator_t)); \
		operator_t* head = op_map_fetch(&hot->op_map, ID); \
		if (head == NULL) \
		{ \
			op_map_cache(&hot->op_map, ID, op); \
		} \
		else									\
		{ \
			for (; head->next != NULL; head = head->next); \
			head->next = op; \
		} \
	}

#define OPERATOR_RIGHT(IDX, TYPE, LEN) operators[IDX].right_type = TYPE_ADDR(TYPE, LEN);

#define OPERATOR_RETURN(IDX, TYPE, LEN) operators[IDX].return_type = TYPE_ADDR(TYPE, LEN);


// TYPES & CLASSES

#define CACHE_ENUM(NAME, LEN) \
	ptr_map_cache(&hot->tagged_map, arena_push(&str_arena, NAME, LEN), LEN, (size_t)e | API_ENUM);

#define CACHE_MEMBER(NAME, LEN) \
	{ \
		member_t* m = arena_alloc(&member_arena, sizeof(member_t)); \
		ptr_map_cache(&hot->tagged_map, arena_push(&str_arena, NAME, LEN), LEN, (size_t)m | API_MEMBER); \
	}

#define MEMBER_TYPE(IDX, TYPE, LEN) \
	members[IDX].type = TYPE_ADDR(TYPE, LEN);

#define CACHE_CONSTANT(NAME, LEN) \
	{ \
		constant_t* c = arena_alloc(&constant_arena, sizeof(constant_t)); \
		ptr_map_cache(&hot->tagged_map, arena_push(&str_arena, NAME, LEN), LEN, (size_t)c | API_CONSTANT); \
	}

#define CACHE_SIGNAL(NAME, LEN) \
	ptr_map_cache(&hot->tagged_map, arena_push(&str_arena, NAME, LEN), LEN, (size_t)f | API_SIGNAL); \

#define BEGIN_TYPE { type_t* hot = arena_calloc(&type_arena, sizeof(type_t));

#define BEGIN_CLASS { class_t* hot = arena_calloc(&class_arena, sizeof(class_t));

#define HAS_DESTRUCTOR hot->bitflags |= CLASS_HAS_DESTRUCTOR;

#define IS_KEYED hot->bitflags |= CLASS_IS_KEYED;

#define NUM_CONSTRUCTORS(NUM) \
	func_t* constructor = func_arena.allocation + func_arena.size; \
	constructor -= NUM; \
	hot->constructors = constructor; \
	for (int i = 0; i < NUM - 1; ++i) \
	{ \
		constructor->next = constructor + 1; \
		constructor = constructor->next; \
	} \

#define BEGIN_SIGNAL { signal_t* f = arena_calloc(&signal_arena, sizeof(signal_t));

#define SINGLETON(NAME, LEN) \
{ \
	void* ptr = ptr_map_fetch(&builtin_api.tagged_map, NAME, LEN); \
	assert (ptr != NULL && "failed to cache class/type " #NAME); \
	assert(((size_t)ptr & 0x0f) == API_CLASS && "marking TYPE " NAME "as a singleton!  This is wrong!\n"); \
	class_t* class = (class_t*)((size_t)ptr & ~0xf); \
	class->bitflags |= CLASS_IS_SINGLETON; \
}

#define TYPE_CONSTRUCTOR(FUNC_IDX, TYPE_IDX) funcs[FUNC_IDX].return_type = types + TYPE_IDX;

#define CLASS_CONSTRUCTOR(FUNC_IDX, CLASS_IDX) funcs[FUNC_IDX].return_type = classes + CLASS_IDX;



#endif /* End BUILTIN_MACROS_H */
