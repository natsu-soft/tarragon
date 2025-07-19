#include "gdscript_shunt.h"
#include <stdio.h>
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
	SHUNT_PAIR = 0x02,
	SHUNT_DUMP = 0x04,
	SHUNT_EXIT = 0x08,
	SHUNT_VARIABLE = 0x10,
	SHUNT_ERROR = 0x20,
};

static inline const bool_t is_unary(const gdscript_id_t id)
{
	// it's all good at -O2
	return !(id < GDSCRIPT_OP_BITWISE_NOT || GDSCRIPT_OP_BOOL_NOT < id) || id == GDSCRIPT_NOT;
}

// predicate
static const bool_t keep_pair_p(const gdscript_id_t id, const gdscript_id_t prev_id)
{
	return id == GDSCRIPT_BRACE_OPEN ||
		id == GDSCRIPT_BRACKET_OPEN ||
		prev_id == GDSCRIPT_IDENTIFIER ||
#define X(ID,...) prev_id == GDSCRIPT_##ID ||
		GDSCRIPT_ANNOTATIONS
#undef X
		prev_id == GDSCRIPT_SUPER;
}

static int32_t shunt(token_table_t* const dest, const token_table_t* src, i32_stack* const stack, const int32_t n);
static int32_t shunt_pairs(token_table_t* const dest, const token_table_t* src, i32_stack* const stack, const int32_t i0, int32_t* swap0, const gdscript_id_t stop_id, const bool_t keep_pairs, const int32_t n);
static int32_t shunt_variable(token_table_t* const dest, const token_table_t* src, i32_stack* const stack, const int32_t i0, int32_t* swap0, const int32_t n);
static int32_t shunt_period(token_table_t* const dest, const token_table_t* src, i32_stack* const stack, const int32_t i0, int32_t* swap0, const int32_t n);

static inline void print_id(const gdscript_id_t id)
{
	switch (id)
	{
	#define X(ID,...) case GDSCRIPT_##ID: printf(#ID); break;
	GDSCRIPT_BUILTIN_TOKENS
	#undef X
	case GDSCRIPT_IDENTIFIER:
		printf("IDENTIFIER");
		break;
	case GDSCRIPT_STRING:
		printf("STRING");
		break;
	case GDSCRIPT_NUMBER:
		printf("NUMBER");
		break;
	case GDSCRIPT_SCOPE_START:
		printf("SCOPE_START");
		break;
	case GDSCRIPT_SCOPE_END:
		printf("SCOPE_END");
		break;
	case GDSCRIPT_EOF:
		printf("EOF");
		break;
	default:
		printf("%#06x", id);
	}
}


/*******************/
/* CLASSIFIER HELL */
/*******************/

static const uint8_t default_mask(const gdscript_id_t id)
{
	const uint8_t operator =
		#define X(ID, ...) (id == GDSCRIPT_##ID) |
		GDSCRIPT_NON_ASSIGNMENT_OPERATORS
		#undef X
		/* (id == GDSCRIPT_PERIOD); */
		0x00;
	const uint8_t pair =
		(id == GDSCRIPT_PAREN_OPEN) |
		(id == GDSCRIPT_BRACE_OPEN) |
		(id == GDSCRIPT_BRACKET_OPEN);
	const uint8_t dump =
		#define X(ID, ...) (id == GDSCRIPT_##ID) |
		GDSCRIPT_ASSIGNMENT_OPERATORS
		#undef X
		#define X(ID, ...) (id == GDSCRIPT_##ID) |
		GDSCRIPT_KEYWORDS
		#undef X
		(id == GDSCRIPT_COLON) |
		(id == GDSCRIPT_LINE_END) |
		(id == GDSCRIPT_EOF);
	const uint8_t var = (id == GDSCRIPT_CONST) | (id == GDSCRIPT_VAR);
	const uint8_t error = (id == GDSCRIPT_COMMA);
	return operator | (pair << 1) | (dump << 2)  | (var << 4) | (error << 5);
	const uint8_t res =  operator | (pair << 1) | (dump << 2)  | (var << 4) | (error << 5);
	if (id == GDSCRIPT_VAR || id == GDSCRIPT_CONST)
	{
	printf("operator: %d\n", operator);
	printf("open: %d\n", pair);
	printf("dump: %d\n", dump);
	printf("var: %d\n", var);
	printf("error: %d\n", error);
	printf("union: %d\n", res);
	}
	return res;
}

