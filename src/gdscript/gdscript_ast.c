#include "gdscript_ast.h"
#include "gdscript_parser.h"
/* #undef ANNOTATIONS */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "pointer_memes.h"
#include "allocators/arena.h"
#include "free_list.h"
#include "token.h"
#include "scratch.h"

/* (1) fuck it.
 * (2) just parse it
 * (3) check afterwards B) */

#define RESIZE_SCALE 1.618
#define RESIZE_OVERHEAD 4096

// FOR CLARITY'S SAKE
typedef uint8_t bool;
#define true 1
#define false 0

static gdscript_id_t assignment_ids[NUM_ASSIGNMENT_OPERATORS] =
{
#define X(A) GDSCRIPT_##A,
	GDSCRIPT_ASSIGNMENT_OPERATORS
#undef X
};

typedef enum __attribute__((packed)) assignment_mask_t
{
	LINE_NONE = 0x00,
	LINE_ASSIGNMENT = 0x01,
	LINE_INFERRED_ASSIGNMENT = 0x02,
} assignment_mask_t;

typedef struct context_t
{
	token_t* tokens;
	gdscript_id_t* ids;
	free_list_t* fl;
	uint32_t num_tokens;
} context_t;

#define UNPACK_CONTEXT(CTX) \
	const gdscript_id_t* ids = CTX->ids; \
	const uint32_t num_tokens = CTX->num_tokens

#define KEYVALUES								\
	X(EXPORT_CATEGORY,				todo)		\
	X(EXPORT_COLOR_NO_ALPHA,		todo)		\
	X(EXPORT_CUSTOM,				todo)		\
	X(EXPORT_DIR,					todo)		\
	X(EXPORT_ENUM,					todo)		\
	X(EXPORT_EXP_EASING,			todo)		\
	X(EXPORT_FILE,					todo)		\
	X(EXPORT_FLAGS_2D_NAVIGATION,	todo)		\
	X(EXPORT_FLAGS_2D_PHYSICS,		todo)		\
	X(EXPORT_FLAGS_2D_RENDER,		todo)		\
	X(EXPORT_FLAGS_3D_NAVIGATION,	todo)		\
	X(EXPORT_FLAGS_3D_PHYSICS,		todo)		\
	X(EXPORT_FLAGS_3D_RENDER,		todo)		\
	X(EXPORT_FLAGS_AVOIDANCE,		todo)		\
	X(EXPORT_FLAGS,					todo)		\
	X(EXPORT_GLOBAL_DIR,			todo)		\
	X(EXPORT_GLOBAL_FILE,			todo)		\
	X(EXPORT_GROUP,					todo)		\
	X(EXPORT_MULTILINE,				todo)		\
	X(EXPORT_NODE_PATH,				todo)		\
	X(EXPORT_PLACEHOLDER,			todo)		\
	X(EXPORT_RANGE,					todo)		\
	X(EXPORT_STORAGE,				todo)		\
	X(EXPORT_SUBGROUP,				todo)		\
	X(EXPORT_TOOL_BUTTON,			todo)		\
	X(EXPORT,						todo)		\
	X(ICON,							todo)		\
	X(ONREADY,						todo)		\
	X(RPC,							todo)		\
	X(STATIC_UNLOAD,				todo)		\
	X(TOOL,							todo)		\
	X(WARNING_IGNORE_RESTORE,		todo)		\
	X(WARNING_IGNORE_STOP,			todo)		\
	X(WARNING_IGNORE,				todo)		\
	X(IF,							stmt_if)	\
	X(ELIF,							stmt_elif)	\
	X(ELSE,							stmt_else)	\
	X(FOR,							stmt_for)	\
	X(WHILE,						stmt_while)	\
	X(MATCH,						stmt_match)	\
	X(BREAK,						stmt_break)	\
	X(CONTINUE,						stmt_continue)	\
	X(PASS,							stmt_pass)	\
	X(RETURN,						stmt_return)	\
	X(CLASS_NAME,					decl_class)	\
	X(CLASS,						decl_subclass)	\
	X(EXTENDS,						decl_extends)	\
	X(SELF,							todo)		\
	X(SUPER,						todo)		\
	X(SIGNAL,						decl_signal)	\
	X(FUNC,							decl_func)	\
	X(STATIC,						decl_static)	\
	X(CONST,						decl_const)	\
	X(ENUM,							decl_enum)	\
	X(VAR,							decl_var)	\
	X(BREAKPOINT,					stmt_breakpoint)	\
	X(PRELOAD,						expr_preload)	\
	X(AWAIT,						expr_await)	\
	X(YIELD,						expr_yield)	\
	X(ASSERT,						expr_assert)	\
	X(GET,							stmt_get)	\
	X(SET,							stmt_set)	\
	X(VOID,							todo)

#define CONSTANTS_AND_LITERALS \
	X(NULL,							expr_literal) \
	X(TRUE,							expr_literal)	\
	X(FALSE,						expr_literal)	\
	X(PI,							expr_literal)	\
	X(TAU,							expr_literal)	\
	X(INF,							expr_literal)	\
	X(NAN,							expr_literal)	\
	X(STRING,						expr_string)	\
	X(NUMBER,						expr_number)	\
	X(IDENTIFIER,				    expr_identifier)


// TODO expand then delete the redundant forward declarations
#define X(_, FUNC) static bool parse_##FUNC(ast_node_t** parent, context_t* ctx, uint32_t* index);
KEYVALUES
#undef X


static void absolute_ast(ast_node_t* node);
static void process_tokens(ast_node_t** parent, context_t* ctx, uint32_t* index);
static bool parse_arguments(ast_node_t** parent, context_t* ctx, uint32_t* index);
static bool parse_parameters(ast_node_t** parent, context_t* ctx, uint32_t* index);


/***********/
/* KONBINI */
/***********/


#define BITFLAGS(DEST, BIT_ARRAY, N)			\
	for (uint32_t i = 0; i < N; ++i)			\
		DEST |= BIT_ARRAY[i] << i;

#define ERR_NODE											\
	ast_node_t* error_node = add_error(parent, ctx->fl);	\
	error_node->err_token = ctx->tokens + *index;			\
	error_node->err_len = 1;

