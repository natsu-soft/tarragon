#include "stack.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

typedef struct header_t
{
	int32_t prev;
} header_t;

static inline const int32_t align_header (const void* ptr)
{
	const int32_t mod = (int64_t)ptr & 3;
	return (mod > 0) * (4 - mod);
}

void stack_init(stack_t* stack, const int32_t bytes)
{
	/* stack->allocation = calloc(bytes, 1); */
	assert(bytes > 0);
	stack->allocation = malloc(bytes);
	stack_free_all(stack);
}

void* stack_alloc(stack_t* stack, const int32_t bytes)
{
	assert(stack->head + bytes < stack->cap && "stack past cap");
	header_t* header = stack->allocation + stack->head;
	const uint8_t* ptr = header + 1;
	header->prev = stack->prev;
	const uint8_t* tail = ptr + bytes;
	stack->head = (tail + align_header(tail)) - stack->allocation;
	return ptr;
}

void* stack_alloc_zero_pad(stack_t* stack, const int32_t bytes)
{
	assert(stack->head + bytes < stack->cap && "stack past cap");
	header_t* header = stack->allocation + stack->head;
	const uint8_t* ptr = header + 1;
	header->prev = stack->prev;
	stack->prev = stack->head;
	const uint8_t* tail = ptr + bytes;
	stack->head = (tail + align_header(tail)) - stack->allocation;
	memset(ptr + bytes, 0x00, stack->head - ((uint8_t*)tail - stack->allocation));
	return ptr;
}

void stack_free(stack_t* stack)
{
	if (stack->prev > -1)
	{
		header_t* header = stack->allocation + stack->prev;
		stack->head = stack->prev;
		--header;
		stack->prev = header->prev;
	}
}

const void* stack_peek(const stack_t* stack)
{
	return (void*)((stack->prev > 0) * (int64_t)((header_t*)(stack->allocation + stack->prev) + 1));
}

void stack_free_all(stack_t* stack)
{
	stack->prev = -1;
	stack->head = 0;
}
