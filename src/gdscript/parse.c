#define ASSERT_NON_NIL
#include "parse.h"
#include <assert.h>
#include <stdio.h>
#include "../error_macros.h"
/* #include <stdlib.h> */
#define DEFINE_TOKEN_PREDICATES
#include "token_ids_types.h"
#include "../linked_list.h"
#include "../allocators/arena.h"
#include "../macro_stack.h"
#include "../macro_bitset.h"

// NOTE: there is a relation between stack depth & elem depth
// but ternary expressions really fuck shit up fam
STACK(i32, int32_t)
BITSET_SPAN_TYPE(id, token_id_t)

struct context
{
	struct id_bitset* parent_tokens;
	arena_t* arena;
	int32_t* indices;
	int16_t* depths;
	int32_t* offsets;
	char* src;
	int16_t* lens;
	token_id_t* ids;
	int32_t num_tokens;
	int16_t lws;
};


// TODO: stop using trees.  Impossible, I'm afraid :(
// NOTE: I'm using child-junctions for my sibling-lists so I dont ever fuck-up with the tree-structure
struct parse_node
{
	LINKED_TREE(struct parse_node)
	int32_t index;
};
IMPL_LINKED_LIST_FUNCS(node, struct parse_node)

#define nil (&nil_node)
static struct parse_node nil_node =
{
	.next = nil,
	.prev = nil,
	.child = nil
};

// NOTE: debug this
static inline struct parse_node* alloc_node(arena_t* arena)
{
	struct parse_node* res = arena_alloc(arena, sizeof(struct parse_node));
	res->next = res;
	res->prev = res;
	return res;
}

static inline struct parse_node* alloc_jct(arena_t* arena)
{
	struct parse_node* res = arena_alloc(arena, sizeof(struct parse_node));
	*res = (struct parse_node)
	{
		.next = res,
		.prev = res,
		.child = nil,
	};
	return res;
}

static inline uint8_t no_children_p(const struct parse_node* node)
{
	return node->child == node->child->next;
}

static inline uint8_t one_child_p(const struct parse_node* node)
{
	return node->child != node->child->next && node->child == node->child->next->next;
}

static inline uint8_t two_children_p(const struct parse_node* node)
{
	return node->child != node->child->next && node->child->next->next == node->child->prev;
}

static inline uint8_t one_or_no_children_p(const struct parse_node* node)
{
	return node->child == node->child->next || node->child->next == node->child->prev;
}

static void node_sexp(struct parse_node* node, struct context* ctx)
{
	putc('(', stdout);
	struct parse_node* n = node;
	do
	{
		const int32_t i = n->index;
		switch(ctx->ids[i])
		{
		case GDSCRIPT_NONE: printf("%d:NONE", i - TOKEN_START); break;
		#define X(ID) case PARSE_##ID: printf("%d:PARSE_" #ID , i - TOKEN_START); break;
		PARSE_IDS
		#undef X
		case TOKEN_START: printf("%d:TOKEN_START", i - TOKEN_START); break;
		#define X(A,...) case GDSCRIPT_##A: printf("%d:GDSCRIPT_" #A, i - TOKEN_START); break;
		GDSCRIPT_BUILTIN_TOKENS
		#undef X
		default:
			printf("%d:%.*s", i - TOKEN_START, ctx->lens[i], ctx->src + ctx->offsets[i]);
		}
		if (n->child != nil)
		{
			putc(' ', stdout);
			node_sexp(n->child, ctx);
		}
		if (n->next != node)
		{
			putc(' ', stdout);
		}
		n = n->next;
	} while(n != node);
	putc(')', stdout);
}

enum token_pos
{
	CONT_CURRENT	= 0x00,
	AFTER_JCT		= 0x01,
	NEW_CURRENT		= 0x02,
};

enum parse_res
{
    PARSED_END = 0x00,
	AT_PARENT_TOKEN
};

struct lambda_res
{
	int16_t lws;
	enum parse_res parse_res;
};