// NOTE: assumes index is ONE AFTER a scope start.
static uint32_t pos_scope_end(const uint32_t index, const gdscript_id_t* ids, const uint32_t n)
{
	uint16_t depth = 0;
	for (uint32_t i = index; i < n; ++i)
	{
		switch(ids[i])
		{
		case GDSCRIPT_SCOPE_START:
			++depth;
			break;
		case GDSCRIPT_SCOPE_END:
			if (depth-- == 0) return i;
			break;
		}
	}
	return 0;
}

// NOTE: assumes index is ONE AFTER an open paren
static uint32_t pos_pair(const gdscript_id_t opener, const uint32_t index, const gdscript_id_t* ids, const uint32_t n)
{
	uint16_t depth = 0;
	for (uint32_t i = index; i < n; ++i)
	{
		switch(ids[i])
		{
		case 0: // bro this is disgusting
			if (depth == 0) return i;
			break;
		case GDSCRIPT_PAREN_CLOSE:
			depth--;
			break;
		case GDSCRIPT_PAREN_OPEN:
			depth++;
			break;
		case GDSCRIPT_SEMICOLON: __attribute__((fallthrough));
		case GDSCRIPT_LINE_END:
			return 0;
		}
	}
	return 0;
}

static uint32_t pos_colon(const uint32_t index, const gdscript_id_t* ids, const uint32_t n)
{
	uint16_t depth = 0;
	for (uint32_t i = index; i < n; ++i)
	{
		switch(ids[i]) // this is excessive even for you
		{
		case GDSCRIPT_COLON:
			if (depth == 0) return i;
			break;
		case GDSCRIPT_PAREN_CLOSE:
			depth--;
			break;
		case GDSCRIPT_PAREN_OPEN:
			depth++;
			break;
		case GDSCRIPT_SEMICOLON: __attribute__((fallthrough));
		case GDSCRIPT_LINE_END:
			return 0;
		}
	}
	return 0;
}

static const uint32_t pos_eol(const uint32_t index, const gdscript_id_t* ids, const uint32_t n)
{
	for (uint32_t i = index; i < n; ++i)
	{
		const gdscript_id_t id = ids[i];
		if (id == GDSCRIPT_LINE_END || id == GDSCRIPT_SEMICOLON)
		{
			return i;
		}
	}
	return 0;
}

static const uint32_t pos_token(const gdscript_id_t token_id, const uint32_t index, const gdscript_id_t* ids, const uint32_t n)
{
	for (uint32_t i = index; i < n; ++i)
	{
		if (ids[i] == token_id)
		{
			return i;
		}
	}
	return 0;
}

static uint32_t pos_assignment(const uint32_t index, const gdscript_id_t* ids, const uint32_t n)
{
	for (uint32_t i = index; i < n; ++i)
	{
		const gdscript_id_t id = ids[i];
		for (uint8_t j = 0; j < NUM_ASSIGNMENT_OPERATORS; ++j)
		{
			if (id == assignment_ids[j])
			{
				return i;
			}
		}
	}
	return 0;
}

static const uint8_t assignment_mask(const uint32_t index, const gdscript_id_t* ids, const uint32_t n)
{
	uint8_t mask = LINE_NONE;
	for (uint32_t i = index; i < n; ++i)
	{
		switch (ids[i])
		{
		case GDSCRIPT_SEMICOLON: __attribute__((fallthrough));
		case GDSCRIPT_LINE_END: return mask;
		case GDSCRIPT_OP_ASSIGNMENT:
			mask |= LINE_ASSIGNMENT;
			break;
		case GDSCRIPT_OP_ASSIGNMENT_INFERRED:
			mask |= LINE_INFERRED_ASSIGNMENT;
			break;
		}
	}
	return mask;
}

static inline void* ast_alloc(free_list_t* fl, const size_t size)
{
	void* res = free_list_calloc(fl, size);
	while (res == NULL)
	{
		free_list_grow(fl, RESIZE_SCALE * fl->cap + RESIZE_OVERHEAD);
		res = free_list_calloc(fl, size);
	}
	return res;
}

static ast_node_t* add_blank(ast_node_t** parent, free_list_t* fl)
{
	const size_t num = (*parent)->num_children + 1;
	point_relative_offset(parent, fl->allocation);
	ast_node_t* children = ast_alloc(fl, num * sizeof *children);
	point_absolute_offset(parent, fl->allocation);
	ast_node_t* p = *parent;
	if (num > 1)
	{
		point_absolute(&p->children);
		memcpy(children, p->children, (num - 1) * sizeof *p);
		free_list_free(fl, p->children);
	}
	ast_node_t* last_child = children + num - 1;
	memset(last_child, 0x00, sizeof *children);
	p->children = ptr_rel_offset(children, &p->children);
	p->num_children = num;
	last_child->parent = p;
	point_relative(&last_child->parent);
	return last_child;
}

static ast_node_t* add_child(ast_node_t** parent, free_list_t* fl, const ast_id_t id)
{
	ast_node_t* last_child = add_blank(parent, fl);
	last_child->id = id;
	/* (*parent)->last_child_id = id; */
	return last_child;
}

static ast_node_t* add_error(ast_node_t** parent, free_list_t* fl)
{
	return add_child(parent, fl, AST_ERROR);
}

static inline ast_node_t* last_child(const ast_node_t* node)
{
	const uint32_t n = node->num_children;
	return node->children + n - 1;
}

static const size_t num_children_of_type(const ast_node_t* parent, const ast_id_t id)
{
	size_t res = 0;
	for (uint32_t i = 0; i < parent->num_children; ++i)
	{
		res += parent->children[i].id == id;
	}
	return res;
}


/*********************/
/* PARSING -- BASICS */
/*********************/


static bool parse_error(ast_node_t** parent, context_t* ctx, const uint32_t index)
{
	ast_node_t* n = add_error(parent, ctx->fl);
	n->err_token = ctx->tokens + index;
	n->err_len = 1;
	return true;
}

static bool parse_identifier(ast_node_t** parent, context_t* ctx, uint32_t* index)
{
	const gdscript_id_t* ids = ctx->ids;
	const uint32_t i = *index;
	if (ids[i] == GDSCRIPT_IDENTIFIER)
	{
		ast_node_t* node = add_child(parent, ctx->fl, AST_IDENTIFIER);
		node->symbol = ctx->tokens + i;
		return true;
	}
	*index = i + 1;
	return false;
}


