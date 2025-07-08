#ifndef STACK_H
#define STACK_H

#include <stdint.h>

typedef struct stack_t
{
	uint8_t* allocation;
	int32_t item_size;
	int32_t cap;
	int32_t size;
} stack_t;

void stack_push(stack_t* stack, void* src);

void* stack_pop(stack_t* stack);

void stack_free_all(stack_t* stack);


#endif /* End STACK_H */
