#ifndef BUILTIN_MACROS_H
#define BUILTIN_MACROS_H

#define SUPPORTED_VERSION(NUM, VAL) supported_version.NUM = VAL;

#define GLOBAL_ENUM(NAME, LEN, BITFIELD)								\
	do																	\
	{																	\
	gdscript_enum_t* e = stack_alloc(enum_stack, sizeof *e);			\
	const int32_t name_pos = str_stack->size;							\
	*e = (gdscript_enum_t)												\
	{																	\
		.name = name_pos,												\
		.values = enum_val_stack->size,									\
		.name_len = LEN,												\
		.bitfield = BITFIELD											\
	};																	\
	char* name = stack_alloc(str_stack, LEN);							\
	memcpy(name, NAME, LEN);											\
	map32_cache(type_map, name, LEN, (uint32_t)((size_t)e - (size_t)enum_stack->allocation) | 0b1, str_stack->allocation); \
	++builtin_table.num_global_enums;									\
	} while(0);

#define ENUM_VALUE(NAME, NAME_LEN, VALUE, DESC, DESC_LEN)				\
	do																	\
	{																	\
		gdscript_enum_t* e = stack_peek(enum_stack);					\
		++(e->num_values);												\
		enum_value_t val = stack_alloc(enum_val_stack, sizeof *val);	\
		const int32_t name_pos = str_stack->size;						\
		*val = (enum_val_t)												\
		{																\
			.name = name_pos,											\
			.desc = str_stack->size,									\
			.value = VALUE,												\
			.name_len = NAME_LEN,										\
			.desc_len = DESC_LEN,										\
		};																\
		char* name = stack_alloc(str_stack, NAME_LEN);					\
		memcpy(name, NAME, LEN);										\
		map32_cache(str_map, name, LEN, name_pos, str_stack->allocation); \
		char* desc = stack_alloc(char_stack, DESC_LEN);					\
		memcpy(desc, DESC, DESC_LEN);									\
	} while(0)

#define UTILITY_FUNC(NAME, NAME_LEN, RET, RET_LEN, IS_VARARG)			\
	do																	\
	{																	\
	gdscript_func_t* f = stack_alloc(func_stack, sizeof *f);			\
	int32_t ret_index = map32_fetch(type_map, RET, RET_LEN, str_stack->allocation); \
	if (ret_index > -1)													\
	{																	\
		ret_index = class_stack.size;									\
		gdscript_class_t* temp = stack_alloc(class_stack, sizeof *temp); \
		*temp = (gdscript_class_t)										\
		{															\
			.name = str_stack.size,									\
			.name_len = RET_LEN										\
		};															\
		char* class_name = stack_alloc(str_stack, RET_LEN);				\
		memcpy(class_name, RET, RET_LEN);								\
		map32_cache(type_map, class_name, RET_LEN, ret_index, str_stack->allocation); \
	}																	\
	*f = (gdscript_func_t)												\
	{																\
		.name = str_stack->size,									\
		.params = param_stack->size,								\
		.name_len = NAME_LEN,										\
		.ret_index = ret_index,										\
		.class_index = -1,											\
		.name_len = NAME_LEN,										\
		.bitflags = IS_VARARG										\
	};																\
	++builtin_table.num_global_funcs;									\
	} while (0);

#define FUNC_PARAM(NAME, NAME_LEN, TYPE, TYPE_LEN)						\
	do																	\
	{																	\
	gdscript_func_t* f = stack_peek(func);								\
	func_param_t* p = stack_alloc(param_stack, sizeof *p);				\
	int32_t type_index = map32_fetch(type_map, RET, RET_LEN, str_stack->allocation); \
	if (type_index == -1)												\
	{																	\
		type_index = class_stack.size;									\
		gdscript_class_t* temp = stack_alloc(class_stack, sizeof *temp); \
		memset(temp, 0x00, sizeof *temp);								\
		temp->name = str_stack.size;									\
		char* class_name = stack_alloc(str_stack, RET_LEN);				\
		memcpy(class_name, TYPE, TYPE_LEN);								\
		map32_cache(type_map, class_name, TYPE_LEN, ret_index, str_stack->allocation); \
	}																	\
	*p = (func_param_t)													\
		{																\
			.name = str_stack->size,									\
			.name_len = NAME_LEN,										\
			.type_index = type_index									\
		};																\
	++(f->num_params);													\
	} while (0);

