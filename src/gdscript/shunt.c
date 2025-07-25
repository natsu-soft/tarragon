#include "shunt.h"
#include <stdlib.h>
#include <assert.h>
#include "token.h"
#include "../allocators/arena.h"
#include "../allocators/scratch.h"
#include "../macro_stack.h"
STACK(i32, int32_t)
#include "gdscript_ids.h"

#define true 1
#define false 0
typedef int8_t bool_t;

#define NOT_DELTA (GDSCRIPT_OP_BOOL_NOT - GDSCRIPT_NOT)


// NOTE: unused.  Actually figure out if this is better, ya?
/* static struct */
/* { */
/* 	gdscript_id_t* assignments; */
/* 	gdscript_id_t* operators; */
/* 	int8_t num_assignments; */
/* 	int8_t num_operators; */
/* } shunt_table; */

/* __attribute__((constructor)) */
/* static void init_shunt(void) */
/* { */
/* 	int32_t i = 0; */
/* #define X(...) ++i; */
/* 	GDSCRIPT_ASSIGNMENT_OPERATORS */
/* 	shunt_table.num_assignments = i; */
/* 	i = 0; */
/* 	GDSCRIPT_REGULAR_OPERATORS */
/* 	shunt_table.num_operators = i; */
/* #undef X */
/* 	gdscript_id_t* assignments = malloc((shunt_table.num_assignments + i) * sizeof(gdscript_id_t)); */
/* 	assert(assignments != NULL && "wow ur fukt"); */
/* 	gdscript_id_t* operators = assignments + shunt_table.num_assignments; */
/* 	i = 0; */
/* #define X(ID,...) assignments[i++] = GDSCRIPT_##ID; */
/* 	GDSCRIPT_ASSIGNMENT_OPERATORS */
/* #undef X */
/* 	i = 0; */
/* #define X(ID,...) operators[i++] = GDSCRIPT_##ID; */
/* 	GDSCRIPT_REGULAR_OPERATORS */
/* #undef X */
/* } */


/**************/
/* PREDICATES */
/**************/

static const bool_t keep_pair_p(const gdscript_id_t id, const gdscript_id_t prev_id)
{
	return id != GDSCRIPT_PAREN_OPEN ||
		id == GDSCRIPT_BRACKET_OPEN ||
		prev_id == GDSCRIPT_IDENTIFIER ||
#define X(ID,...) prev_id == GDSCRIPT_##ID ||
		GDSCRIPT_ANNOTATIONS
#undef X
		prev_id == GDSCRIPT_SUPER;
}

static const bool_t unary_p(const gdscript_id_t id)
{
	return GDSCRIPT_OP_BITWISE_NOT <= id && id <= GDSCRIPT_OP_BOOL_NOT;
}

enum __attribute__((packed)) shunt_enum
{
	SHUNT_NONE = 0x00,
	SHUNT_OPERATOR = 0x01,
	SHUNT_PAIR = 0x02,
	SHUNT_DUMP = 0x04,
	SHUNT_EXIT = 0x08,
};

static const int8_t vaniglia_mask(const gdscript_id_t id)
{
#define X(ID, ...) (id == GDSCRIPT_##ID) |
	const uint8_t operator =
		GDSCRIPT_REGULAR_OPERATORS
		(id == GDSCRIPT_NOT);
	const uint8_t pair =
		(id == GDSCRIPT_PAREN_OPEN) |
		(id == GDSCRIPT_BRACE_OPEN) |
		(id == GDSCRIPT_BRACKET_OPEN);
	const uint8_t dump =
		GDSCRIPT_ASSIGNMENT_OPERATORS
		GDSCRIPT_KEYWORDS
		(id == GDSCRIPT_COLON) |
		(id == GDSCRIPT_LINE_END) |
		(id == GDSCRIPT_EOF) |
		(id == GDSCRIPT_PAREN_CLOSE) |
		(id == GDSCRIPT_BRACE_CLOSE) |
		(id == GDSCRIPT_BRACKET_CLOSE);
#undef X
	return operator | (pair << 1) | (dump << 2);
}

static void dump_to_binary(token_table_t* dest, const token_table_t* src, i32_stack* stack, int32_t* swap0)
{
	const int32_t* offsets0 = src->offsets;
	const int16_t* lens0 = src->lens;
	const gdscript_id_t* ids0 = src->ids;
	int32_t* offsets = dest->offsets;
	int16_t* lens = dest->lens;
	gdscript_id_t* ids = dest->ids;
	int32_t swap = *swap0;
	for (int32_t i = stack->size; i > 0; --i)
	{
		const int32_t index = i32_stack_pop(stack);
		const int32_t id = ids0[index];
		offsets[swap] = offsets0[index];
		lens[swap] = lens0[index];
		ids[swap] = ids0[index];
		++swap;
		if(!unary_p(id))
		{
			break;
		}
	}
	*swap0 = swap;
}

