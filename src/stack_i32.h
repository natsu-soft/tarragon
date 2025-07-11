#ifndef STACK_I32_H
#define STACK_I32_H

#include <stdint.h>

typedef struct stack_i32_t
{
	int32_t* allocation;
	int32_t cap;
	int32_t size;
} stack_i32_t;


void stack_i32_init(stack_i32_t* const stack, const int32_t cap);

void stack_i32_push(stack_i32_t* const stack, const int32_t value);

const int32_t stack_i32_pop(stack_i32_t* const stack);

const int32_t stack_i32_peek(const stack_i32_t* stack);

#endif /* End STACK_I32_H */