#define FUNC_DESC(DESC, DESC_LEN)						\
	do													\
	{													\
		gdscript_func_t* f = stack_peek(func);			\
		f->desc = str_stack->size;						\
		f->desc_len = DESC_LEN;							\
		char* desc = stack_alloc(str_stack, DESC_LEN); \
		memcpy(desc, DESC, DESC_LEN);					\
	} while (0);

#define CLASS(NAME, NAME_LEN, IS_KEYED)									\
	do																	\
	{																	\
		last_class = map32_fetch(type_map, NAME, NAME_LEN, str_stack->allocation); \
		gdscript_class_t* c;											\
		if (last_class > -1)											\
		{																\
			c = class_stack->allocation + last_class;					\
			c->bitflags |= IS_KEYED;									\
		}																\
		else															\
		{																\
			last_class = class_stack.size;								\
			c = stack_alloc(class_stack, sizeof *c);					\
			*c = (gdscript_class_t)										\
			{														\
				.name = str_stack.size,								\
				.name_len = NAME_LEN,								\
				.bitflags = IS_KEYED								\
			};														\
			char* name = stack_alloc(str_stack, RET_LEN);				\
			memcpy(name, NAME, NAME_LEN);								\
			map32_cache(type_map, name, NAME_LEN, last_class, str_stack->allocation); \
		}																\
	} while(0);

#define OPERATOR(ID, RET_NAME, RET_LEN)									\
	do																	\
	{																	\
	operator_t* op = stack_alloc(operator_stack, sizeof *op);			\
	*op = (operator_t)													\
	{																	\
		.id = ID,														\
		.right_index = -1												\
	};																	\
	} while(0);

#define OPERATOR_RETURN(RET_NAME, RET_LEN)								\
	do																	\
	{																	\
		int32_t ret_index = map32_fetch(TYPE_NAME, TYPE_LEN, str_stack->allocation); \
		if (ret_index == -1)											\
		{																\
			ret_index = class_stack.size;								\
			gdscript_class_t* temp = stack_alloc(class_stack, sizeof *temp); \
			memset(temp, 0x00, sizeof *temp);							\
			temp->name = str_stack.size;								\
			char* class_name = stack_alloc(str_stack, RET_LEN);			\
			memcpy(class_name, TYPE, TYPE_LEN);							\
			map32_cache(type_map, class_name, TYPE_LEN, ret_index, str_stack->allocation); \
		}																\
		operator_t* op = stack_peek(operator_stack);					\
		op->ret_index = ret_index;										\
	}																	\
	while (0);

#define OPERATOR_RIGHT(TYPE_NAME, TYPE_LEN)								\
	do																	\
	{																	\
	int32_t right_index = map32_fetch(TYPE_NAME, TYPE_LEN, str_stack->allocation); \
	if (right_index == -1)												\
	{																	\
		right_index = class_stack.size;									\
		gdscript_class_t* temp = stack_alloc(class_stack, sizeof *temp); \
		memset(temp, 0x00, sizeof *temp);								\
		temp->name = str_stack.size;									\
		char* class_name = stack_alloc(str_stack, RET_LEN);				\
		memcpy(class_name, TYPE, TYPE_LEN);								\
		map32_cache(type_map, class_name, TYPE_LEN, ret_index, str_stack->allocation); \
	}																	\
	operator_t* op = stack_peek(operator_stack);						\
	op->right_index = right_index;										\
	} while(0);

#define OPERATOR_DESC(DESC, DESC_LEN)					\
	do													\
	{													\
		operator_t* op = stack_peek(operator_stack);	\
		op->desc = str_stack->size;						\
		char* desc = stack_alloc(str_stack, DESC_LEN);	\
		memcpy(desc, DESC, DESC_LEN);					\
		op->desc_len = DESC_LEN;						\
	} while(0);

#endif /* End BUILTIN_MACROS_H */
