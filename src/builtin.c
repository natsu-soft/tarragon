#include "builtin.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include "map32.h"
#include "allocators/arena.h"
#include "allocators/scratch.h"

/******************************/
/* START STACK IMPLEMENTAITON */
/******************************/

/* completely unique stack that I totally needed to implement */

typedef struct stack_t
{
	uint8_t* allocation;
	int32_t size;
	int32_t cap;
	int32_t prev;
} stack_t;


static void* stack_alloc(stack_t* stack, int32_t size)
{
	if (stack->size + size > stack->cap)
	{
		void* addr = realloc(stack->allocation, stack->cap + size + 4096);
		assert(addr != NULL && "failed to realloc in builtin.c:stack_alloc");
		stack->allocation = addr;
	}
	stack->prev = stack->size;
	void* res = stack->allocation + stack->size;
	stack->size += size;
	return res;
}

static void* stack_alloc_aligned(stack_t* stack, int32_t size)
{
	if (stack->size + size > stack->cap)
	{
		void* addr = realloc(stack->allocation, stack->cap + size + 4096);
		assert(addr != NULL && "failed to realloc in builtin.c:stack_alloc");
		stack->allocation = addr;
	}
	stack->prev = stack->size;
	void* res = stack->allocation + stack->size;
	stack->size += size;
	return res;
}

static void stack_init(stack_t* stack, int32_t cap)
{
	void* addr = malloc(cap);
	assert(addr != NULL && "failed to malloc in builtin.c:stack_init");
	stack->allocation = addr;
	stack->size = 0;
	stack->cap = cap;
	stack->prev = -1;
}

static void* stack_peek(stack_t* stack)
{
	return stack->prev >= 0 ? stack->allocation + stack->prev : NULL;
}

static void stack_shrinkwrap(stack_t* stack)
{
	void* _ = realloc(stack->allocation, stack->size);
	// TODO: brain-off assertion
}

/****************************/
/* END STACK IMPLEMENTATION */
/****************************/

supported_version_t supported_version = { 0x00 };

struct
{
	gdscript_enum_t* global_enums;
	gdscript_func_t* global_funcs;
	int num_global_enums;
	int nmum_global_funcs;
} builtin_table = { 0x00 };

// what's the difference between strs & char? strs get hashed >:^)
// types is disgusting.  It includes enums as well as
#define STACKS \
	X(str) \
	X(enum) \
	X(enum_val) \
	X(func) \
	X(param) \
	X(class) \
	X(opretor)

#define MAPS \
	X(type, 2048)	 \
	// NOTE: values that end in 0x1 are enums!

void gdscript_builtin(void)
{
#define X(NAME) stack_t* NAME##_stack; stack_init(NAME##_stack, 4096);
	STACKS
#undef X
#define X(NAME, N) SCRATCH* NAME##_scratch = scratch_checkout_size(N * 4 * 10);
	MAPS
#undef X
#define X(NAME, N) \
	map32_t* NAME##_map = alloca(sizeof *NAME##_map); \
	NAME##_map->rkeys = NAME##_scratch->allocation; \
	NAME##_map->values = NAME##_map->rkeys + N; \
	NAME##_map->lens = NAME##_map->lens + N; \
	NAME##_map->cap = N; \
	NAME##_map->size = 0;
	MAPS
#undef X
	int32_t last_class;
#include "builtin_macros.h"
#include "builtin.gen"
#define X(NAME) shrinkwrap_stack(NAME##_stack);
	STACKS
#undef X
	// turn all relative ptrs into absoute ptrs
	return;
}


void print_supported_version(void)
{
	printf("%d.%d.%d\n",
		   supported_version.major,
		   supported_version.minor,
		   supported_version.patch);
}
