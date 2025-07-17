#include "gdscript_shunt.h"
/* #include <stdlib.h> */
#include <stdint.h>
#include "token.h"
#include "gdscript_ids.h"
#include "../macro_stack.h"
STACK(i32, int32_t)
#include "../allocators/scratch.h"

#define true 0x01
#define false 0x00
typedef int8_t bool_t;

enum __attribute__((packed)) shunt_enum
{
	SHUNT_NONE = 0x00,
	SHUNT_OPERATOR = 0x01,
	SHUNT_OPEN = 0x02,
	SHUNT_DUMP = 0x04,
	SHUNT_EXIT = 0x08,
	SHUNT_VARIABLE = 0x10,
	SHUNT_UNEXPECTED = 0x20,
};

static inline const bool_t is_unary(const gdscript_id_t id)
{
	return 1 - (id < GDSCRIPT_OP_BITWISE_NOT | GDSCRIPT_OP_BOOL_NOT < id);
}

static int32_t shunt(token_table_t* const dest, const token_table_t* src, i32_stack* const stack, int32_t* swap0, const int32_t n);
static int32_t shunt_pair(token_table_t* const dest, const token_table_t* src, i32_stack* const stack, int32_t* swap0, const gdscript_id_t stop_id, const int32_t n);
static int32_t shunt_variable(token_table_t* const dest, const token_table_t* src, i32_stack* const stack, int32_t* swap0, const int32_t n);


/*******************/
/* CLASSIFIER HELL */
/*******************/


static int8_t default_mask(const gdscript_id_t id)
{
	return 0x00;
}


/****************/
/* PROPPA SHUNT */
/****************/


static int32_t shunt(token_table_t* const dest, const token_table_t* src, i32_stack* const stack, int32_t* swap0, const int32_t n)
{
	const int32_t* offsets0 = src->offsets;
	const int16_t* lens0 = src->lens;
	const gdscript_id_t* ids0 = src->ids;
	int32_t* offsets1 = dest->offsets;
	int16_t* lens1 = dest->lens;
	gdscript_id_t* ids1 = dest->ids;
	int32_t swap = 0;
	gdscript_id_t prev_id = GDSCRIPT_NONE;
	for (int32_t i = 0; i < n; ++i)
	{
		const gdscript_id_t id = ids0[i];
		const int8_t id_mask = default_mask(id);
		if (id_mask & SHUNT_OPERATOR)
		{
			i32_stack_push(stack, i);
		}
		else if (id_mask & SHUNT_OPEN)
		{
			// TODO: sensible handling
			// for now, keep the group if it's preceeded by an identifier
			// (function calls, array[index])
			// doesn't handle weird cases but I'll get to that when im not eepy
			i32_stack short_stack =
			{
				.allocation = stack->allocation + stack->size,
				.cap = n - stack->size
			};
			/* i = shunt_pair(dest, src, &short_stack, i, n, &swap, id - 3, prev_id == GDSCRIPT_IDENTIFIER); */
			prev_id = ids0[i];
			continue;
		}
		else if (id_mask & (SHUNT_DUMP | SHUNT_EXIT))
		{
			for (int32_t j = stack->size; j > 0; --j)
			{
				const int32_t index = i32_stack_pop(stack);
				offsets1[swap] = offsets0[index];
				lens1[swap] = lens0[index];
				ids1[swap] = ids0[index];
				++swap;
			}
			offsets1[swap] = offsets0[i];
			lens1[swap] = lens0[i];
			ids1[swap] = ids0[i];
			++swap;
			if (id_mask & SHUNT_EXIT)
			{
				*swap0 = swap;
				return i;
			}
		}
		else if (id_mask & SHUNT_VARIABLE)
		{
			offsets1[swap] = offsets0[i];
			lens1[swap] = lens0[i];
			ids1[swap] = ids0[i];
			++swap;
			/* shunt_variable(); */ // shunt til eol or assignment
		}
		else if (id_mask & SHUNT_UNEXPECTED)
		{
			// elevate id[i] to an error
		}
		else
		{
			offsets1[swap] = offsets0[i];
			lens1[swap] = lens0[i];
			ids1[swap] = ids0[i];
			++swap;
			while (is_unary(ids0[i32_stack_peek(stack)]))
			{
				const int32_t index = i32_stack_pop(stack);
				offsets1[swap] = offsets0[index];
				lens1[swap] = lens0[index];
				ids1[swap] = ids0[index];
				++swap;
			}
			if (stack->size > 0)
			{
				const int32_t index = i32_stack_pop(stack);
				offsets1[swap] = offsets0[index];
				lens1[swap] = lens0[index];
				ids1[swap] = ids0[index];
				++swap;
			}
		}
		prev_id = id;
	}
	/* return swap; */
	return n;
}