const static uint8_t pair_mask(const gdscript_id_t id)
{
	const uint8_t operator =
		#define X(ID, ...) (id == GDSCRIPT_##ID) |
		GDSCRIPT_NON_ASSIGNMENT_OPERATORS
		#undef X
		/* (id == GDSCRIPT_COLON) | */
		/* (id == GDSCRIPT_PERIOD); */
		(id == GDSCRIPT_COLON);
	const uint8_t pair =
		(id == GDSCRIPT_PAREN_OPEN) |
		(id == GDSCRIPT_BRACE_OPEN) |
		(id == GDSCRIPT_BRACKET_OPEN);
	const uint8_t dump =
		#define X(ID, ...) (id == GDSCRIPT_##ID) |
		GDSCRIPT_ASSIGNMENT_OPERATORS
		#undef X
		#define X(ID, ...) (id == GDSCRIPT_##ID) |
		GDSCRIPT_KEYWORDS
		#undef X
		(id == GDSCRIPT_COMMA);
	const uint8_t exit =
		(id == GDSCRIPT_LINE_END) |
		(id == GDSCRIPT_EOF);
	const uint8_t var = (id == GDSCRIPT_CONST) | (id == GDSCRIPT_VAR);
	const uint8_t error = (id == GDSCRIPT_COMMA);
	return operator | (pair << 1) | (dump << 2) | (exit << 3) | (var << 4) | (error << 5);
}

/****************/
/* PROPPA SHUNT */
/****************/


static int32_t shunt(token_table_t* const dest, const token_table_t* src, i32_stack* const stack, const int32_t n)
{
	const int32_t* offsets0 = src->offsets;
	const int16_t* lens0 = src->lens;
	gdscript_id_t* ids0 = src->ids;
	int32_t* offsets1 = dest->offsets;
	int16_t* lens1 = dest->lens;
	gdscript_id_t* ids1 = dest->ids;
	int32_t swap = 0;
	gdscript_id_t prev_id = GDSCRIPT_NONE;
	for (int32_t i = 0; i < n; ++i)
	{
		const gdscript_id_t id = ids0[i];
		const uint8_t id_mask = default_mask(id);
		/* printf("%03d:%02x, ", i, id_mask); */
		/* print_id(id); */
		/* printf("\n"); */
		/* if (id == GDSCRIPT_VAR) */
		/* { */
		/* 	printf("var mask: %04x\n", id_mask); */
		/* } */
		if (id_mask & SHUNT_OPERATOR)
		{
			i32_stack_push(stack, i);
		}
		else if (id == GDSCRIPT_PERIOD)
		{
			i32_stack short_stack =
			{
				.allocation = stack->allocation + stack->size,
				.cap = n - stack->size
			};
			i = shunt_period(dest, src, &short_stack, i, &swap, n);
			prev_id = ids0[i];
			continue;
		}
		else if (id_mask & SHUNT_PAIR)
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
			const bool_t keep_flag = keep_pair_p(id, prev_id);
			i = shunt_pairs(dest, src, &short_stack, i, &swap, id - 3, keep_flag , n);
			prev_id = ids0[i];
			continue;
		}
		else if (id_mask & SHUNT_VARIABLE)
		{
			offsets1[swap] = offsets0[i];
			lens1[swap] = lens0[i];
			ids1[swap] = ids0[i];
			++swap;
			i32_stack short_stack =
			{
				.allocation = stack->allocation + stack->size,
				.cap = n - stack->size
			};
			i = shunt_variable(dest, src, &short_stack, i + 1, &swap, n);
			prev_id = ids0[i];
			continue;
		}
		else if (id_mask & SHUNT_DUMP)
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
			/* if (id_mask & SHUNT_EXIT) */
			/* { */
			/* 	*swap0 = swap; */
			/* 	return i; */
			/* } */
		}
		else if (id_mask & SHUNT_ERROR)
		{
			// demote id[i] to an error
		}
		else
		{
			offsets1[swap] = offsets0[i];
			lens1[swap] = lens0[i];
			ids1[swap] = ids0[i];
			++swap;
			const gdscript_id_t next_id = i + 1 < n ? ids0[i + 1] : GDSCRIPT_NONE;
			/* if (next_id != GDSCRIPT_PERIOD && (next_id < GDSCRIPT_PAREN_OPEN && GDSCRIPT_BRACKET_OPEN < next_id)) */
			if (next_id != GDSCRIPT_PERIOD && (next_id < GDSCRIPT_PAREN_OPEN || GDSCRIPT_BRACKET_OPEN < next_id))
			{
				while (stack->size > 0 && is_unary(ids0[i32_stack_peek(stack)]))
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
		}
		prev_id = id;
	}
	return swap;
}