static const int32_t shunt_period(token_table_t* dest, const token_table_t* src, i32_stack* stack, const int32_t i0, int32_t* swap0, const int32_t n)
{
	const int32_t* offsets0 = src->offsets;
	const int16_t* lens0 = src->lens;
	gdscript_id_t* ids0 = src->ids;
	int32_t* offsets = dest->offsets;
	int16_t* lens = dest->lens;
	gdscript_id_t* ids = dest->ids;
	int32_t swap = *swap0;
	for (int32_t i = i0; i < n; ++i)
	{
		const gdscript_id_t id = ids0[i];
		if (id == GDSCRIPT_IDENTIFIER)
		{
			offsets[swap] = offsets0[i];
			lens[swap] = lens0[i];
			ids[swap] = ids0[i];
			++swap;
		}
		else if (id == GDSCRIPT_PERIOD)
		{
			i32_stack_push(stack, i);
		}
		else
		{
			const int32_t m = stack->size;
			const int32_t* indices = stack->allocation;
			for (int32_t j = m; j > 0; --j)
			{
				const int32_t index = indices[j - 1];
				offsets[swap + m - j] = offsets0[index];
				lens[swap + m - j] = lens0[index];
				ids[swap + m - j] = ids0[index];
			}
			*swap0 = swap + m;
			return i - 1;
		}
	}
	*swap0 = swap;
	return n;
}

static int32_t shunt_pairs(token_table_t* dest, const token_table_t* src, i32_stack* stack, const int32_t i0, int32_t* swap0, const bool_t keep_pairs, const gdscript_id_t stop_id, const int32_t n)
{
	const int32_t* offsets0 = src->offsets;
	const int16_t* lens0 = src->lens;
	gdscript_id_t* ids0 = src->ids;
	int32_t* offsets = dest->offsets;
	int16_t* lens = dest->lens;
	gdscript_id_t* ids = dest->ids;
	int32_t swap = *swap0;
	gdscript_id_t prev_id = stop_id + 3;
	if (keep_pairs)
	{
		offsets[swap] = offsets0[i0];
		lens[swap] = lens0[i0];
		ids[swap] = ids0[i0];
		++swap;
	}
	for (int32_t i = i0; i < n; ++i)
	{
		const gdscript_id_t id = ids0[i];
		const gdscript_id_t next_id = i + 1 < n ? ids0[i + 1] : GDSCRIPT_NONE;
		const int8_t id_mask = vaniglia_mask(id);
		if(id_mask & SHUNT_OPERATOR)
		{
			ids0[i] += (id == GDSCRIPT_NOT) * NOT_DELTA;
			if (id != GDSCRIPT_IS && next_id != GDSCRIPT_NOT)
			{
				i32_stack_push(stack, i);
			}
			else
			{
				ids0[i + 1] = GDSCRIPT_OP_BOOL_NOT;
				i32_stack_push(stack, i + 1);
				i32_stack_push(stack, i);
				++i;
			}
		}
		else if(id_mask & SHUNT_DUMP)
		{
			const int32_t m = stack->size;
			const int32_t* indices = stack->allocation;
			for (int32_t j = m; j > 0; --j)
			{
				const int32_t index = indices[j - 1];
				offsets[swap + m - j] = offsets0[index];
				lens[swap + m - j] = lens[index];
				ids[swap + m - j] = ids[index];
			}
			swap += m;
			if (id != stop_id || keep_pairs) // NOTE: THNK ABOUT IT
			{
				offsets[swap] = offsets0[i];
				lens[swap] = lens0[i];
				ids[swap] = ids0[i];
				++swap;
			}
			if (id == stop_id)
			{
				*swap0 = swap;
				return i - 1;
			}
		}
		else if (id == GDSCRIPT_PERIOD)
		{
			const int32_t N = stack->size;
			i32_stack short_stack =
			{
				.allocation = stack->allocation + N,
				.cap = n - N,
			};
			i = shunt_period(dest, src, &short_stack, i, &swap, n);
			prev_id = ids0[i];
			continue;
		}
		else if (id_mask & SHUNT_PAIR)
		{
			const int32_t N = stack->size;
			i32_stack short_stack =
			{
				.allocation = stack->allocation + N,
				.cap = n - N,
			};
			const bool_t keep_flag = keep_pair_p(id, prev_id);
			const gdscript_id_t stop_id = id - 3;
			i = shunt_pairs(dest, src, &short_stack, i, &swap, keep_flag, stop_id, n);
			prev_id = stop_id;
			continue;
		}
		else
		{
			offsets[swap] = offsets0[i];
			lens[swap] = lens0[i];
			ids[swap] = ids0[i];
			++swap;
			if (next_id != GDSCRIPT_PERIOD && (next_id < GDSCRIPT_PAREN_OPEN || GDSCRIPT_BRACKET_OPEN < next_id))
			{
				dump_to_binary(dest, src, stack, &swap);
			}
		}
		prev_id = id;
	}
	*swap0 = swap;
	return n;
}

