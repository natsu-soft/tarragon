#ifndef MACRO_STACK_H
#define MACRO_STACK_H

#include <stdint.h>
#include <assert.h>

#define STACK(NAME, TYPE)	\
typedef struct NAME##_stack \
{ \
	TYPE* allocation; \
	int32_t item_size; \
	int32_t cap; \
	int32_t size; \
} NAME##_stack; \
\
void NAME##_stack_push(NAME##_stack* const stack, const TYPE value) \
{ \
	stack->allocation[stack->size++] = value; \
} \
\
const TYPE NAME##_stack_pop(NAME##_stack* const stack) \
{ \
	return stack->allocation[--stack->size]; \
} \
\
const TYPE NAME##_stack_peek(const NAME##_stack* stack) \
{ \
	return stack->allocation[stack->size - 1]; \
}

#endif /* End MACRO_STACK_H */