/**************************/
/* PARSING -- EXPRESSIONS */
/**************************/


static bool parse_expr_strict_unary(ast_node_t** parent, context_t* ctx, uint32_t* index, const bool errs);
static bool parse_expr_unary(ast_node_t** parent, context_t* ctx, uint32_t* index, const bool errs);
static bool parse_expr_binary(ast_node_t** parent, context_t* ctx, uint32_t* index, const bool errs);
static bool parse_expr_operation(ast_node_t** parent, context_t* ctx, uint32_t* index, const bool errs);
static bool parse_expr_func(ast_node_t** parent, context_t* ctx, uint32_t* index, const bool errs);
static bool parse_expr_group(ast_node_t** parent, context_t* ctx, uint32_t* index, const bool errs);
static bool parse_expr_array(ast_node_t** parent, context_t* ctx, uint32_t* index, const bool errs);
static bool parse_expr_dict(ast_node_t** parent, context_t* ctx, uint32_t* index, const bool errs);
static bool parse_expr_ternary(ast_node_t** parent, context_t* ctx, uint32_t* index, const bool errs);

// NOTE: these funcs are ONLY called if they are what they are
static bool parse_expr_identifier(ast_node_t** parent, context_t* ctx, uint32_t* index)
{
	ast_node_t* n = add_child(parent, ctx->fl, AST_EXPR_SYMBOLIC);
	n->symbolic_id = SYMBOL_IDENTIFIER;
	n->symbol = ctx->tokens + *index;
	return true;
}

static bool parse_expr_string(ast_node_t** parent, context_t* ctx, uint32_t* index)
{
	ast_node_t* n = add_child(parent, ctx->fl, AST_EXPR_SYMBOLIC);
	n->symbolic_id = SYMBOL_STRING;
	n->symbol = ctx->tokens + *index;
	return true;
}

static bool parse_expr_number(ast_node_t** parent, context_t* ctx, uint32_t* index)
{
	ast_node_t* n = add_child(parent, ctx->fl, AST_EXPR_SYMBOLIC);
	n->symbolic_id = SYMBOL_NUMBER;
	n->symbol = ctx->tokens + *index;
	return true;
}

static bool parse_expr_literal(ast_node_t** parent, context_t* ctx, uint32_t* index)
{
	ast_node_t* n = add_child(parent, ctx->fl, AST_EXPR_SYMBOLIC);
	n->symbolic_id = SYMBOL_LITERAL;
	n->symbol = ctx->tokens + *index;
	return true;
}

static bool parse_expr_symbolic(ast_node_t** parent, context_t* ctx, uint32_t* index, const bool errs)
{
	UNPACK_CONTEXT(ctx);
	const uint32_t i = *index;
	switch(ids[i])
	{
#define X(VAL, FUNC) case GDSCRIPT_##VAL: return parse_##FUNC(parent, ctx, index);
		CONSTANTS_AND_LITERALS
#undef X
	}
	if (errs)
	{
		parse_error(parent, ctx, i);
	}
	return false;
}

static bool parse_expr_declaration(ast_node_t** parent, context_t* ctx, uint32_t* index, const bool errs)
{
	/* UNPACK_CONTEXT(ctx); */
	/* const uint32_t i0 = *index; */
	/* const uint32_t eol = pos_eol(i0, ids, num_tokens); */
	/* // search for = or := */
	/* const uint8_t mask = assignment_mask(i0, ids, eol); */
	/* const uint32_t pivot = (mask & LINE_ASSIGNMENT) * pos_token(GDSCRIPT_OP_ASSIGNMENT, i0, ids, eol) || */
	/* 	((mask & LINE_INFERRED_ASSIGNMENT) >> 1) * pos_token(GDSCRIPT_OP_ASSIGNMENT_INFERRED, i0, ids, eol); */
	/* ast_node_t* node = NULL; */
	/* if (has_colon) */
	/* { */
	/* } */
	/* if (mask & LINE_ASSIGNMENT) */
	/* { */
	/* 	bool res = true; */
	/* 	ast_node_t* node = add_child(parent, ctx->fl, AST_EXPR_BINARY); */
	/* 	node->operation = OP_ASSIGNMENT; */
	/* 	const bool inferred = ids[pivot] == GDSCRIPT_OP_ASSIGNMENT_INFERRED; */
	/* 	node->operation |= inferred ? OP_TYPE : OP_NONE; */
	/* 	context_t subctx = *ctx; */
	/* 	subctx.num_tokens = pivot; */
	/* 	return res; */
	/* } */
	/* else if ((mask & LINE_INFERRED_ASSIGNMENT) && !has_colon) */
	/* { */
	/* } */
	/* else if (mask & LINE_INFERRED_ASSIGNMENT) // implicit has_colon is true */
	/* { */
	/* 	// error! */
	/* } */
	/* else if (errs) */
	/* { */
	/* 	// throw an error */
	/* } */
	/* return false; */
}

static bool parse_expr_assignment(ast_node_t** parent, context_t* ctx, uint32_t* index)
{
	UNPACK_CONTEXT(ctx);
	const uint32_t i0 = *index;
	const uint32_t eol = pos_eol(i0, ids, num_tokens);
	return false;
}

static bool parse_expression(ast_node_t** parent, context_t* ctx, uint32_t* index, const bool errs)
{
	UNPACK_CONTEXT(ctx);
	for (uint32_t i = *index; i < num_tokens; ++i)
	{
		switch (ids[i])
		{
		case GDSCRIPT_SEMICOLON: __attribute__((fallthrough));
		case GDSCRIPT_LINE_END:
			*index = i;
			return 1;
		case GDSCRIPT_IF:
			parse_expr_ternary(parent, ctx, index, errs);
			break;
		case GDSCRIPT_IDENTIFIER:
			parse_expr_identifier(parent, ctx, index);
			break;
		case GDSCRIPT_PAREN_OPEN:
			parse_expr_func(parent, ctx, index, false) ||
			parse_expr_group(parent, ctx, index, false) ||
			parse_error(parent, ctx, *index);
			break;
		case GDSCRIPT_BRACE_OPEN:
			parse_expr_dict(parent, ctx, index, errs);
			break;
		case GDSCRIPT_BRACKET_OPEN:
			parse_expr_array(parent, ctx, index, errs);
			break;
		#define X(_, VALUE) case GDSCRIPT_##VALUE: parse_expr_operation(parent, ctx, index, errs); break;
			 GDSCRIPT_OPERATORS
		#undef X
		default:
			parse_error(parent, ctx, *index);
		}
	}
	*index = num_tokens - 1;
	return true;
}