#define MISSING_DELTA GDSCRIPT_MISSING_PAREN_CLOSE - GDSCRIPT_PAREN_CLOSE
static int32_t shunt_pairs(token_table_t* const dest, const token_table_t* src, i32_stack* const stack, const int32_t i0, int32_t* swap0, const gdscript_id_t stop_id, const bool_t keep_pairs, const int32_t n)
{
	const int32_t* offsets0 = src->offsets;
	const int16_t* lens0 = src->lens;
	gdscript_id_t* ids0 = src->ids;
	int32_t* offsets1 = dest->offsets;
	int16_t* lens1 = dest->lens;
	gdscript_id_t* ids1 = dest->ids;
	int32_t swap = *swap0;
	gdscript_id_t prev_id = GDSCRIPT_NONE;
	if (keep_pairs)
	{
		offsets1[swap] = offsets0[i0];
		lens1[swap] = lens0[i0];
		ids1[swap] = ids0[i0];
		++swap;
	}
	for (int32_t i = i0 + 1; i < n; ++i)
	{
		const gdscript_id_t id = ids0[i];
		const uint8_t id_mask = (id == stop_id) << 3 |
			(id == stop_id + MISSING_DELTA) << 3 |
			pair_mask(id);
		if (id_mask & SHUNT_OPERATOR)
		{
			i32_stack_push(stack, i);
		}
		else if (id == GDSCRIPT_PERIOD)
		{
			i32_stack short_stack =
			{
				.allocation = stack->allocation + stack->size,
				.cap = n - stack->size
			};
			i = shunt_period(dest, src, &short_stack, i, &swap, n);
			prev_id = ids0[i];
			continue;
		}
		else if (id_mask & SHUNT_PAIR)
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
			const bool_t keep_flag = keep_pair_p(id, prev_id);
			i = shunt_pairs(dest, src, &short_stack, i, &swap, id - 3, keep_flag, n);
			prev_id = ids0[i];
			continue;
		}
		/* else if (id_mask & SHUNT_VARIABLE) // NOTE: this should NOT trigger! */
		/* { */
		/* 	offsets1[swap] = offsets0[i]; */
		/* 	lens1[swap] = lens0[i]; */
		/* 	ids1[swap] = ids0[i]; */
		/* 	++swap; */
		/* 	i32_stack short_stack = */
		/* 	{ */
		/* 		.allocation = stack->allocation + stack->size, */
		/* 		.cap = n - stack->size */
		/* 	}; */
		/* 	i = shunt_variable(dest, src, &short_stack, i, &swap, n); */
		/* 	prev_id = ids0[i]; */
		/* 	continue; */
		/* } */
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
			if ((id_mask & SHUNT_DUMP) || (keep_pairs && id_mask & SHUNT_EXIT))
			{
				// else if shunt exit && keep pair keep
				offsets1[swap] = offsets0[i];
				lens1[swap] = lens0[i];
				ids1[swap] = ids0[i];
				++swap;
			}
			if (id_mask & SHUNT_EXIT)
			{
				*swap0 = swap;
				return i;
			}
		}
		else if (id_mask & SHUNT_ERROR)
		{
			// elevate id[i] to an error
		}
		else
		{
			offsets1[swap] = offsets0[i];
			lens1[swap] = lens0[i];
			ids1[swap] = ids0[i];
			++swap;
			const gdscript_id_t next_id = i + 1 < n ? ids0[i + 1] : GDSCRIPT_NONE;
			if (next_id != GDSCRIPT_PERIOD && (next_id < GDSCRIPT_PAREN_OPEN || GDSCRIPT_BRACKET_OPEN < next_id))
			{
				while (stack->size > 0 && is_unary(ids0[i32_stack_peek(stack)]))
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
		}
		prev_id = id;
	}
	return n;
}