static int16_t parse_lines(int32_t* cursor, int32_t* n, const int16_t depth, const int16_t lws_count, struct context* ctx);
static enum parse_res parse_line(struct parse_node** root, int32_t* cursor, struct context* ctx);
static enum parse_res parse_decl(struct parse_node** current, struct parse_node** last, int32_t* cursor, struct context* ctx);
static enum parse_res parse_to_pair(const token_id_t pair, struct parse_node* root, int32_t* cursor, struct context* ctx);
static struct lambda_res parse_lambda(struct parse_node* root, int32_t* cursor, struct context* ctx);
static enum parse_res parse_ternary_expr(struct parse_node* root, int32_t* cursor, struct context* ctx);
static void parse_period_expr(struct parse_node** root, int32_t* cursor, struct context* ctx);
static enum token_pos ingest_token(const token_id_t id, struct parse_node** current, struct parse_node** last, int32_t* cursor, struct context* ctx);

// as a humble warmup
int32_t shunt(parse_table_t* p_table, token_table_t* token_table, arena_t* backing_arena, arena_t* work_arena)
{
	arena_free_all(work_arena);
	struct i32_stack idx_stack =
	{
		.buffer = arena_alloc(work_arena, work_arena->cap),
		.cap = work_arena->cap >> 2
	};
	const int32_t num_tokens = token_table->num_tokens;
	token_id_t* ids = token_table->ids;
	/* int32_t* indices = arena_alloc(backing_arena, (num_tokens << 2) * sizeof *indices); */
	int32_t* indices = arena_alloc(backing_arena, 2 * num_tokens * sizeof *indices);
	int32_t n = 0;
	for (int32_t i = 0; i < num_tokens; ++i)
	{
		const token_id_t id = ids[i];
		const token_id_t peek_id = ids[i + 1];
		// keywords insert in place
		#define X(A,...) id == GDSCRIPT_##A ||
		if (!(GDSCRIPT_BINARY_OPERATORS GDSCRIPT_UNARY_OPERATORS id == GDSCRIPT_PERIOD || id == GDSCRIPT_COLON))
		#undef X
		{
			assert(n < num_tokens);
			indices[n++] = i;
		}
		else
		{
			i32_stack_push(&idx_stack, i);
		}
		#define DUMP_ALL \
			X(LINE_END) \
			X(PAREN_OPEN) \
			X(PAREN_CLOSE) \
			X(BRACE_OPEN) \
			X(BRACE_CLOSE) \
			X(BRACKET_OPEN) \
			X(BRACKET_CLOSE) \
			X(STRUCTURAL_COLON) \
			X(SCOPE_OPEN) \
			X(SCOPE_CLOSE) \
			GDSCRIPT_ASSIGNMENT_OPERATORS \
			GDSCRIPT_KEYWORDS \
			GDSCRIPT_ANNOTATIONS
		#define DUMP_PERIODS \
			GDSCRIPT_REGULAR_OPERATORS
		#define X(A,...) peek_id == GDSCRIPT_##A ||
		if (DUMP_ALL peek_id == GDSCRIPT_EOF)
		/* if (peek_id == GDSCRIPT_EOF) */
		{
			for (int32_t j = idx_stack.size - 1; j >= 0; --j)
			{
				assert(n < num_tokens);
				indices[n++] = idx_stack.buffer[j];
			}
			idx_stack.size = 0;
		}
		else if (DUMP_PERIODS peek_id == GDSCRIPT_COLON)
		/* else if (0) */
		{
			while (idx_stack.size > 0)
			{
				const int32_t idx = i32_stack_peek(&idx_stack);
				if (ids[idx] != GDSCRIPT_PERIOD)
				{
					break;
				}
				assert(n < num_tokens);
				indices[n++] = idx;
				idx_stack.size--;
			}
		}
		#undef X
	}
	p_table->token_indices = indices;
	p_table->size = n;
	return n;
}

// just parse, warts & all
int32_t parse(parse_table_t* p_table, token_table_t* token_table, arena_t* backing_arena, arena_t* work_arena)
{
	const int32_t num_tokens = token_table->num_tokens;
	struct id_bitset default_bitset = {0};
	struct context ctx =
	{
		.arena = work_arena,
		.indices = arena_alloc(backing_arena, (num_tokens << 1) * sizeof(int32_t)),
		.depths = arena_alloc(backing_arena, (num_tokens << 1) * sizeof(int16_t)),
		.offsets = token_table->offsets,
		.src = token_table->src,
		.lens = token_table->lens,
		.ids = token_table->ids,
		.num_tokens = num_tokens,
		.parent_tokens = &default_bitset
	};
	// also ZII the indices & depths for the parse table
	int32_t cursor = TOKEN_START; // token_table[0] & [1] are 0-values to make id lookup ezpz
	int32_t n = 0;
	ctx.indices[n] = 0;
	ctx.depths[n++] = 0;
	parse_lines(&cursor, &n, 0, 0, &ctx);
	p_table->token_indices = ctx.indices;
	p_table->depths = ctx.depths;
	// TODO: concat token_indices & depths
	p_table->size = n;
	ctx.indices[n + 1] = num_tokens + 1;
	ctx.depths[n + 1] = 0;
	return n;
}