static bool parse_expr_rvalue(ast_node_t** parent, context_t* ctx, uint32_t* index, const bool errs)
{
	UNPACK_CONTEXT(ctx);
	for (uint32_t i = *index; i < num_tokens; ++i)
	{
		switch (ids[i])
		{
		case GDSCRIPT_SEMICOLON: __attribute__((fallthrough));
		case GDSCRIPT_LINE_END:
			*index = i;
			return 1;
		case GDSCRIPT_IF:
			parse_expr_ternary(parent, ctx, index, errs);
			break;
		case GDSCRIPT_IDENTIFIER:
			parse_expr_identifier(parent, ctx, index);
			break;
		case GDSCRIPT_PAREN_OPEN:
			parse_expr_func(parent, ctx, index, false) ||
			parse_expr_group(parent, ctx, index, false) ||
			parse_error(parent, ctx, i);
			break;
		case GDSCRIPT_BRACE_OPEN:
			parse_expr_dict(parent, ctx, index, errs);
			break;
		case GDSCRIPT_BRACKET_OPEN:
			parse_expr_array(parent, ctx, index, errs);
			break;
		#define X(_, VALUE) case GDSCRIPT_##VALUE: parse_expr_operation(parent, ctx, index, errs); break;
			GDSCRIPT_OPERATORS
		#undef X
		default:
			parse_error(parent, ctx, *index);
		}
	}
	*index = num_tokens - 1;
	return true;
}

static bool parse_expr_rvalue_to_colon(ast_node_t** parent, context_t* ctx, uint32_t* index, const bool errs)
{
	UNPACK_CONTEXT(ctx);
	const uint32_t i0 = *index;
	bool res = false;
	for (uint32_t i = i0; i < num_tokens; ++i)
	{
		switch (ids[i])
		{
		case GDSCRIPT_COLON:
			*index = i;
			return res;
		case GDSCRIPT_SEMICOLON: __attribute__((fallthrough));
		case GDSCRIPT_LINE_END:
			{
				ast_node_t* err_node = add_error(parent, ctx->fl);
				err_node->err_token = ctx->tokens + i0;
				err_node->err_len = i - i0 + 1;
			}
			return false;
		case GDSCRIPT_IF:
			res |= parse_expr_ternary(parent, ctx, index, errs);
			break;
		case GDSCRIPT_IDENTIFIER:
			res |= parse_expr_identifier(parent, ctx, index);
			break;
		case GDSCRIPT_PAREN_OPEN:
			res |= parse_expr_func(parent, ctx, index, 0) ||
			parse_expr_group(parent, ctx, index, 0) ||
			parse_error(parent, ctx, i);
			break;
		case GDSCRIPT_BRACE_OPEN:
			res |= parse_expr_dict(parent, ctx, index, errs);
			break;
		case GDSCRIPT_BRACKET_OPEN:
			res |= parse_expr_array(parent, ctx, index, errs);
			break;
		#define X(_, VALUE) case GDSCRIPT_##VALUE: res |= parse_expr_operation(parent, ctx, index, errs); break;
			GDSCRIPT_OPERATORS
		#undef X
		default:
			parse_error(parent, ctx, *index);
		}
	}
	*index = num_tokens - 1;
	return res;
}

static bool parse_expr_func(ast_node_t** parent, context_t* ctx, uint32_t* index, const bool errs)
{
	const uint32_t i = *index;
	const ast_node_t* p = *parent;
	bool func_flag = p->num_children > 0;
	ast_node_t* node = func_flag ? p->children + p->num_children - 1 : NULL;
	// check to see if preceeding child is an identifier or a dot expression
	func_flag &= node->id == AST_IDENTIFIER ||
		(node->id == AST_EXPR_SYMBOLIC && node->symbolic_id == SYMBOL_IDENTIFIER) ||
		(node->id == AST_EXPR_BINARY && node->operation == OP_DOT);
	if (func_flag)
	{
		ast_node_t cache = *node;
		if (cache.id == AST_EXPR_SYMBOLIC)
		{
			cache.id = AST_IDENTIFIER;
		}
		node->id = AST_EXPR_FUNC;
		node->num_children = 0;
		ast_node_t* temp = add_blank(&node, ctx->fl);
		memcpy(temp, &cache, sizeof cache);
		temp->parent = node;
		uint32_t j = i + 1;
		parse_arguments(&node, ctx, &j);
		*index = j;
		return true;
	}
	else if (errs)
	{
		// TODO: error node
	}
	return false;
}

static bool parse_expr_group(ast_node_t** parent, context_t* ctx, uint32_t* index, const bool errs)
{
	UNPACK_CONTEXT(ctx);
	const uint32_t i = *index;
	uint32_t pos;
	bool valid_group = ids[i] == GDSCRIPT_PAREN_OPEN;
	valid_group &= (pos = pos_pair(GDSCRIPT_PAREN_OPEN, i, ids, num_tokens)) > 0;
	if (valid_group)
	{
		ast_node_t* node = add_child(parent, ctx->fl, AST_EXPR_GROUP);
		uint32_t j = i + 1;
		context_t subctx = *ctx;
		subctx.num_tokens = pos;
		parse_expr_rvalue(&node, &subctx, &j, true);
		*index = j;
		return true;
	}
	else if (errs)
	{
		// TODO: error
	}
	return false;
}

