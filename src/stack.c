#include "stack.h"
/* #include <stdlib.h> */
#include <assert.h>
#include <string.h>



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

void stack_free_all(stack_t* stack)
{
	stack->size = 0;
}