int32_t parse_backing_size(const int32_t num_tokens)
{
	return 2 * num_tokens * (sizeof(struct parse_node));
}

// Gomen, NASA-chan
// NOTE: this only works IF node is junction--otherwise you WILL have MAJOR PROBLEMS
static void dump_postorder(struct parse_node* node, int32_t* indices, int16_t* depths, int32_t* n, const int16_t depth)
{
	assert(node->index == 0 && "NON-JCT\n");
	for(struct parse_node* p = node->next; p != node; p = p->next)
	{
		if (p->child != nil)
		{
			dump_postorder(p->child, indices, depths, n, depth + 1);
		}
		indices[*n] = p->index;
		depths[*n] = depth;
		++*n;
	}
}

static void print_postorder(struct parse_node* node, const int16_t depth, struct context* ctx)
{
	assert(node != NULL);
	assertf(node->index == 0, "NON-JUNCTION: %d:%.*s\n", node->index, ctx->lens[node->index], ctx->src + ctx->offsets[node->index]);
	for(struct parse_node* p = node->next; p != node; p = p->next)
	{
		if (p->child != nil)
		{
			print_postorder(p->child, depth + 1, ctx);
		}
		for (int16_t i = 0; i < 2 * depth; ++i)
		{
			putc(' ', stdout);
		}
		const int32_t i = p->index;
		switch(ctx->ids[i])
		{
		#define X(A,...) case GDSCRIPT_##A: printf("%d: GDSCRIPT_" #A "\n", i - TOKEN_START); break;
		GDSCRIPT_BUILTIN_TOKENS
		#undef X
		case GDSCRIPT_NONE: printf("%d: NONE\n", i); break;
		case TOKEN_START: printf("%d: TOKEN_START\n", i); break;
		#define X(ID) case PARSE_##ID: printf("%d: PARSE_" #ID "\n", i); break;
		PARSE_IDS
		#undef X
		default:
			printf("%d: %.*s\n", i - TOKEN_START, ctx->lens[i], ctx->src + ctx->offsets[i]);
		}
		assertf(p->child != NULL, "(%d:%.*s)'s child is NULL\n", i, ctx->lens[i], ctx->src + ctx->offsets[i]);
	}
}

static void print_preorder(struct parse_node* node, const int16_t depth, struct context* ctx)
{
	assert(node != NULL);
	assertf(node->index == 0, "NON-JUNCTION: %d:%.*s\n", node->index, ctx->lens[node->index], ctx->src + ctx->offsets[node->index]);
	for(struct parse_node* p = node->next; p != node; p = p->next)
	{
		for (int16_t i = 0; i < 2 * depth; ++i)
		{
			putc(' ', stdout);
		}
		const int32_t i = p->index;
		switch(ctx->ids[i])
		{
		#define X(A,...) case GDSCRIPT_##A: printf("%d: GDSCRIPT_" #A "\n", i - TOKEN_START); break;
		GDSCRIPT_BUILTIN_TOKENS
		#undef X
		case GDSCRIPT_NONE: printf("%d: NONE\n", i); break;
		case TOKEN_START: printf("%d: TOKEN_START\n", i); break;
		#define X(ID) case PARSE_##ID: printf("%d: PARSE_" #ID "\n", i); break;
		PARSE_IDS
		#undef X
		default:
			printf("%d: %.*s\n", i - TOKEN_START, ctx->lens[i], ctx->src + ctx->offsets[i]);
		}
		assertf(p->child != NULL, "(%d:%.*s)'s child is NULL\n", i, ctx->lens[i], ctx->src + ctx->offsets[i]);
		if (p->child != nil)
		{
			print_preorder(p->child, depth + 1, ctx);
		}
	}
}