static bool parse_expr_operation(ast_node_t** parent, context_t* ctx, uint32_t*index, const bool errs)
{
	gdscript_id_t* ids = ctx->ids;
	const uint32_t i = *index;
	const gdscript_id_t operation = ids[i];
	const gdscript_id_t id0 = i > 0 ? ids[i - 1] : GDSCRIPT_NONE;
	switch (operation) {
	case GDSCRIPT_OP_BITWISE_NOT: __attribute__((fallthrough));
	case GDSCRIPT_OP_BOOL_NOT:
		return parse_expr_unary(parent, ctx, index, errs);
	case GDSCRIPT_OP_IDENTITY: __attribute__((fallthrough));
	case GDSCRIPT_OP_SUM: __attribute__((fallthrough));
	case GDSCRIPT_OP_NEGATION: __attribute__((fallthrough));
	case GDSCRIPT_OP_DIFFERENCE:
		switch (i > 0 ? ids[i - 1] : 0) {
#define X(_, A) case GDSCRIPT_##A: __attribute__((fallthrough));
			GDSCRIPT_STRUCTURE
				GDSCRIPT_OPERATORS
#undef X
		case GDSCRIPT_IF: __attribute__((fallthrough));
		case GDSCRIPT_ELIF: __attribute__((fallthrough));
		case GDSCRIPT_WHILE: __attribute__((fallthrough));
		case GDSCRIPT_SCOPE_START: __attribute__((fallthrough));
		case GDSCRIPT_SCOPE_END: __attribute__((fallthrough));
		case GDSCRIPT_RETURN:
		{
			ids[i] = operation == GDSCRIPT_OP_SUM ? GDSCRIPT_OP_IDENTITY : operation;
			ids[i] = operation == GDSCRIPT_OP_DIFFERENCE ? GDSCRIPT_OP_NEGATION : operation;
		}
		return parse_expr_unary(parent, ctx, index, errs);
		default:
			ids[i] = operation == GDSCRIPT_OP_IDENTITY ? GDSCRIPT_OP_SUM : operation;
			ids[i] = operation == GDSCRIPT_OP_NEGATION ? GDSCRIPT_OP_DIFFERENCE : operation;
		}
		__attribute__((fallthrough));
	default:
		return parse_expr_binary(parent, ctx, index, errs);
	}
	return false;
}

static bool parse_expr_unary(ast_node_t** parent, context_t* ctx, uint32_t* index, const bool errs)
{
	UNPACK_CONTEXT(ctx);
	const uint32_t i = *index;
	ast_op_t op;
	switch(ids[i])
	{
		#define X(_, OPERATION) case GDSCRIPT_##OPERATION: op = OPERATION; break;
		GDSCRIPT_UNARY_OPERATORS
		#undef X
	default:
		op = OP_NONE;
	}
	if (op)
	{
		ast_node_t* unary_node = add_child(parent, ctx->fl, AST_EXPR_UNARY);
		unary_node->operation = op;
		*index = i + 1;
#define UNARY_RHS X(group) || X(func) || X(symbolic)
#define X(A) parse_expr_##A(&unary_node, ctx, index, errs)
		return UNARY_RHS;
#undef X
#undef UNARY_RHS
	}
	else if (errs)
	{
	/* TODO: error */
	}
	return 0;
}

static bool parse_expr_strict_unary(ast_node_t** parent, context_t* ctx, uint32_t* index, const bool errs)
{
	gdscript_id_t* ids = ctx->ids;
	const uint32_t num_tokens = ctx->num_tokens;
	const uint32_t i = *index;
#define AMBIGUOUS_OPS \
	X(OP_IDENTITY, OP_SUM) \
	X(OP_NEGATION, OP_DIFFERENCE)
	const gdscript_id_t operation = ids[i];
	/* Unary ops are NEVER proceeded by other operators */
	/* Unary ops are immeadiately preceeded by:
	   0. Nothing
	   1. other ops
	   2. commas
	   3. newlines/semicolons
	*/
	uint8_t ambivalent_mask = operation == GDSCRIPT_OP_SUM ||
		operation == GDSCRIPT_OP_IDENTITY ||
		operation == GDSCRIPT_OP_DIFFERENCE ||
		operation == GDSCRIPT_OP_NEGATION;
	// 2nd bit is 1 if operator should be binary, 0 if unary
	ambivalent_mask |= (i < num_tokens - 1 && (GDSCRIPT_OPERATORS_START < ids[i + 1] || ids[i + 1] < GDSCRIPT_OPERATORS_END)) << 1;
	ambivalent_mask |= (i > 0 && (ids [i - 1] != GDSCRIPT_COMMA &&
								  ids[i - 1] != GDSCRIPT_LINE_END &&
								  ids[i - 1] != GDSCRIPT_SEMICOLON &&
								  (ids[i - 1] < GDSCRIPT_OPERATORS_START || GDSCRIPT_OPERATORS_END < ids[i - 1]))) << 1;
	if (ambivalent_mask == 0x01)
	{
		switch (ids[i])
		{
			#define X(UNARY, BINARY) case GDSCRIPT_##BINARY: ids[i] = GDSCRIPT_##UNARY; break;
			AMBIGUOUS_OPS
			#undef X
		}
		return parse_expr_unary(parent, ctx, index, errs);
	}
	else if (ambivalent_mask == 0x03)
	{
		switch (ids[i])
		{
			#define X(UNARY, BINARY) case GDSCRIPT_##UNARY: ids[i] = GDSCRIPT_##BINARY; break;
			AMBIGUOUS_OPS
			#undef X
		}
	}
	return 0;
}

