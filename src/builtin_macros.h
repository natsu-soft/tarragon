#ifndef BUILTIN_MACROS_H
#define BUILTIN_MACROS_H

#define SUPPORTED_VERSION(NUM, VAL) supported_version.NUM = VAL;

#define GLOBAL_ENUM(NAME, LEN, BITFIELD)								\
	{																	\
		gdscript_enum_t* e = stack_alloc(enum_stack, sizeof *e);		\
		const int32_t name_pos = str_arena->size;						\
		*e = (gdscript_enum_t)											\
		{																\
			.name = name_pos,											\
			.values = enum_val_stack->size,								\
			.name_len = LEN,											\
			.is_bitfield = BITFIELD										\
		};																\
		char* name = arena_alloc(str_arena, LEN);						\
		memcpy(name, NAME, LEN);										\
		map32_cache(type_map, name, LEN, (int32_t)((size_t)e - (size_t)enum_stack->allocation) | 0b1); \
		++builtin_table.num_global_enums;								\
	}

#define ENUM_VALUE(NAME, NAME_LEN, VALUE, DESC, DESC_LEN)				\
	{																	\
		gdscript_enum_t* e = stack_peek(enum_stack);					\
		++(e->num_values);												\
		enum_value_t* val = stack_alloc(enum_val_stack, sizeof *val);	\
		const int32_t name_pos = str_arena->size;						\
		*val = (enum_val_t)												\
		{																\
			.name = (char*)name_pos,									\
			.desc = str_arena->size,									\
			.value = VALUE,												\
			.name_len = NAME_LEN,										\
			.desc_len = DESC_LEN,										\
		};																\
		char* name = arena_alloc(str_arena, NAME_LEN);					\
		memcpy(name, NAME, NAME_LEN);										\
		map32_cache(str_map, name, LEN, name_pos); \
		char* desc = stack_alloc(char_stack, DESC_LEN);					\
		memcpy(desc, DESC, DESC_LEN);									\
	}

#define FUNC_BASE(NAME, NAME_LEN)											\
	{																	\
	gdscript_func_t* f = stack_alloc(func_stack, sizeof *f);			\
	*f = (gdscript_func_t)												\
		{																\
			.name = str_arena->size,									\
			.params = param_stack->size,								\
			.name_len = NAME_LEN,										\
			.class_index = last_class,									\
			.name_len = NAME_LEN,										\
		};																\
	char* name = arena_alloc(str_arena, NAME_LEN);						\
	memcpy(name, NAME, NAME_LEN);										\
	}

#define FUNC(NAME, NAME_LEN)										\
	FUNC_BASE(NAME, NAME_LEN)										\
	{																\
		gdscript_class_t* c = class_stack->allocation + last_class; \
		c->num_methods++;											\
	}


#define GLOBAL_FUNC(NAME, NAME_LEN)				\
	FUNC_BASE(NAME, NAME_LEN)					\
	++builtin_table.num_global_funcs;

#define FUNC_FLAGS(BITFLAGS)							\
	{													\
		gdscript_func_t* f = stack_peek(func_stack);	\
		f->flags = BITFLAGS;							\
	}

#define FUNC_RET(TYPE, TYPE_LEN)										\
	{																	\
		int32_t type_index = map32_fetch(type_map, TYPE, TYPE_LEN);		\
		if (type_index == -1)											\
		{																\
			type_index = class_stack.size;								\
			gdscript_class_t* temp = stack_alloc(class_stack, sizeof *temp); \
			memset(temp, 0x00, sizeof *temp);							\
			temp->name = str_arena.size;								\
			char* class_name = arena_alloc(str_arena, TYPE_LEN);		\
			memcpy(class_name, TYPE, TYPE_LEN);							\
			map32_cache(type_map, class_name, TYPE_LEN, ret_index);		\
		}																\
		gdscript_func_t* f = stack_peek(func_stack);					\
		f->ret_index = type_index;										\
		f->ret_len = RET_LEN;											\
	}


#define FUNC_PARAM(NAME, NAME_LEN, TYPE, TYPE_LEN)						\
	{																	\
		gdscript_func_t* f = stack_peek(func);							\
		func_param_t* p = stack_alloc(param_stack, sizeof *p);			\
		int32_t type_index = map32_fetch(type_map, TYPE, TYPE_LEN);		\
		if (type_index == -1)											\
		{																\
			type_index = class_stack.size;								\
			gdscript_class_t* temp = stack_alloc(class_stack, sizeof *temp); \
			memset(temp, 0x00, sizeof *temp);							\
			temp->name = str_arena.size;								\
			char* class_name = arena_alloc(str_arena, TYPE_LEN);			\
			memcpy(class_name, TYPE, TYPE_LEN);							\
			map32_cache(type_map, class_name, TYPE_LEN, ret_index);		\
		}																\
		*p = (func_param_t)												\
		{															\
			.name = str_arena->size,								\
			.name_len = NAME_LEN,									\
			.type_index = type_index								\
		};															\
		++(f->num_params);												\
	}

#define FUNC_DESC(DESC, DESC_LEN)				\
	{													\
		gdscript_func_t* f = stack_peek(func_stack);			\
		f->desc = str_arena->size;						\
		f->desc_len = DESC_LEN;							\
		char* desc = arena_alloc(str_arena, DESC_LEN); \
		memcpy(desc, DESC, DESC_LEN);					\
	}