static int16_t parse_lines(int32_t* cursor, int32_t* n, const int16_t depth, const int16_t lws_count, struct context* ctx)
{
	while (*cursor < ctx->num_tokens)
	/* for (int i = 0; i < 12; ++i) */
	{
		int16_t lws_actual = ctx->offsets[*cursor] - (ctx->offsets[*cursor - 1] + ctx->lens[*cursor - 1]);
		if (lws_actual == lws_count || ctx->ids[*cursor - 1] == GDSCRIPT_SEMICOLON)
		{
			arena_free_all(ctx->arena);
			struct parse_node* root = alloc_jct(ctx->arena);
			for (; *cursor < ctx->num_tokens && ctx->ids[*cursor] == GDSCRIPT_LINE_END; ++*cursor);
			parse_line(&root, cursor, ctx);
			/* printf("\npost-order:\n"); */
			/* print_postorder(root, depth, ctx); */
			/* putc('\n', stdout); */
			dump_postorder(root, ctx->indices, ctx->depths, n, depth);
		}
		else if (lws_actual > lws_count)
		{
			const int32_t n0 = *n;
			do
			{
				for (int32_t i = n0; i < *n; ++i) { ++ctx->depths[i]; }
				lws_actual = parse_lines(cursor, n, depth + 1, lws_actual, ctx);
				ctx->indices[*n] = PARSE_SCOPE;
				ctx->depths[*n] = depth;
				++*n;
			} while(lws_actual > lws_count);
		}
		else
		{
			return lws_actual;
		}
	}
	return lws_count;
}

/* NOTE: gdscript operands are LEFT-ASSOCIATIVE
   THIS INCLUDES PERIODS!
   i.e. 2 + 3 * 5 is (2 + 3) * 5
   i.e. 2 + 3 * 5
   -- becomes --
		  *
		 / \
		+  5
	   / \
	  2  3
*/
/* NOTE: PAIN. SUFFERING EVEN.
	A if B else C
	-- becomes --
	-TERN_EXPR-
	 |
	jct-if-B-else
		|     |
		A     C
	A if B C else TERN_EXPR

*/

static enum parse_res parse_line(struct parse_node** root, int32_t* cursor, struct context* ctx)
{
	struct parse_node* current = *root;
	struct parse_node* last = *root;
	struct context subctx = *ctx;
	struct id_bitset ctx_bitset = {0};
	id_bitset_add(&ctx_bitset, GDSCRIPT_LINE_END);
	id_bitset_add(&ctx_bitset, GDSCRIPT_SEMICOLON);
	id_bitset_union(&ctx_bitset, ctx->parent_tokens);
	subctx.parent_tokens = &ctx_bitset;
	for (; *cursor < ctx->num_tokens; ++*cursor)
	{
		const token_id_t prev_id = ctx->ids[*cursor - 1];
		const token_id_t id = ctx->ids[*cursor];
		const token_id_t var_or_const = ctx->ids[(*root)->next->index] == GDSCRIPT_STATIC ?
			ctx->ids[(*root)->next->index] : ctx->ids[(*root)->next->next->index];
		/* if (id == GDSCRIPT_LINE_END || id == GDSCRIPT_SEMICOLON) */
		if (id == GDSCRIPT_LINE_END)
		{
			struct parse_node* node = alloc_node(ctx->arena);
			node->index = *cursor;
			node->child = *root;
			*root = alloc_jct(ctx->arena);
			node_append(*root, node);
			++*cursor;
			return PARSED_END;
		}
		// gotta meme this outta parse_token to prevent parse_ternary
		else if ((prev_id == GDSCRIPT_LINE_END || prev_id == GDSCRIPT_SEMICOLON || prev_id == GDSCRIPT_NONE) && id == GDSCRIPT_IF)
		{
			struct parse_node* node = alloc_node(ctx->arena);
			node->index = *cursor;
			node->child = nil;
			node_append(current, node);
			current = node;
			last = node;
		}
		else if (id == GDSCRIPT_CONST || id == GDSCRIPT_VAR)
		{
			struct parse_node* node = alloc_node(ctx->arena);
			node->index = *cursor;
			node->child = nil;
			node_append(current, node);
			++*cursor;
			current = node;
			last = node;
			parse_decl(&current, &last, cursor, &subctx);
		}
		else if (id == GDSCRIPT_COLON)
		{
			struct parse_node* node = alloc_node(ctx->arena);
			node->index = *cursor;
			node->child = nil;
			node_append(current, node);
			current = node;
			last = node;
		}
		else if (id_bitset_has(ctx->parent_tokens, id))
		{
			--*cursor;
			return AT_PARENT_TOKEN;
		}
		else
		{
			ingest_token(id, &current, &last, cursor, &subctx);
		}
	}
	return PARSED_END;
}