static bool parse_expr_binary(ast_node_t** parent, context_t* ctx, uint32_t* index, const bool errs)
{
	UNPACK_CONTEXT(ctx);
	const uint32_t i = *index;
#define BINARY_ASSIGNMENTS				\
		X(OP_ASSIGNMENT_POWER,			OP_POWER)			\
		X(OP_ASSIGNMENT_PRODUCT,		OP_PRODUCT)			\
		X(OP_ASSIGNMENT_SUM,			OP_SUM)				\
		X(OP_ASSIGNMENT_DIFFERENCE,		OP_DIFFERENCE)		\
		X(OP_ASSIGNMENT_DIVISION,		OP_DIVISION)		\
		X(OP_ASSIGNMENT_MODULO,			OP_MODULO)			\
		X(OP_ASSIGNMENT_BITWISE_AND,	OP_BITWISE_AND)		\
		X(OP_ASSIGNMENT_BITWISE_OR,		OP_BITWISE_OR)		\
		X(OP_ASSIGNMENT_BITSHIFT_LEFT,	OP_BITSHIFT_LEFT)	\
		X(OP_ASSIGNMENT_BITSHIFT_RIGH,	OP_BITSHIFT_RIGHT)	\
		X(OP_ASSIGNMENT_XOR,			OP_BITWISE_XOR)		\
		X(OP_ASSIGNMENT_TYPED,			OP_TYPE)
#define BINARY_OPERATIONS			\
		X(OP_POWER)					\
		X(OP_PRODUCT)				\
		X(OP_SUM)					\
		X(OP_DIFFERENCE)			\
		X(OP_DIVISION)				\
		X(OP_MODULO)				\
		X(OP_BOOL_AND)				\
		X(OP_BITWISE_AND)			\
		X(OP_BOOL_OR)				\
		X(OP_BITWISE_OR)			\
		X(OP_BITSHIFT_LEFT)			\
		X(OP_BITSHIFT_RIGHT)		\
		X(OP_BITWISE_XOR)			\
		X(OP_EQUALS)				\
		X(OP_INEQUALS)				\
		X(OP_LESS_THAN_EQUALS)		\
		X(OP_LESS_THAN)				\
		X(OP_GREATER_THAN_EQUALS)	\
		X(OP_GREATER_THAN)		\
		X(OP_DOT)
	ast_op_t op;
	switch(ids[i])
	{
	case GDSCRIPT_OP_ASSIGNMENT:
		op = OP_ASSIGNMENT;
		break;
	#define X(NAME, VALUE) case GDSCRIPT_##NAME: op = VALUE | OP_ASSIGNMENT; break;
	BINARY_ASSIGNMENTS
	#undef X
	#define X(A) case GDSCRIPT_##A: op = A; break;
	BINARY_OPERATIONS
	#undef X
	default:
		op = OP_NONE;
	}
	const ast_node_t* p = *parent;
	ast_node_t* last_child = p->children + p->num_children - 1;
	const bool flag = op > 0 && p->num_children > 0 && last_child->id & AST_EXPRESSION;
	if (flag)
	{
		ast_node_t* temp = alloca(sizeof *temp);
		memcpy(temp, last_child, sizeof *temp);
		last_child->id = AST_EXPR_BINARY;
		last_child->operation = op;
		last_child->num_children = 0;
		ast_node_t* lhs_node = add_blank(&last_child, ctx->fl);
		memcpy(lhs_node, temp, sizeof *temp);
		lhs_node->parent = last_child;
		*index += 1;
#define BINARY_RHS X(group) || X(dict) || X(array) || X(func) || X(strict_unary) || X(symbolic)
#define X(A) parse_expr_##A(&temp, ctx, index, errs)
		return BINARY_RHS;
#undef X
#undef BINARY_RHS
	}
	else if (errs)
	{
		// TODO: error
	}
	return false;
}


/*****************/
/* ARGS & PARAMS */
/*****************/


static bool parse_arguments(ast_node_t** parent, context_t* ctx, uint32_t* index)
{
	UNPACK_CONTEXT(ctx);;
	const uint32_t i0 = *index;
	bool res = true;
	for(uint32_t i = i0; i < num_tokens; ++i)
	{
		bool temp = parse_expr_rvalue(parent, ctx, &i, true);
		temp &= ids[i] == GDSCRIPT_COMMA ||
			(i == num_tokens && ids[i] == GDSCRIPT_PAREN_CLOSE);
		if (!temp)
		{
			ast_node_t* node = add_error(parent, ctx->fl);
			node->err_token = ctx->tokens + i;
			node->err_len = num_tokens - i;
			res = false;
			break;
		}
	}
	return res;
}

static bool parse_parameters(ast_node_t** parent, context_t* ctx, uint32_t* index)
{
	return false;
}


/*************************/
/* PARSING -- STATEMENTS */
/*************************/


static bool parse_stmt_if(ast_node_t** parent, context_t* ctx, uint32_t* index)
{
	UNPACK_CONTEXT(ctx);
	const uint32_t i0 = *index;
	uint32_t i = i0 + 1;
	ast_node_t* node = add_child(parent, ctx->fl, AST_STMT_IF);
	if (parse_expr_rvalue_to_colon(&node, ctx, &i, true))
	{
		++i; // go past the colon
		switch(i + 1 < num_tokens * ids[i + 1])
		{
		case 0: __attribute__((fallthrough));
		case GDSCRIPT_SEMICOLON:
			// is this allowed? I feel like it should NOT be.
			// TODO: test this.
			// I am ethicist & aesthete.
			goto err_line;
			break;
		case GDSCRIPT_LINE_END:
			if (i + 2 < num_tokens && ids[i + 2] == GDSCRIPT_SCOPE_START)
			{
				context_t subctx = *ctx;
				subctx.num_tokens = pos_scope_end(i + 3, ids, num_tokens);
				*index = i + 3;
				process_tokens(&node, &subctx, index);
				return true;
			}
			else
			{
				goto err_line;
			}
			break;
		default:
			*index = i + 1;
			return parse_expression(&node, ctx, index, false);
		}
	}
err_line:
	node->id = AST_ERROR;
	node->err_token = ctx->tokens + i0;
	const uint32_t eol = pos_eol(i0, ids, num_tokens);
	node->err_len = pos_eol(i0, ids, num_tokens) - i0;
	*index = eol;
	return false;
}

static bool parse_stmt_elif(ast_node_t** parent, context_t* ctx, uint32_t* index)
{
	UNPACK_CONTEXT(ctx);
	const uint32_t i0 = *index;
	uint32_t i = *index;
	ast_node_t* node = add_child(parent, ctx->fl, AST_STMT_ELIF);
	*index = i + 1;
	if (parse_expr_rvalue_to_colon(&node, ctx, index, 1))
	{
		++i; // go past the colon
		switch(i + 1 < num_tokens ? ids[i + 1] : 0)
		{
		case 0: __attribute__((fallthrough));
		case GDSCRIPT_SEMICOLON:
			// is this allowed? I feel like it should NOT be.
			// TODO: test this.
			// I am ethicist & aesthete.
			goto err_line;
			break;
		case GDSCRIPT_LINE_END:
			if (i + 2 < num_tokens && ids[i + 2] == GDSCRIPT_SCOPE_START)
			{
				context_t subctx = *ctx;
				subctx.num_tokens = pos_scope_end(i + 3, ids, num_tokens);
				*index = i + 3;
				process_tokens(&node, &subctx, index);
				return true;
			}
			else
			{
				goto err_line;
			}
			break;
		default:
			*index = i + 1;
			return parse_expression(&node, ctx, index, false);
		}
	}
err_line:
	node->id = AST_ERROR;
	node->err_token = ctx->tokens + i0;
	const uint32_t eol = pos_eol(i0, ids, num_tokens);
	node->err_len = pos_eol(i0, ids, num_tokens) - i0;
	*index = eol;
	return false;
}