static const int32_t shunt_vaniglia(token_table_t* dest, const token_table_t* src, i32_stack* stack, const int32_t n)
{
	const int32_t* offsets0 = src->offsets;
	const int16_t* lens0 = src->lens;
	gdscript_id_t* ids0 = src->ids;
	int32_t* offsets = dest->offsets;
	int16_t* lens = dest->lens;
	gdscript_id_t* ids = dest->ids;
	int32_t swap = 0;
	gdscript_id_t prev_id = GDSCRIPT_NONE;
	for (int32_t i = 0; i < n; ++i)
	{
		const gdscript_id_t id = ids0[i];
		const gdscript_id_t next_id = i + 1 < n ? ids0[i + 1] : GDSCRIPT_NONE;
		const int8_t id_mask = vaniglia_mask(id);
		if(id_mask & SHUNT_OPERATOR)
		{
			ids0[i] += (id == GDSCRIPT_NOT) * NOT_DELTA;
			if (id != GDSCRIPT_IS && next_id != GDSCRIPT_NOT)
			{
				i32_stack_push(stack, i);
			}
			else
			{
				ids0[i + 1] = GDSCRIPT_OP_BOOL_NOT;
				i32_stack_push(stack, i + 1);
				i32_stack_push(stack, i);
				++i;
			}
		}
		else if(id_mask & SHUNT_DUMP)
		{
			const int32_t N = stack->size;
			const int32_t* indices = stack->allocation;
			for (int32_t j = N; j > 0; --j)
			{
				const int32_t index = indices[j - 1];
				offsets[swap + N - j] = offsets0[index];
				lens[swap + N - j] = lens[index];
				ids[swap + N - j] = ids[index];
			}
			offsets[swap + N] = offsets0[i];
			lens[swap + N] = lens0[i];
			ids[swap + N] = ids0[i];
			swap += N + 1;
		}
		else if (id == GDSCRIPT_PERIOD)
		{
			const int32_t N = stack->size;
			i32_stack short_stack =
			{
				.allocation = stack->allocation + N,
				.cap = n - N,
			};
			i = shunt_period(dest, src, &short_stack, i, &swap, n);
			prev_id = ids0[i];
			continue;
		}
		else if (id_mask & SHUNT_PAIR)
		{
			const int32_t N = stack->size;
			i32_stack short_stack =
			{
				.allocation = stack->allocation + N,
				.cap = n - N,
			};
			const bool_t keep_flag = keep_pair_p(id, prev_id);
			const gdscript_id_t stop_id = id - 3;
			i = shunt_pairs(dest, src, &short_stack, i, &swap, keep_flag, stop_id, n);
			prev_id = stop_id;
			continue;
		}
		else
		{
			offsets[swap] = offsets0[i];
			lens[swap] = lens0[i];
			ids[swap] = ids0[i];
			++swap;
			if (next_id != GDSCRIPT_PERIOD && (next_id < GDSCRIPT_PAREN_OPEN || GDSCRIPT_BRACKET_OPEN < next_id))
			{
				dump_to_binary(dest, src, stack, &swap);
			}
		}
		prev_id = id;
	}
	return swap;
}

const int32_t shunt(arena_t* arena, token_table_t* dest, const token_table_t* src, const int32_t n)
{
	dest->offsets = arena->allocation;
	dest->lens = dest->offsets + n;
	dest->ids = dest->lens + n;
	if (n > 0)
	{
		SCRATCH* scratch = scratch_checkout_size(n * sizeof(int32_t));
		i32_stack stack =
		{
			.allocation = scratch->allocation,
			.cap = n
		};
		int32_t res = shunt_vaniglia(dest, src, &stack, n);
		/* dest->src = src->src; */
		dest->num_tokens = res;
		return res;
	}
	else
	{
		dest->num_tokens = 0;
		return 0;
	}
}