static enum parse_res parse_decl(struct parse_node** current, struct parse_node** last, int32_t* cursor, struct context* ctx)
{
	for (; *cursor < ctx->num_tokens; ++*cursor)
	{
		const token_id_t id = ctx->ids[*cursor];
		if (!id_bitset_has(ctx->parent_tokens, id))
		{
			ingest_token(id, current, last, cursor, ctx);
		}
		else
		{
			--*cursor;
			return AT_PARENT_TOKEN;
		}
	}
	return PARSED_END;
}


// NOTE: cursor is at the OPENING PAIR!!! for loop STARTS by incrementing cursor!
// hear me out: just BALL
const uint8_t missing_delta = GDSCRIPT_MISSING_PAREN_CLOSE - GDSCRIPT_PAREN_CLOSE;
static enum parse_res parse_to_pair(const token_id_t delim, struct parse_node* root, int32_t* cursor, struct context* ctx)
{
	if (delim < GDSCRIPT_PAREN_CLOSE || GDSCRIPT_BRACKET_CLOSE < delim)
	{
		printf("ERROR: PARSING TO INVALID PAIR: %d\n", delim);
		fflush(stdout);
		assert(0);
	}
	struct parse_node* current = root;
	struct parse_node* last = root;
	struct context subctx = *ctx;
	subctx.lws = 0;
	struct id_bitset ctx_bitset = {0};
	id_bitset_add(&ctx_bitset, GDSCRIPT_COMMA);
	subctx.parent_tokens = &ctx_bitset;
	for (++*cursor ; *cursor < ctx->num_tokens; ++*cursor)
	{
		const token_id_t id = ctx->ids[*cursor];
		if (id == delim || id == delim + missing_delta)
		{
			/* printf("> FOUND CLOSING PAIR\n"); */
			struct parse_node* pair = alloc_node(ctx->arena);
			pair->index = *cursor;
			pair->child = nil;
			node_prepend(pair, root);
			return PARSED_END;
		}
		else if (id == GDSCRIPT_COLON && delim == GDSCRIPT_BRACE_CLOSE)
		{
			struct parse_node* node = alloc_node(ctx->arena);
			node->child = nil;
			node->index = *cursor;
			++*cursor;
			node_append(current, node);
			current = node;
		}
		else if (id == GDSCRIPT_ELLIPSES && delim == GDSCRIPT_PAREN_CLOSE)
		{
			const int32_t peek = *cursor + 1;
			struct parse_node* ellipses = alloc_node(ctx->arena);
			ellipses->child = nil;
			ellipses->index = *cursor;
			if (ctx->ids[peek] == GDSCRIPT_IDENTIFIER && ctx->offsets[peek] - 3 == ctx->offsets[*cursor])
			{
				struct parse_node* identifier = alloc_node(ctx->arena);
				identifier->index = peek;
				identifier->child = nil;
				struct parse_node* parent = alloc_node(ctx->arena);
				parent->index = PARSE_VARARG;
				parent->child = alloc_jct(ctx->arena);
				// ya im just gonna write it out because idk how well cat works sequentially
				parent->child->prev = identifier;
				parent->child->next = ellipses;
				ellipses->prev = parent->child;
				ellipses->next = identifier;
				identifier->prev = ellipses;
				identifier->next = parent->child;
				current = parent;
				last = identifier;
				++*cursor;
			}
			else
			{
				node_append(current, ellipses);
				current = ellipses;
				last = ellipses;
			}
		}
		else if (id == GDSCRIPT_LINE_END || id == GDSCRIPT_SEMICOLON)
		{
			continue;
		}
		else if (id_bitset_has(ctx->parent_tokens, id))
		{
			--*cursor;
			return AT_PARENT_TOKEN;
		}
		else
		{
			ingest_token(id, &current, &last, cursor, &subctx);
		}
	}
	return PARSED_END;
}