static bool parse_stmt_else(ast_node_t** parent, context_t* ctx, uint32_t* index)
{

	UNPACK_CONTEXT(ctx);
	const uint32_t i = *index;
	ast_node_t* node = add_child(parent, ctx->fl, AST_STMT_ELSE);
	if (i + 1 < num_tokens && ids[i + 1] == GDSCRIPT_COLON)
	{
		switch(i + 2 < num_tokens ? ids[i + 2] : 0)
		{
		case 0: __attribute__((fallthrough));
		case GDSCRIPT_SEMICOLON:
			// is this allowed? I feel like it should NOT be.
			// TODO: test this.
			// I am ethicist & aesthete.
			goto err_line;
			break;
		case GDSCRIPT_LINE_END:
			if (i + 3 < num_tokens && ids[i + 3] == GDSCRIPT_SCOPE_START)
			{
				context_t subctx = *ctx;
				subctx.num_tokens = pos_scope_end(i + 4, ids, num_tokens);
				*index = i + 3;
				process_tokens(&node, &subctx, index);
				return true;
			}
			else
			{
				goto err_line;
			}
			break;
		default:
			*index = i + 2;
			return parse_expression(&node, ctx, index, 0);
		}
	}
err_line:
	node->id  = AST_ERROR;
	node->err_token = ctx->tokens + i;
	node->err_len = pos_eol(i, ids, num_tokens) - i;
	return false;
}

static bool parse_stmt_while(ast_node_t** parent, context_t* ctx, uint32_t* index)
{
	UNPACK_CONTEXT(ctx);
	const uint32_t i0 = *index;
	uint32_t i = i0;
	ast_node_t* node = add_child(parent, ctx->fl, AST_STMT_WHILE);
	if (parse_expr_rvalue_to_colon(&node, ctx, &i, true))
	{
		++i;
		switch(i + 1 < num_tokens ? ids[i + 1] : 0)
		{
		case 0: __attribute__((fallthrough));
		case GDSCRIPT_SEMICOLON:
			goto err_line;
		case GDSCRIPT_LINE_END:
			if (i + 2 < num_tokens && ids[i + 2] == GDSCRIPT_SCOPE_START)
			{
				context_t subctx = *ctx;
				subctx.num_tokens = pos_scope_end(i + 3, ids, num_tokens);
				*index = i + 3;
				process_tokens(&node, &subctx, index);
				return true;
			}
			else
			{
				goto err_line;
			}
			break;
		default: // TODO: compare to godot
			*index = i + 1;
			return parse_expression(&node, ctx, index, false);
		}
	}
err_line:
	node->id = AST_ERROR;
	node->err_token = ctx->tokens + i0;
	node->err_len = pos_eol(i0, ids, num_tokens) - i0;
	return false;
}

static bool parse_stmt_for(ast_node_t** parent, context_t* ctx, uint32_t* index)
{
	UNPACK_CONTEXT(ctx);
	return false;
}


static bool parse_stmt_return(ast_node_t** parent, context_t* ctx, uint32_t* index)
{
	UNPACK_CONTEXT(ctx);
	const uint32_t i = *index;
	ast_node_t* node = add_child(parent, ctx->fl, AST_STMT_RETURN);
	if (i + 1 < num_tokens && (ids[i + 1] != GDSCRIPT_LINE_END || ids[i + 1] != GDSCRIPT_SEMICOLON))
	{
		*index = i + 1;
		parse_expr_rvalue(&node, ctx, index, true);
	}
	return true;
}

static void error_til_line_end(ast_node_t** parent, context_t* ctx, uint32_t* index)
{
	UNPACK_CONTEXT(ctx);
	const uint32_t i0 = *index;
	const uint32_t i = pos_eol(i0, ids, num_tokens);
	if (i > i0)
	{
		ast_node_t* e = add_error(parent, ctx->fl);
		e->err_token = ctx->tokens + i0;
		e->err_len = i - i0;
		*index = i;
	}
}

static bool parse_stmt_break(ast_node_t** parent, context_t* ctx, uint32_t* index)
{
	ast_node_t* node = add_child(parent, ctx->fl, AST_STMT_BREAK);
	*index += 1;
	error_til_line_end(&node, ctx, index);
	return true;
}

static bool parse_stmt_continue(ast_node_t** parent, context_t* ctx, uint32_t* index)
{
	ast_node_t* node = add_child(parent, ctx->fl, AST_STMT_CONTINUE);
	*index += 1;
	error_til_line_end(&node, ctx, index);
	return true;
}

static bool parse_stmt_pass(ast_node_t** parent, context_t* ctx, uint32_t* index)
{
	ast_node_t* node = add_child(parent, ctx->fl, AST_STMT_PASS);
	*index += 1;
	error_til_line_end(&node, ctx, index);
	return true;
}


/***************************/
/* PARSING -- DECLARATIONS */
/***************************/

static bool variable_declaration(ast_node_t** parent, context_t* ctx, uint32_t* index, const bool var_flag)
{
	UNPACK_CONTEXT(ctx);
	const uint32_t i0 = *index;
	ast_node_t* node = add_child(parent, ctx->fl, var_flag? AST_DECL_VAR : AST_DECL_CONST);
	uint8_t flags = i0 + 1 < num_tokens;
	flags &= ids[i0 + 1] == GDSCRIPT_IDENTIFIER;
	/* flags &= parse_expression(&node, ctx, index, true); */
	if (flags)
	{
		node->symbol = ctx->tokens + i0 + 1;
		switch (i0 + 2 < num_tokens ? ids[i0 + 2] : 0)
		{
		case 0: __attribute__((fallthrough));
		case GDSCRIPT_SEMICOLON: __attribute__((fallthrough));
		case GDSCRIPT_LINE_END:
			return true;
		case GDSCRIPT_COLON:
			return true;
		case GDSCRIPT_OP_ASSIGNMENT:
			return true;
		case GDSCRIPT_OP_ASSIGNMENT_INFERRED:
			return true;
		default:
			goto line_err;
		}
	}
line_err:
		node->id = AST_ERROR;
		node->err_token = ctx->tokens + i0;
		node->err_len = pos_eol(i0, ids, num_tokens);
		return false;
}

