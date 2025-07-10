#include "stack.h"
/* #include <stdlib.h> */
#include <assert.h>
#include <string.h>

#define X(BITSIZE) \
void stack_push##BITSIZE(stack_t* stack, const uint##BITSIZE##_t val) \
{ \
	const int32_t k = stack->item_size; \
	memcpy(stack->allocation + (k * stack->size++), &val, BITSIZE); \
}
STACK_LITERAL_BITSIZES
#undef X

#define X(BITSIZE) \
uint##BITSIZE##_t stack_pop##BITSIZE(stack_t* stack) \
{ \
	uint##BITSIZE##_t* res = stack->allocation + (stack->item_size * stack->size--); \
	return *res; \
}
STACK_LITERAL_BITSIZES
#undef X


#define X(BITSIZE) \
const uint##BITSIZE##_t stack_peek##BITSIZE(stack_t* stack) \
{ \
	uint##BITSIZE##_t* res = stack->allocation + (stack->item_size * stack->size); \
	return *res; \
}
STACK_LITERAL_BITSIZES
#undef X

void stack_push(stack_t* stack, void* src)
{
	// TODO: assertion
	const int32_t k = stack->item_size;
	memcpy(stack->allocation + (k * stack->size++), src, k);
}

void* stack_pop(stack_t* stack)
{
	// TODO: assertion
	return stack->allocation + (stack->item_size * stack->size--);
}

void* stack_peek(const stack_t* stack)
{
	return stack->allocation + (stack->item_size * stack->size);
}

void stack_free_all(stack_t* stack)
{
	stack->size = 0;
}
