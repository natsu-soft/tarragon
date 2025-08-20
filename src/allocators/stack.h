#ifndef STACK_H
#define STACK_H

#include <stdint.h>

// honestly, this is just used for the xml parser
typedef struct stack_t
{
	uint8_t* allocation;
	int32_t cap;
	int32_t prev;
	int32_t head;
} stack_t;

void stack_init(stack_t* stack, const int32_t bytes);

void* stack_alloc(stack_t* stack, const int32_t bytes);

void* stack_alloc_zero_pad(stack_t* stack, const int32_t bytes);

void stack_free(stack_t* stack);

const void* stack_peek(const stack_t* stack);

void stack_free_all(stack_t* stack);

#endif /* End STACK_H */