static bool parse_decl_var(ast_node_t** parent, context_t* ctx, uint32_t* index)
{
	return variable_declaration(parent, ctx, index, true);
}

static bool parse_decl_const(ast_node_t** parent, context_t* ctx, uint32_t* index)
{
	return variable_declaration(parent , ctx, index, false);
}

static bool parse_decl_class(ast_node_t** parent, context_t* ctx, uint32_t* index)
{
	UNPACK_CONTEXT(ctx);
	const uint32_t i = *index;
	ast_node_t* node = add_child(parent, ctx->fl, AST_DECL_CLASS);
	*index = i + 1;
	if (parse_identifier(&node, ctx, index)) return true;
	node->id = AST_ERROR;
	node->err_token = ctx->tokens + i;
	*index = pos_eol(i, ids, num_tokens);
	node->err_len = *index - i;
	return false;
}


/**************************/
/* PARSING -- ANNOTATIONS */
/**************************/


/********************/
/* TOKEN PROCESSING */
/********************/


static void process_tokens(ast_node_t** parent, context_t* ctx, uint32_t* index)
{
	UNPACK_CONTEXT(ctx);
	for (uint32_t i = *index; i < num_tokens; ++i)
	{
		const gdscript_id_t id = ids[i];
		switch (id)
		{
		case GDSCRIPT_LINE_END: break;
		case GDSCRIPT_SEMICOLON: break;
		#define X(ENUM, FUNC) case GDSCRIPT_##ENUM: parse_##FUNC(&parent, ctx, index); break;
			KEYVALUES
		#undef X
		case GDSCRIPT_SCOPE_START:
			{
				context_t subctx = *ctx;
				// find scope end
				ast_node_t* node = add_child(parent, ctx, AST_STMT_SCOPE);
				subctx.num_tokens = pos_scope_end(i, ids, num_tokens);
				node->scope_len = subctx.num_tokens;
				if (subctx.num_tokens > 0)
				{
					process_tokens(&node, &subctx, &i);
				}
				else
				{
					// TODO: big brain error handling
				}
			}
			break;
		default:
			{
				context_t subctx = *ctx;
				subctx.num_tokens = pos_eol(i + 1, ids, num_tokens);
				parse_expression(parent, &subctx, index, true);
			}
		}
	}
}

ast_node_t* gdscript_ast_generate(arena_t* arena, const token_t* tokens, gdscript_id_t* ids, const uint32_t num_tokens)
{
	free_list_t fl = { .allocation = arena->allocation, .cap = arena->cap };
	free_list_clear(&fl);
	ast_node_t* root = free_list_calloc(&fl, sizeof *root);
	ast_node_t* rel_root = ptr_rel_offset(root, fl.allocation);
	root->id = AST_ROOT;
	/* root->len = num_tokens; */
	context_t ctx =
	{
		.tokens = tokens,
		.ids = ids,
		.num_tokens = num_tokens,
		.fl= &fl,
	};
	uint32_t index = 0;
	process_tokens(&root, &ctx, &index);
	if (index < num_tokens)
	{
		ast_node_t* err = add_error(&root, &fl);
		err->err_token = tokens + index;
		err->err_len = num_tokens - index;
	}
	// make children into absolute pointers
	root = ptr_abs_offset(rel_root, fl.allocation);
	absolute_ast(root);
	// update the arena
	arena->allocation = fl.allocation;
	arena->cap = fl.cap;
	return root;
}

static void absolute_ast(ast_node_t* node)
{
	point_absolute(&node->children);
	for (uint32_t i = 0; i < node->num_children; ++i)
	{
		point_absolute(&node->children[i].parent);
		absolute_ast(node->children + i);
	}
}


/******************/
/* POST-GEN CHECK */
/******************/


static void check_ast(ast_node_t* node, ast_node_t* parent, uint8_t* flags, const uint32_t index, const uint32_t num_siblings);

typedef enum __attribute__((packed)) ast_flags_t
{
	AST_FLAGS_NONE			   = 0x00,
	AST_FLAG_HAS_ICON		   = 0x01,
	AST_FLAG_HAS_TOOL		   = 0x02,
	AST_FLAG_HAS_STATIC_UNLOAD = 0x04,
	AST_FLAG_HAS_CLASS         = 0x08,
	AST_FLAG_HAS_EXTENDS       = 0x10
} ast_flags_t;

void gdscript_ast_check(ast_node_t* root)
{
	uint8_t flags = 0x00;
	const uint32_t n = root->num_children;
	for (uint32_t i = 0; i < n; ++i)
	{
		check_ast(root->children + i, root, &flags, i, n);
	}
}

// note pass in
static void check_ast(ast_node_t* node, ast_node_t* parent, uint8_t* flags, const uint32_t index, const uint32_t num_siblings)
{
	switch ((uint16_t)node->id)
	{
	case AST_ANNO_CLASS:
		// make sure parent is root & not preceeded by anything but themselves
		break;
	case AST_ANNO_VAR:
		// make sure parent is root & next not-anno node is a decl_var or decl_const
		break;
	case AST_ANNO_FUNC:
		// make sure parent is root & next not-anno node is a decl_func
		break;
	case AST_ANNO_WARN:
		// make sure parent is statement body
		break;
	case AST_EXPRESSION ... AST_EXPRESSION_END:
		// make sure parent is NOT root
		break;
	case AST_DECL_CLASS:
		// make sure parent IS root
		break;
	case AST_DECL_EXTENDS:
		// make sure parent is root
		break;
	// check the location of other things
	}
	const uint32_t n = node->num_children;
	for (int32_t i = 0; i < n; ++i)
	{
		check_ast(node->children + i, node, flags, i, n);
	}
}