#define CLASS(NAME, NAME_LEN, IS_KEYED)							\
	{															\
		last_class = map32_fetch(type_map, NAME, NAME_LEN);		\
		gdscript_class_t* c;									\
		if (last_class > -1)									\
		{														\
			c = class_stack->allocation + last_class;			\
			c->bitflags |= IS_KEYED;							\
		}														\
		else													\
		{														\
			last_class = class_stack.size;						\
			c = stack_alloc(class_stack, sizeof *c);			\
			*c = (gdscript_class_t)								\
			{												\
				.name = str_arena.size,						\
				.name_len = NAME_LEN,						\
				.bitflags = IS_KEYED						\
				.indexing_ret_index = -1,					\
			};												\
			char* name = arena_alloc(str_arena, RET_LEN);		\
			memcpy(name, NAME, NAME_LEN);						\
			map32_cache(type_map, name, NAME_LEN, last_class);	\
		}														\
	}

#define DESTRUCTOR(VALUE)												\
	{																	\
		gdscript_class_t* class = class_stack->allocation + last_class; \
		class->bitflags |=  CLASS_DESTRUCTOR & (VALUE << 1);			\
	}

#define CLASS_INDEXING_TYPE(RET_NAME, RET_LEN)							\
	{																	\
		int32_t index = map32_fetch(type_map, RET_NAME, RET_LEN); \
		gdscript_class_t* c = class_stack->allocation + last_class;		\
		if (index > -1)													\
		{																\
			c->indexing_ret_index = index;								\
		}																\
		else															\
		{																\
		} \
	}

#define OPERATOR(ID, RET_NAME, RET_LEN)									\
	{																	\
		gdscript_class_t* c = class_stack->allocation + last_class; \
		c->num_operators += 1;										\
		operator_t* op = stack_alloc(operator_stack, sizeof *op);		\
		*op = (operator_t)												\
		{															\
			.id = ID,												\
			.right_index = -1										\
		};															\
	}

#define OPERATOR_RETURN(RET_NAME, RET_LEN)								\
	{																	\
		int32_t ret_index = map32_fetch(TYPE_NAME, TYPE_LEN); \
		if (ret_index == -1)											\
		{																\
			ret_index = class_stack.size;								\
			gdscript_class_t* temp = stack_alloc(class_stack, sizeof *temp); \
			memset(temp, 0x00, sizeof *temp);							\
			temp->name = str_arena.size;								\
			char* class_name = arena_alloc(str_arena, RET_LEN);			\
			memcpy(class_name, TYPE, TYPE_LEN);							\
			map32_cache(type_map, class_name, TYPE_LEN, ret_index); \
		}																\
		operator_t* op = stack_peek(operator_stack);					\
		op->ret_index = ret_index;										\
	}

#define OPERATOR_RIGHT(TYPE_NAME, TYPE_LEN)								\
	{																	\
	int32_t right_index = map32_fetch(TYPE_NAME, TYPE_LEN); \
	if (right_index == -1)												\
	{																	\
		right_index = class_stack.size;									\
		gdscript_class_t* temp = stack_alloc(class_stack, sizeof *temp); \
		memset(temp, 0x00, sizeof *temp);								\
		temp->name = str_arena.size;									\
		char* class_name = arena_alloc(str_arena, RET_LEN);				\
		memcpy(class_name, TYPE, TYPE_LEN);								\
		map32_cache(type_map, class_name, TYPE_LEN, ret_index); \
	}																	\
	operator_t* op = stack_peek(operator_stack);						\
	op->right_index = right_index;										\
	}

#define OPERATOR_DESC(DESC, DESC_LEN)					\
	{													\
		operator_t* op = stack_peek(operator_stack);	\
		op->desc = str_arena->size;						\
		char* desc = arena_alloc(str_arena, DESC_LEN);	\
		memcpy(desc, DESC, DESC_LEN);					\
		op->desc_len = DESC_LEN;						\
	}


#define MEMBER(NAME, NAME_LEN, TYPE, TYPE_LEN, DESC, DESC_LEN)			\
	{																	\
	int32_t type_index = map32_fetch(type_map, TYPE, TYPE_LEN);			\
	if (type_index == -1)												\
	{																	\
		type_index = class_stack.size;									\
		gdscript_class_t* temp = stack_alloc(class_stack, sizeof *temp); \
		memset(temp, 0x00, sizeof *temp);								\
		temp->name = str_arena.size;									\
		char* class_name = arena_alloc(str_arena, TYPE_LEN);			\
		memcpy(class_name, TYPE, TYPE_LEN);								\
		map32_cache(type_map, class_name, TYPE_LEN, ret_index);			\
	}																	\
	member_t* m = stack_alloc(member_stack, sizeof *m);					\
	*m = (member_t)														\
	{																\
		.name = str_arena->size,									\
		.desc = str_arena->size + NAME_LEN,							\
		.type_index = tpe_index,									\
		.name_len = NAME_LEN,										\
		.dsec_len = DESC_LEN										\
	};																\
	char* name = arena_alloc(str_arena, NAME_LEN);						\
	memcpy(name, NAME, NAME_LEN);										\
	char* desc = arena_alloc(str_arena, DESC_LEN);						\
	memcpy(desc, DESC, DESC_LEN);										\
	gdscript_class_t* c = class_stack->allocation + last_class;			\
	c->num_members++;													\
	}

#endif /* End BUILTIN_MACROS_H */