static struct lambda_res parse_lambda(struct parse_node* root, int32_t* cursor, struct context* ctx)
{
	const int16_t lws_count = ctx->lws;
	while (*cursor < ctx->num_tokens)
	{
		int16_t lws_actual = ctx->offsets[*cursor] - (ctx->offsets[*cursor - 1] + ctx->lens[*cursor - 1]);
		if (lws_actual == lws_count || ctx->ids[*cursor - 1] == GDSCRIPT_SEMICOLON)
		{
			if (parse_line(&root, cursor, ctx) == PARSED_END) /* lol lmao */;
			else return (struct lambda_res){ .lws = lws_actual, .parse_res = AT_PARENT_TOKEN };
		}
		else if (lws_actual > lws_count)
		{
			struct context subctx = *ctx;
			struct lambda_res res;
			do
			{
				subctx.lws = lws_actual;
				struct parse_node* scope = alloc_node(ctx->arena);
				scope->index = PARSE_SCOPE;
				node_prepend(scope, root);
				scope->child = alloc_jct(ctx->arena);
				res = parse_lambda(scope->child, cursor, &subctx);
				if(res.parse_res == PARSED_END) /* lol lmao */;
				else return res;
			} while(lws_actual > lws_count);
		}
		else
		{
			return (struct lambda_res){ .lws = lws_actual, .parse_res = PARSED_END };
		}
	}
	return (struct lambda_res){ .lws = lws_count, .parse_res = PARSED_END };
}

static enum parse_res parse_ternary_expr(struct parse_node* root, int32_t* cursor, struct context* ctx)
{
	enum __attribute__((packed))
	{
		PARSING_COND = 0x00,
		PARSING_ELSE,
	} state = PARSING_COND;
	struct parse_node* current = root;
	struct parse_node* last = root;
	/* printf(">> ternary:"); */
	for(++*cursor; *cursor < ctx->num_tokens; ++*cursor)
	{
		const token_id_t id = ctx->ids[*cursor];
		/* printf(" [%.*s]", ctx->lens[*cursor], ctx->src + ctx->offsets[*cursor]); */
		if (state == PARSING_COND)
		{
			if(!(id == GDSCRIPT_ELSE || id_bitset_has(ctx->parent_tokens, id)))
			{
				ingest_token(id, &current, &last, cursor, ctx);
			}
			else if (id == GDSCRIPT_ELSE)
			{
				struct parse_node* node = alloc_node(ctx->arena);
				node->index = *cursor;
				/* cat_nodes(current, last); */
				node_append(current, node);
				current = alloc_jct(ctx->arena);
				node->child = current;
				last = current;
				/* printf("found 'else': "); */
				/* node_sexp(root, ctx); */
				/* putc('\n', stdout); */
				state = PARSING_ELSE;
			}
			else
			{
				--*cursor;
				return AT_PARENT_TOKEN;
			}
		}
		else
		{
			if (!id_bitset_has(ctx->parent_tokens, id))
			{
				const int32_t size0 = ctx->arena->size;
				if (ingest_token(id, &current, &last, cursor, ctx) == NEW_CURRENT)
				{
					node_remove(current);
					ctx->arena->size = size0;
					--*cursor;
					return PARSED_END;
				}
			}
			else
			{
				--*cursor;
				return AT_PARENT_TOKEN;
			}
		}
	}
	return PARSED_END;
}

// starts with cursor RIGHT AT A PERIOD, & guaranteed root is an period
static void parse_period_expr(struct parse_node** root, int32_t* cursor, struct context* ctx)
{
	/* for (int32_t i = *cursor + 1; i < ctx->num_tokens; i += 2) */
	for (++*cursor; *cursor < ctx->num_tokens; *cursor += 2)
	{
		const token_id_t id = ctx->ids[*cursor];
		const token_id_t peek_id = ctx->ids[*cursor + 1];
		if (id == GDSCRIPT_IDENTIFIER && peek_id == GDSCRIPT_PERIOD)
		{
			struct parse_node* node = alloc_node(ctx->arena);
			node->index = *cursor;
			node->child = nil;
			node_prepend(node, (*root)->child);
			node = alloc_node(ctx->arena);
			node->index = *cursor + 1;
			node->child = alloc_jct(ctx->arena);
			node_prepend(node->child, *root);
			*root = node;
		}
		else if (id == GDSCRIPT_IDENTIFIER)
		{
			struct parse_node* node = alloc_node(ctx->arena);
			node->index = *cursor;
			node->child = nil;
			node_prepend(node, (*root)->child);
			return;
		}
		else
		{
			--*cursor;
			return;
		}
	}
}