static int32_t shunt_period(token_table_t* const dest, const token_table_t* src, i32_stack* const stack, const int32_t i0, int32_t* swap0, const int32_t n)
{
	/* printf(">shunting period\n"); */
	const int32_t* offsets0 = src->offsets;
	const int16_t* lens0 = src->lens;
	gdscript_id_t* ids0 = src->ids;
	int32_t* offsets1 = dest->offsets;
	int16_t* lens1 = dest->lens;
	gdscript_id_t* ids1 = dest->ids;
	int32_t swap = *swap0;
	for (int32_t i = i0; i < n; ++i)
	{
		const gdscript_id_t id = ids0[i];
		if (id == GDSCRIPT_IDENTIFIER)
		{
				offsets1[swap] = offsets0[i];
				lens1[swap] = lens0[i];
				ids1[swap] = ids0[i];
				++swap;
		}
		else if (id == GDSCRIPT_PERIOD)
		{
			i32_stack_push(stack, i);
		}
		else
		{
			for (int32_t j = stack->size; j > 0; --j)
			{
				const int32_t index = i32_stack_pop(stack);
				offsets1[swap] = offsets0[index];
				lens1[swap] = lens0[index];
				ids1[swap] = ids0[index];
				++swap;
			}
			*swap0 = swap;
			return i - 1;
		}
	}
	*swap0 = swap;
	return n;
}

static int32_t shunt_variable(token_table_t* const dest, const token_table_t* src, i32_stack* const stack, const int32_t i0, int32_t* swap0, const int32_t n)
{
	const int32_t* offsets0 = src->offsets;
	const int16_t* lens0 = src->lens;
	gdscript_id_t* ids0 = src->ids;
	int32_t* offsets1 = dest->offsets;
	int16_t* lens1 = dest->lens;
	gdscript_id_t* ids1 = dest->ids;
	int32_t swap = *swap0;
	for (int32_t i = i0; i < n; ++i)
	{
		const gdscript_id_t id = ids0[i];
		if (id == GDSCRIPT_IDENTIFIER)
		{
				offsets1[swap] = offsets0[i];
				lens1[swap] = lens0[i];
				ids1[swap] = ids0[i];
				++swap;
		}
		else if (id == GDSCRIPT_COLON)
		{
			i32_stack_push(stack, i);
		}
		else if (id == GDSCRIPT_PERIOD)
		{
			i32_stack short_stack =
			{
				.allocation = stack->allocation + stack->size,
				.cap = n - stack->size
			};
			i = shunt_period(dest, src, &short_stack, i, &swap, n);
		}
		else if  (id == GDSCRIPT_BRACKET_OPEN)
		{
			i32_stack short_stack =
			{
				.allocation = stack->allocation + stack->size,
				.cap = n - stack->size
			};
			i = shunt_pairs(dest, src, &short_stack, i, &swap, GDSCRIPT_BRACKET_CLOSE, true, n);
		}
		else
		{
			for (int32_t j = stack->size; j > 0; --j)
			{
				const int32_t index = i32_stack_pop(stack);
				offsets1[swap] = offsets0[index];
				lens1[swap] = lens0[index];
				ids1[swap] = ids0[index];
				++swap;
			}
			*swap0 = swap;
			return i - 1;
		}
	}
	*swap0 = swap;
	return n;
}

int32_t gdscript_shunt(token_table_t* const dest, const token_table_t* src, const int32_t n)
{
	SCRATCH *scratch = scratch_checkout_size(4 * n);
	i32_stack stack =
	{
		.allocation = scratch->allocation,
		.cap = n
	};
	/* memcpy(dest->offsets, src->offsets, 7 * n); */
	int32_t res = shunt(dest, src, &stack, n);
	dest->num_tokens = res;
	return res;
}
