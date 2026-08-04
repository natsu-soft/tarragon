#ifndef MACRO_STACK_H
#define MACRO_STACK_H

#include <stdint.h>
#include <assert.h>

#define STACK(NAME, TYPE)	\
struct NAME##_stack \
{ \
	TYPE* buffer; \
	int cap; \
	int size; \
}; \
\
static inline void NAME##_stack_push(struct NAME##_stack* stack, const TYPE value) \
{ \
	stack->buffer[stack->size++] = value; \
} \
\
static inline TYPE NAME##_stack_pop(struct NAME##_stack* stack) \
{ \
	return stack->buffer[--stack->size]; \
} \
\
static inline TYPE NAME##_stack_peek(struct NAME##_stack* stack) \
{ \
	return stack->buffer[stack->size - 1]; \
} \

#endif /* End MACRO_STACK_H */