#define MISSING_DELTA = GDSCRIPT_MISSING_PAREN_CLOSE - GDSCRIPT_PAREN_CLOSE;
int32_t shunt_pairs(token_table_t* const dest, const token_table_t* src, i32_stack* const stack, int32_t i0, const int32_t n, int32_t* swap0, const gdscript_id_t stop_id, const bool_t keep_closing_pair)
{
	const int32_t* offsets = src->offsets;
	const int16_t* lens = src->lens;
	const gdscript_id_t* ids = src->ids;
	int32_t* dest_offsets = dest->offsets;
	int16_t* dest_lens = dest->lens;
	gdscript_id_t* dest_ids = dest->ids;
	int32_t swap = *swap0;
	gdscript_id_t prev_id = GDSCRIPT_NONE;
	if (keep_closing_pair)
	{
		dest_offsets[swap] = offsets[i0];
		dest_lens[swap] = lens[i0];
		dest_ids[swap] = ids[i0];
		++swap;
	}
	for (int32_t i = i0 + 1 ; i < n; ++i)
	{
		const gdscript_id_t id = ids[i];
		uint8_t id_mask = 0x00;
		// OPERATOR TOKEN
#define X(ID,...) id_mask |= (id == GDSCRIPT_##ID) << 0;
		GDSCRIPT_NON_ASSIGNMENT_OPERATORS
#undef X
		id_mask |= (id == GDSCRIPT_COLON) << 0;
		id_mask |= (id == GDSCRIPT_PERIOD) << 0;
		// OPEN TOKEN
		id_mask |= (id == GDSCRIPT_PAREN_OPEN) << 1;
		id_mask |= (id == GDSCRIPT_BRACE_OPEN) << 1;
		id_mask |= (id == GDSCRIPT_BRACKET_OPEN) << 1;
		// DUMP TOKEN
		id_mask |= (id == GDSCRIPT_LINE_END) << 2;
		id_mask |= (id == GDSCRIPT_IF) << 2;
		id_mask |= (id == GDSCRIPT_ELSE) << 2;
		id_mask |= (id == GDSCRIPT_EOF) << 2;
#define X(ID,...) id_mask |= (id == GDSCRIPT_##ID) << 2;
		GDSCRIPT_ASSIGNMENT_OPERATORS
#undef X
		// END TOKEN
		id_mask |= (id == stop_id) << 3;
		// CONST OR VAR KEYWORD
		/* id_mask |= (id == GDSCRIPT_CONST) << 4; */
		/* id_mask |= (id == GDSCRIPT_VAR) << 4; */
		// UNEXPECTED TOKENS
		/* id_mask |= (id == GDSCRIPT_COMMA) << 5; */
		id_mask |= (id == GDSCRIPT_CONST) << 5;
		id_mask |= (id == GDSCRIPT_VAR) << 5;
		// all keywords (except if & else) tbqh
		if (id_mask & SHUNT_OPERATOR)
		{
			i32_stack_push(&stack, i);
		}
		else if (id_mask & SHUNT_OPEN)
		{
			// TODO: sensible handling
			// for now, keep the group if it's preceeded by an identifier
			// (function calls, array[index])
			// doesn't handle weird cases but I'll get to that when im not eepy
			i32_stack short_stack =
			{
				.allocation = stack->allocation + stack->size,
				.cap = n - stack->size
			};
			/* i = shunt_pairs(dest, src, &short_stack, i, &swap, id - 3, prev_id == GDSCRIPT_IDENTIFIER); */
			prev_id = ids[i];
		}
		else if (id_mask & (SHUNT_DUMP | SHUNT_EXIT))
		/* else if (id_mask & SHUNT_DUMP) */
		{
			for (int32_t j = stack->size; j > 0; --j)
			{
				const int32_t index = i32_stack_pop(stack);
				dest_offsets[swap] = offsets[index];
				dest_lens[swap] = lens[index];
				dest_ids[swap] = ids[index];
				++swap;
			}
			if (id_mask & SHUNT_DUMP || (keep_closing_pair && (id_mask & SHUNT_EXIT)))
			{
				dest_offsets[swap] = offsets[i];
				dest_lens[swap] = lens[i];
				dest_ids[swap] = ids[i];
				++swap;
			}
			if (id_mask & SHUNT_EXIT)
			{
				*swap0 = swap;
				return i;
			}
		}
		else if (id_mask & SHUNT_UNEXPECTED)
		{
			// elevate id[i] to an error
		}
		else
		{
			dest_offsets[swap] = offsets[i];
			dest_lens[swap] = lens[i];
			dest_ids[swap] = ids[i];
			++swap;
			while (is_unary(ids[i32_stack_peek(stack)]))
			{
				const int32_t index = i32_stack_pop(stack);
				dest_offsets[swap] = offsets[index];
				dest_lens[swap] = lens[index];
				dest_ids[swap] = ids[index];
				++swap;
			}
			if (stack->size > 0)
			{
				const int32_t index = i32_stack_pop(stack);
				dest_offsets[swap] = offsets[index];
				dest_lens[swap] = lens[index];
				dest_ids[swap] = ids[index];
				++swap;
			}
		}
		prev_id = id;
	}
	return n;
}

void gdscript_shunt(token_table_t* dest, const token_table_t* src)
{
	const int32_t n = src->num_tokens;
	SCRATCH *scratch = scratch_checkout_size(4 * n);
	i32_stack stack =
	{
		.allocation = scratch->allocation,
		.cap = n
	};
	int32_t swap = 0;
	dest->num_tokens = shunt(dest, src, &stack, &swap, n);
}
