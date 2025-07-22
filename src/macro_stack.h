#ifndef MACRO_STACK_H
#define MACRO_STACK_H

#include <stdint.h>
#include <assert.h>

#define STACK(NAME, TYPE)	\
typedef struct NAME##_stack \
{ \
	TYPE* allocation; \
	int32_t cap; \
	int32_t size; \
} NAME##_stack; \
\
static void NAME##_stack_push(NAME##_stack* const stack, const TYPE value) \
{ \
	stack->allocation[stack->size++] = value; \
} \
\
static const TYPE NAME##_stack_pop(NAME##_stack* const stack) \
{ \
	return stack->allocation[--stack->size]; \
} \
\
static const TYPE NAME##_stack_peek(const NAME##_stack* stack) \
{ \
	return stack->allocation[stack->size - 1]; \
} \
static const int32_t NAME##_stack_has(const NAME##_stack* stack, const TYPE value) \
{ \
	const TYPE* items = stack->allocation; \
	for (int32_t i = stack->size - 1; i >= 0; --i) \
	{ \
		if (items[i] == value) return i; \
	} \
	return -1; \
}

#endif /* End MACRO_STACK_H */
