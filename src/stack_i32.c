#include "stack_i32.h"
#include <stdlib.h>
#include <assert.h>

void stack_i32_init(stack_i32_t* const stack, const int32_t cap)
{
	stack->allocation = malloc(cap);
	stack->cap = cap;
	stack->size = 0;
}

void stack_i32_push(stack_i32_t* const stack, const int32_t value)
{
	assert(stack->size < stack->cap && "STAKCC: lol lmao\n");
	stack->allocation[stack->size++] = value;
}

const int32_t stack_i32_pop(stack_i32_t* const stack)
{
	assert(stack->size > 0 && "STAKCC: lol lmao\n");
	return stack->allocation[--stack->size];
}

const int32_t stack_i32_peek(const stack_i32_t* stack)
{
	assert(stack->size > 0 && "STAKCC: lol lmao\n");
	return stack->allocation[stack->size - 1];
}