static enum token_pos ingest_token(const token_id_t id, struct parse_node** current, struct parse_node** last, int32_t* cursor, struct context* ctx)
{
	struct parse_node* node = alloc_node(ctx->arena);
	node->index = *cursor;
	if (0)
	{
	switch (id)
	{
#define X(A,...) case GDSCRIPT_##A: printf(" - ingesting: %d: GDSCRIPT_" #A "\n", *cursor); break;
		GDSCRIPT_BUILTIN_TOKENS
	#undef X
	default:
	printf(" - ingesting: %d:%.*s\n", *cursor, ctx->lens[*cursor], ctx->src + ctx->offsets[*cursor]);
	}
	fflush(stdout);
	}
	const token_id_t current_id = ctx->ids[(*current)->index];
	const token_id_t last_id = ctx->ids[(*last)->index];
	enum token_pos res = last_id != GDSCRIPT_NONE ? CONT_CURRENT : AFTER_JCT;
	// BINARY OPERATORS (incl. colon & arrow)
	if (binary_operator_p(id))
	{
		node->child = alloc_jct(ctx->arena);
		const token_id_t current_id = ctx->ids[(*current)->index];
		if (binary_operator_p(current_id) ||  unary_operator_p(current_id) ||
			operator_arg_p(current_id))
		{
			struct parse_node temp =
			{
				.next = &temp,
				.prev = &temp,
				.child = nil
			};
			/* node_replace(node, *current); */
			node_replace(&temp, *current);
			(*current)->next = *current;
			(*current)->prev = *current;
			node_prepend(node->child, *current);
			/* node_sexp(node, ctx); */
			/* putc('\n', stdout); */
			node_replace(node, &temp);
		}
		else
		{
			node_append(*current, node);
			res |= NEW_CURRENT;
		}
		*current = node;
	}
	// UNARY OPERATORS
	else if (unary_operator_p(id))
	{
		// unary operators are cheeky shits tbqh
		node->child = alloc_jct(ctx->arena);
		// if last is binary or unary become the last node's last child
		if (binary_operator_p(last_id) || unary_operator_p(last_id))
		{
			node_prepend(node, (*last)->child);
		}
		// else follow current & become current
		else
		{
			node_append(*current, node);
			*current = node;
			res |= NEW_CURRENT;
		}
	}
	else if (id == GDSCRIPT_PERIOD)
	{
		if (last_id == GDSCRIPT_IDENTIFIER)
		{
			struct parse_node temp =
			{
				.next = &temp,
				.prev = &temp,
				.child = nil
			};
			node_replace(&temp, *last);
			node->child = alloc_jct(ctx->arena);
			(*last)->next = *last;
			(*last)->prev = *last;
			node_prepend(node->child, *last);
			parse_period_expr(&node, cursor, ctx);
			node_replace(node, &temp);
			*current = *last == *current ? node : *current;
		}
		else if (binary_operator_p(last_id) || unary_operator_p(last_id))
		{
			node->child = nil;
			node_prepend(node, (*last)->child);
		}
		else
		{
			node->child = nil;
			node_append(*current, node);
			/* cat_nodes(*current, node); */
			*current = node;
			res |= NEW_CURRENT;
		}
	}
	else if (identifier_or_literal_p(id))
	{
		node->child = nil;
		if (binary_operator_p(last_id) || unary_operator_p(last_id))
		{
			/* cat_nodes(node, (*last)->child); */
			node_prepend(node, (*last)->child);
		}
		else
		{
			/* cat_nodes(*current, node); */
			node_append(*current, node);
			*current = node;
			res |= NEW_CURRENT;
		}
	}
	else if (GDSCRIPT_PAREN_OPEN <= id && id <= GDSCRIPT_BRACKET_OPEN)
	{
		node->child = nil;
		struct parse_node* parent = alloc_node(ctx->arena);
		parent->child = alloc_jct(ctx->arena);
		parent->index = 0;
		/* printf("about to parse to pair: %d:'%.*s'\n", id, ctx->lens[*cursor], ctx->src + ctx->offsets[*cursor]); */
		parse_to_pair(id + 3, node, cursor, ctx);
		node_prepend(parent->child, node);
		/* node_sexp(parent->child, ctx); */
		/* putc('\n', stdout); */
		/* fflush(stdout); */
		/* printf("> LAST: %.*s\n", ctx->lens[(*last)->index], ctx->src + ctx->offsets[(*last)->index]); */
		/* printf("  >> identifier? %c\n", last_id == GDSCRIPT_IDENTIFIER || last_id == GDSCRIPT_PERIOD ? 'Y' : 'n'); */
		if (id == GDSCRIPT_PAREN_OPEN &&
			(last_id == GDSCRIPT_IDENTIFIER || last_id == GDSCRIPT_PERIOD || annotation_p(last_id)))
		{
			parent->index = last_id == GDSCRIPT_IDENTIFIER || last_id == GDSCRIPT_PERIOD ?
				PARSE_FUNC : PARSE_ANNOTATION_PARAMS;
			node_replace(parent, *last);
			node_append(parent->child, *last);
			if (*current == *last)
			{
				*current = parent;
				res |= NEW_CURRENT;
			}
		}
		else if (id == GDSCRIPT_BRACKET_OPEN && (last_id == GDSCRIPT_IDENTIFIER || last_id == GDSCRIPT_PERIOD))
		{
			parent->index = PARSE_INDEXED_ARRAY;
			node_replace(parent, *last);
			/* cat_nodes(parent->child, *last); */
			node_append(parent->child, *last);
			if (*current == *last)
			{
				*current = parent;
				res |= NEW_CURRENT;
			}
		}
		else
		{
			parent->index = id - (GDSCRIPT_PAREN_OPEN - PARSE_GROUP);
			if (binary_operator_p(last_id) || unary_operator_p(last_id))
			{
				node_prepend(parent, (*last)->child);
			}
			else
			{
				node_append(*current, parent);
				*current = parent;
				res |= NEW_CURRENT;
			}
		}
		/* node_sexp(parent, ctx); */
		node = parent;
		/* fflush(stdout); */
	}
	else if (id == GDSCRIPT_IF)
	{
		if (expression_p(current_id))
		{
			struct parse_node* tern = alloc_node(ctx->arena);
			tern->index = PARSE_TERNARY;
			tern->child = alloc_jct(ctx->arena);
			node_replace(tern, *current);
			(*current)->next = *current;
			(*current)->prev = *current;
			node->child = alloc_jct(ctx->arena);
			node_prepend(node->child, *current);
			parse_ternary_expr(node, cursor, ctx);
			/* node_sexp(node, ctx); */
			/* putc('\n', stdout); */
			tern->child = alloc_jct(ctx->arena);
			node_prepend(tern->child, node);
			*current = tern;
			node = tern;
		}
		else
		{
			node->child = nil;
			/* cat_nodes(*current, node); */
			node_append(*current, node);
			*current = node;
			res |= NEW_CURRENT;
		}
	}
	// https://docs.godotengine.org/en/stable/tutorials/scripting/gdscript/gdscript_basics.html#lambda-functions
	else if (id == GDSCRIPT_FUNC && ctx->ids[*cursor + 1] == GDSCRIPT_PAREN_OPEN)
	{
		node->child = nil;
		++*cursor;
		parse_lambda(node, cursor, ctx);
		struct parse_node* lambda = alloc_node(ctx->arena);
		lambda->index = PARSE_LAMBDA;
		lambda->child = alloc_jct(ctx->arena);
		node_prepend(lambda->child, node);
		*current = lambda;
		node = lambda;
		res |= NEW_CURRENT;
	}
	else if (id != GDSCRIPT_NONE)// catch all keywords, annotations, etc.
	{
		node->child = nil;
		/* cat_nodes(*current, node); */
		node_append(*current, node);
		*current = node;
		res |= NEW_CURRENT;
	}
	*last = node;
	return res;
}
