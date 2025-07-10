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

#define STACK_LITERAL_BITSIZES \
	X(8) \
	X(16) \
	X(32) \
	X(64)

#define X(BITSIZE) void stack_push##BITSIZE(stack_t* stack, const uint##BITSIZE##_t val);
STACK_LITERAL_BITSIZES
#undef X

#define X(BITSIZE) uint##BITSIZE##_t stack_pop##BITSIZE(stack_t* stack);
STACK_LITERAL_BITSIZES
#undef X

#define X(BITSIZE) const uint##BITSIZE##_t stack_peek##BITSIZE(stack_t* stack);
STACK_LITERAL_BITSIZES
#undef X

void stack_push(stack_t* stack, void* src);

void* stack_pop(stack_t* stack);

void* stack_peek(const stack_t* stack);

void stack_free_all(stack_t* stack);


#endif /* End STACK_H */
