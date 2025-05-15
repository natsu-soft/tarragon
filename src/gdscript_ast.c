#include "gdscript_ast.h"
#include <stdint.h>
#undef ANNOTATIONS
#include <stdlib.h>
#include <string.h>
#include "gdscript_parser.h"
#include "pointer_memes.h"
#include "free_list.h"
#include "token.h"
#include "scratch.h"
#include "arena.h"

/* (1) fuck it
 * (2) just parse it then
 * (3) check afterwards */

#define RESIZE_SCALE 1.618
#define RESIZE_OVERHEAD 4096

// FOR CLARITY'S SAKE
typedef unsigned char bool_t;

typedef struct context_t
{
	token_t* tokens;
	gdscript_id_t* ids;
	free_list_t* fl;
	ast_node_t* root;
	uint32_t n;
	uint32_t req_lws;
} context_t;

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
	X(VOID,							expr_literal)	\
	X(NULL,							expr_literal)	\
	X(TRUE,							expr_literal)	\
	X(FALSE,						expr_literal)	\
	X(PI,							expr_literal)	\
	X(TAU,							expr_literal)	\
	X(INF,							expr_literal)	\
	X(NAN,							expr_literal)	\
	X(STRING,						expr_string)	\
	X(NUMBER,						expr_number)	\
	X(IDENTIFIER,				    identifier)


// TODO expand then delete the redundant forward declarations
#define X(_, FUNC) static bool_t parse_##FUNC(ast_node_t* parent, context_t* ctx, uint32_t* index);
KEYVALUES
#undef X


static bool_t process_tokens(ast_node_t* parent, context_t* ctx, uint32_t* index);
static bool_t process_line(ast_node_t* parent, context_t* ctx, uint32_t* index);
static bool_t parse_arguments(ast_node_t* parent, context_t* ctx, uint32_t* index, const bool_t errs);


/*****************************/
/* CONVENIENT MACROS & FUNCS */
/*****************************/


#define BITFLAGS(DEST, BIT_ARRAY, N) \
	for (uint32_t i = 0; i < N; ++i) \
		DEST |= BIT_ARRAY[i] << i;

#define ERR_NODE									\
	ast_node_t* error_node = add_error(&parent, ctx);	\
	error_node->err_token = ctx->tokens + *index; \
	error_node->err_len = 1;

static bool_t match(bool_t* matches, const gdscript_id_t* seq, const uint32_t len, const gdscript_id_t* ground_truth, const uint32_t n)
{
	bool_t res = 1;
	for (uint32_t i = 0; i < len && i < n; ++i)
	{
		const bool_t temp = seq[i] == ground_truth[i];
		matches[i] = temp;
		res &= temp;
	}
	for (uint32_t i = n; i < len; ++i)
	{
		matches[i] = 0;
		res = 0;
	}
	return res;
}

static uint32_t dist_pair(const gdscript_id_t opener, const uint32_t index, const gdscript_id_t* ids, const uint32_t n)
{
	uint16_t depth = 0;
	for (uint32_t i = index; i < n; ++i)
	{
		const gdscript_id_t id = ids[i];
		switch(id * (depth > 0 || id != opener)) // this is excessive even for you
		{
		case 0: // bro this is disgusting
			return i - index;
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

static void relative_children(ast_node_t* node)
{
	if (node->num_children > 0)
	{
		for (int i = 0; i < node->num_children; ++i)
		{
			relative_children(node->children + i);
		}
		point_relative(&node->children);
	}
}

static void absolute_children(ast_node_t* node)
{
	if (node->num_children > 0)
	{
		point_absolute(&node->children);
		for (int i = 0; i < node->num_children; ++i)
		{
			absolute_children(node->children + i);
		}
	}
}

static void* ast_alloc(free_list_t* fl, ast_node_t** root, const size_t size)
{
	void* res = free_list_calloc(fl, size);
	if (res == NULL)
	{
		relative_children(*root);
		point_relative_offset(root, fl->memory);
		do {
			free_list_grow(fl, RESIZE_SCALE * fl->size + RESIZE_OVERHEAD);
			res = free_list_calloc(fl, size);
		} while (res == NULL);
		point_absolute_offset(root, fl->memory);
		absolute_children(*root);
	}
	return res;
}

static ast_node_t* add_blank(ast_node_t** parent, context_t* ctx)
{
	const size_t num = (*parent)->num_children + 1;
	point_relative_offset(parent, ctx->fl->memory);
	ast_node_t* children = ast_alloc(ctx->fl, &ctx->root, num * sizeof **parent);
	point_absolute_offset(parent, ctx->fl->memory);
	ast_node_t* p = *parent;
	if (num > 1)
	{
		memcpy(children, p->children, (num - 1) * sizeof *p);
		free_list_free(ctx->fl, p->children);
	}
	memset(children + num - 1, 0x00, sizeof *p);
	p->children = children;
	p->num_children = num;
	ast_node_t* last_child = children + num - 1;
	last_child->parent = p;
	return last_child;
}

static ast_node_t* add_child(ast_node_t** parent, context_t* ctx, const ast_id_t id)
{
	ast_node_t* last_child = add_blank(parent, ctx);
	last_child->id = id;
	(*parent)->last_child_id = id;
	return last_child;
}

static ast_node_t* add_error(ast_node_t** parent, context_t* ctx)
{
	return add_child(parent, ctx, AST_ERROR);
}

static ast_node_t* last_child(const ast_node_t* node)
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


static bool_t parse_error(ast_node_t* parent, context_t* ctx, uint32_t* index)
{
	ERR_NODE
	return 1;
}

static bool_t parse_symbol(ast_node_t* parent, context_t* ctx, uint32_t* index)
{
	const uint32_t i = *index;
	ast_node_t* n = add_child(&parent, ctx, AST_IDENTIFIER);
	n->token_id = ctx->ids[i];
	return 1;
}

static bool_t parse_identifier(ast_node_t* parent, context_t* ctx, uint32_t* index)
{
	ast_node_t* n = add_child(&parent, ctx, AST_IDENTIFIER);
	n->symbol = ctx->tokens + *index;
	return 1;
}

static bool_t parse_string(ast_node_t* parent, context_t* ctx, uint32_t* index)
{
	ast_node_t* n = add_child(&parent, ctx, AST_EXPR_SYMBOLIC);
	n->symbolic_id = SYMBOL_STRING;
	return 1;
}

static bool_t parse_number(ast_node_t* parent, context_t* ctx, uint32_t* index)
{
	ast_node_t* n = add_child(&parent, ctx, AST_EXPR_SYMBOLIC);
	n->symbolic_id = SYMBOL_NUMBER;
	return 1;
}

static bool_t parse_literal(ast_node_t* parent, context_t* ctx, uint32_t* index)
{

	const uint32_t i = *index;
	ast_symbol_t symbol;
	switch (ctx->ids[i])
	{
	#define X(_, VALUE) case GDSCRIPT_##VALUE:
	GDSCRIPT_LITERALS
		symbol = SYMBOL_LITERAL;
		break;
	#undef X
	default:
		symbol = SYMBOL_NONE;
	}
	if (symbol)
	{
		ast_node_t* node = add_child(&parent, ctx, AST_EXPR_SYMBOLIC);
		node->symbolic_id = symbol;
		return 1;
	}
	else
	{
		ERR_NODE
		return 0;
	}
}


/**************************/
/* PARSING -- EXPRESSIONS */
/**************************/

static bool_t parse_expr_symbolic(ast_node_t* parent, context_t* ctx, uint32_t* index, const bool_t errs);
static bool_t parse_expr_literal(ast_node_t* parent, context_t* ctx, uint32_t* index);
static bool_t parse_expr_string(ast_node_t* parent, context_t* ctx, uint32_t* index);
static bool_t parse_expr_number(ast_node_t* parent, context_t* ctx, uint32_t* index);
static bool_t parse_expr_unary(ast_node_t* parent, context_t* ctx, uint32_t* index, const bool_t errs);
static bool_t parse_expr_binary(ast_node_t* parent, context_t* ctx, uint32_t* index, const bool_t errs);
static bool_t parse_expr_operation(ast_node_t* parent, context_t* ctx, uint32_t* index, const bool_t errs);
static bool_t parse_expr_func(ast_node_t* parent, context_t* ctx, uint32_t* index, const bool_t errs);
static bool_t parse_expr_array(ast_node_t* parent, context_t* ctx, uint32_t* index, const bool_t errs);
static bool_t parse_expr_dict(ast_node_t* parent, context_t* ctx, uint32_t* index, const bool_t errs);
static bool_t parse_expr_group(ast_node_t* parent, context_t* ctx, uint32_t* index, const bool_t errs);
static bool_t parse_expr_ternary(ast_node_t* parent, context_t* ctx, uint32_t* index, const bool_t errs);

static bool_t parse_expression(ast_node_t* parent, context_t* ctx, uint32_t* index, const bool_t errs)
{
	const gdscript_id_t* ids = ctx->ids;
	for (uint32_t i = *index; i < ctx->n; ++i)
	{
		switch (ids[i])
		{
		case GDSCRIPT_IF:
			parse_expr_ternary(parent, ctx, index, errs);
			break;
		case GDSCRIPT_IDENTIFIER:
			parse_expr_symbolic(parent, ctx, index, errs);
			break;
		case GDSCRIPT_PAREN_OPEN:
			parse_expr_func(parent, ctx, index, errs) || parse_expr_group(parent, ctx, index, errs);
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
			if (errs)
			{
				ERR_NODE
			}
			return 0;
		}
	}
	return 1;
}

static bool_t parse_expr_group(ast_node_t* parent, context_t* ctx, uint32_t* index, const bool_t errs)
{
	const uint32_t i = *index;
	gdscript_id_t* ids = ctx->ids;
	const uint32_t n = ctx->n;
	uint32_t count;
	bool_t flag = ids[i] == GDSCRIPT_PAREN_OPEN;
	flag &= (count = dist_pair(GDSCRIPT_PAREN_OPEN, i, ids, n)) > 0;
	if (flag)
	{
		ast_node_t* node = add_child(&parent, ctx, AST_EXPR_GROUP);
		/* node->pos = i; */
		uint32_t j = i + 1;
		context_t sub_ctx = *ctx;
		sub_ctx.n = i + count;
		parse_expression(node, &sub_ctx, &j, 1);
		*index = j;
		return 1;
	}
	else if (errs)
	{
		ERR_NODE
	}
	return 0;
}

static bool_t parse_expr_symbolic(ast_node_t* parent, context_t* ctx, uint32_t* index, const bool_t errs)
{
	const uint32_t i = *index;
	ast_symbol_t symbol;
	switch (ctx->ids[i])
	{
	case GDSCRIPT_IDENTIFIER:
		symbol = SYMBOL_IDENTIFIER;
		break;
	case GDSCRIPT_NUMBER:
		symbol = SYMBOL_NUMBER;
		break;
	case GDSCRIPT_STRING:
		symbol = SYMBOL_STRING;
		break;
	#define X(_, VALUE) case GDSCRIPT_##VALUE:
	GDSCRIPT_LITERALS
		symbol = SYMBOL_LITERAL;
		break;
	#undef X
	default:
		symbol = SYMBOL_NONE;
	}
	if (symbol)
	{
		ast_node_t* node = add_child(&parent, ctx, AST_EXPR_SYMBOLIC);
		node->symbolic_id = symbol;
		return 1;
	}
	else if (errs)
	{
		ERR_NODE
	}
	return 0;
}

static bool_t parse_expr_operation(ast_node_t* parent, context_t* ctx, uint32_t* index, const bool_t errs)
{
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
		X(OP_GREATER_THAN) \
		X(OP_DOT) \

	const uint32_t i = *index;
	const uint32_t n = ctx->n;
	gdscript_id_t* ids = ctx->ids;
	ast_op_t op;
	switch(ids[i])
	{
	case GDSCRIPT_COLON:
		if (parent->id == AST_EXPR_DICT && !parent->lua_dict)
		{
			op = OP_PAIR;
			ids[i] = GDSCRIPT_OP_PAIR;
		}
		else
		{
			op = OP_TYPE;
			ids[i] = GDSCRIPT_OP_TYPE;
		}
		break;
	case GDSCRIPT_OP_ASSIGNMENT:
		if (parent->id == AST_EXPR_DICT && parent->lua_dict)
		{
			op = OP_PAIR;
			ids[i] = GDSCRIPT_OP_PAIR;
		}
		else
		{
			op = OP_ASSIGNMENT;
		}
		break;
	#define X(NAME, VALUE) case GDSCRIPT_##NAME: op = VALUE | OP_ASSIGNMENT; break;
	BINARY_ASSIGNMENTS
	#undef X
	#define X(A) case GDSCRIPT_##A: op = A; break;
	BINARY_OPERATIONS
	#undef X
	default:
		return 0;
	}
	// NOTE: this IS wrong >:^(
	bool_t binop_flag = parent->num_children > 0;
	binop_flag &= parent->last_child_id & AST_EXPRESSION && // NOTE: bitwise AND-assignment
		(parent->last_child_id == AST_EXPR_SYMBOLIC ||
		 parent->last_child_id == AST_EXPR_UNARY ||
		 parent->last_child_id == AST_EXPR_GROUP ||
		 parent->last_child_id == AST_EXPR_FUNC);
	uint32_t j = i + 1;
	bool_t res;
	if (binop_flag)
	{
		switch (ids[i])
		{
		case GDSCRIPT_OP_IDENTITY:
			ids[i] = GDSCRIPT_OP_SUM;
			op = OP_SUM;
			break;
		case GDSCRIPT_OP_NEGATION:
			ids[i] = GDSCRIPT_OP_DIFFERENCE;
			op = OP_DIFFERENCE;
			break;
		}
		ast_node_t* last_child = parent->children + parent->num_children - 1;
		ast_node_t* temp = alloca(sizeof *temp);
		memcpy(temp, last_child, sizeof *temp);
		last_child->id = AST_EXPR_BINARY;
		last_child->operation = op;
		ast_node_t* lhs = add_child(&last_child, ctx, temp->id);
		last_child->lhs = lhs; // redundant -- look at the structs
		res = parse_expression(last_child, ctx, &j, 1);
		last_child->rhs = last_child->children + 1;
	}
	else
	{
		ast_node_t* unary_node = add_child(&parent, ctx, AST_EXPR_UNARY);
		unary_node->operation = op;
	#define UNARY_RHS \
		X(group) || \
		X(func) || \
		X(symbolic)
	#define X(A) parse_expr_##A(unary_node, ctx, &j, 0)
		res = UNARY_RHS;
		#undef X
	}
	*index = j;
	return res;
}

static bool_t parse_expr_unary(ast_node_t* parent, context_t* ctx, uint32_t* index, const bool_t errs)
{
	const uint32_t i = *index;
	const uint32_t n = ctx->n;
	gdscript_id_t* ids = ctx->ids;
	ast_op_t op;
	switch(ids[i])
	{
	case GDSCRIPT_OP_BITWISE_NOT:
		op = OP_BITWISE_NOT;
		break;
	case GDSCRIPT_OP_BOOL_NOT:
		op = OP_BOOL_NOT;
		break;
	case GDSCRIPT_OP_IDENTITY:
		op = OP_IDENTITY;
		break;
	case GDSCRIPT_OP_NEGATION:
		op = OP_NEGATION;
		break;
	default:
		op = OP_NONE;
	}
	if (op)
	{
		ast_node_t* unary_node = add_child(&parent, ctx, AST_EXPR_UNARY);
		unary_node->operation = op;
		uint32_t j = i + 1;
	#define UNARY_RHS \
		X(group) || \
		X(func) || \
		X(symbolic)
	#define X(A) parse_expr_##A(unary_node, ctx, &j, 0)
		return UNARY_RHS;
		#undef X
	}
	else if (errs)
	{
		ERR_NODE
	}
	return 0;
}

static bool_t parse_expr_binary(ast_node_t* parent, context_t* ctx, uint32_t* index, const bool_t errs)
{
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
		X(OP_GREATER_THAN) \
		X(OP_DOT) \

	const uint32_t i = *index;
	const uint32_t n = ctx->n;
	gdscript_id_t* ids = ctx->ids;
	ast_op_t op;
	switch(ids[i])
	{
	case GDSCRIPT_COLON:
		if (parent->id == AST_EXPR_DICT && !parent->lua_dict)
		{
			op = OP_PAIR;
			ids[i] = GDSCRIPT_OP_PAIR;
		}
		else
		{
			op = OP_TYPE;
			ids[i] = GDSCRIPT_OP_TYPE;
		}
		break;
	case GDSCRIPT_OP_ASSIGNMENT:
		if (parent->id == AST_EXPR_DICT && parent->lua_dict)
		{
			op = OP_PAIR;
			ids[i] = GDSCRIPT_OP_PAIR;
		}
		else
		{
			op = OP_ASSIGNMENT;
		}
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
	bool_t flag = op > 0;
	flag &= parent->num_children > 0 && // NOTE: bitwise AND-assignment
		parent->last_child_id & AST_EXPRESSION &&
		(parent->last_child_id == AST_EXPR_SYMBOLIC ||
		 parent->last_child_id == AST_EXPR_UNARY ||
		 parent->last_child_id == AST_EXPR_GROUP ||
		 parent->last_child_id == AST_EXPR_FUNC);
	if (flag)
	{
		ast_node_t* temp = alloca(sizeof *temp);
		ast_node_t* node = parent->children + parent->num_children - 1;
		memcpy(temp, node, sizeof *temp);
		node->id = AST_EXPR_BINARY;
		node->operation = op;
		node->num_children = 0;
		ast_node_t* lhs_node = add_blank(&node, ctx);
		node->last_child_id = temp->id;
		memcpy(lhs_node, temp, sizeof *temp);
		lhs_node->parent = node;
		/* node->pos = node->children->pos; */
		/* node->len = node->children->len + 1; */
		parent->last_child_id = AST_EXPR_BINARY;
		uint32_t j = i + 1;
		parse_expr_operand(node, ctx, &j, 1);
		*index = j;
		/* node->len += node->children[1].len; */
		return 1;
	}
	else if (errs)
	{
		ERR_NODE;
	}
	return 0;
}

static bool_t parse_expr_func(ast_node_t* parent, context_t* ctx, uint32_t* index, const bool_t errs)
{
	const uint32_t i = *index;
	bool_t func_flag = parent->num_children > 0;
	ast_node_t* node = func_flag ? parent->children + parent->num_children - 1 : NULL;
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
		ast_node_t* temp = add_blank(&node, ctx);
		memcpy(temp, &cache, sizeof cache);
		temp->parent = node;
		uint32_t j = i + 1;
		parse_arguments(node, ctx, &j, 1);
		*index = j;
		return 1;
	}
	else if (errs)
	{
		// oh shid error handling aaaaAAAAAAAAAAAaaaaaaaaaa
		ERR_NODE
	}
	return 0;
}


static bool_t parse_arguments(ast_node_t* parent, context_t* ctx, uint32_t* index, const bool_t errs)
{
	uint32_t i = *index;
	gdscript_id_t* ids = ctx->ids;
	bool_t res = 1;
	uint32_t n;
	if (ids[i] == GDSCRIPT_PAREN_OPEN && (n = dist_pair(GDSCRIPT_PAREN_OPEN, i, ids, ctx->n)) > 0)
	{
		bool_t arg_flag = 1;
		for (uint32_t j = i + 1; j <= n; ++j)
		{
			if (arg_flag)
			{
				parse_argument(parent, ctx, &j, errs);
				arg_flag = 0;
			}
			else if (ids[i] == GDSCRIPT_COMMA)
			{
				arg_flag = 1;
			}
			else if (errs)
			{
				ERR_NODE;
			}
		}

		*index = i + n;
		return 1;
	}
	else if (errs)
	{
		ERR_NODE
	}
	return res;
}

static bool_t parse_type(ast_node_t* parent, context_t* ctx, uint32_t* index, const bool_t errs)
{

}


// var foo: int = blah blah blah
// for assignment operations, the LHS is either an identifier or a dot expression
static bool_t parse_expr_typed_assignment(ast_node_t* parent, context_t* ctx, uint32_t* index, const bool_t errs)
{
	uint32_t i = *index;
	#define TEMP X(type) && X(assignment)
	#define X(A) parse_expr_##A(parent, ctx, &i, 0)
	if (TEMP && parse_expression(parent, ctx, &i, 0))
	{
		*index = i;
		return 1;
	}
	else if (errs)
	{
		ERR_NODE
	}
	return 0;
}


/*************************/
/* PARSING -- STATEMENTS */
/*************************/


static bool_t parse_stmt_if(ast_node_t* parent, context_t* ctx, uint32_t* index)
{
	// keyword 'if' at index
	ast_node_t* node = add_child(&parent, ctx, AST_STMT_IF);
	uint32_t j = *index + 1;
	if (parse_expression(node, ctx, &j, 1) && ctx->ids[j + 1] == GDSCRIPT_COLON)
	{
		uint32_t req_lws0 = ctx->req_lws;
		ctx->req_lws++;
		j += 2;
		process_tokens(node, ctx, &j);
		ctx->req_lws = req_lws0;
	}
	else
	{
		node->id = AST_ERROR;
		node->err_token = ctx->tokens + *index;
		node->err_len = j - *index + 1;
	}
	*index = j;
	return 1;
}


static bool_t parse_stmt_elif(ast_node_t* parent, context_t* ctx, uint32_t* index)
{
	// keyword 'if' at index
	ast_node_t* node = add_child(&parent, ctx, AST_STMT_ELIF);
	uint32_t j = *index + 1;
	/* parse_expression(if_node, ctx, &j, 1); */
	if (parse_expression(node, ctx, &j, 1) && ctx->ids[j + 1] == GDSCRIPT_COLON)
	{
		uint32_t req_lws0 = ctx->req_lws;
		ctx->req_lws++;
		j += 2;
		process_tokens(node, ctx, &j);
		ctx->req_lws = req_lws0;
	}
	else
	{
		node->id = AST_ERROR;
		node->err_token = ctx->tokens + *index;
		node->err_len = j - *index + 1;
	}
	*index = j;
	return 1;
}

static bool_t parse_stmt_else(ast_node_t* parent, context_t* ctx, uint32_t* index)
{
	// keyword 'else' at index
	ast_node_t* node = add_child(&parent, ctx, AST_STMT_ELSE);
	uint32_t j = *index + 1;
	/* parse_expression(if_node, ctx, &j, 1); */
	if (ctx->ids[j] == GDSCRIPT_COLON)
	{
		uint32_t req_lws0 = ctx->req_lws;
		ctx->req_lws++;
		j++;
		process_tokens(node, ctx, &j);
		ctx->req_lws = req_lws0;
	}
	else
	{
		node->id = AST_ERROR;
		node->err_token = ctx->tokens + *index;
		node->err_len = j - *index + 1;
	}
	*index = j;
	return 1;
}

static bool_t parse_stmt_for(ast_node_t* parent, context_t* ctx, uint32_t* index)
{
	ast_node_t* node = add_child(&parent, ctx, AST_STMT_FOR);
	uint32_t j = *index + 1;
	if (parse_expression(node, ctx, &j, 1) && ctx->ids[j] == GDSCRIPT_COLON)
	{
		uint32_t req_lws0 = ctx->req_lws;
		ctx->req_lws++;
		j++;
		process_tokens(node, ctx, &j);
		ctx->req_lws = req_lws0;
	}
	else
	{
		node->id = AST_ERROR;
		node->err_token = ctx->tokens + *index;
		node->err_len = j - *index + 1;
	}
	*index = j;
	return 1;
}

static bool_t parse_stmt_while(ast_node_t* parent, context_t* ctx, uint32_t* index)
{
	ast_node_t* node = add_child(&parent, ctx, AST_STMT_WHILE);
	uint32_t j = *index + 1;
	if (parse_expression(node, ctx, &j, 1) && ctx->ids[j] == GDSCRIPT_COLON)
	{
		uint32_t req_lws0 = ctx->req_lws;
		ctx->req_lws++;
		j++;
		process_tokens(node, ctx, &j);
		ctx->req_lws = req_lws0;
	}
	else
	{
		node->id = AST_ERROR;
		node->err_token = ctx->tokens + *index;
		node->err_len = j - *index + 1;
	}
	*index = j;
	return 1;
}

static bool_t parse_stmt_return(ast_node_t* parent, context_t* ctx, uint32_t* index)
{
	const uint32_t i = *index;
	ast_node_t* node = add_child(&parent, ctx, AST_STMT_RETURN);
	if (ctx->ids[i + 1] != GDSCRIPT_LINE_END || ctx->ids[i + 1] != GDSCRIPT_SEMICOLON)
	{
		uint32_t j = i + 1;
		parse_expression(node, ctx, &j, 1);
		*index = j;
	}
	return 1;
}

static void error_til_line_end(ast_node_t* parent, context_t* ctx, uint32_t* index)
{
	uint32_t err_len = *index;
	const gdscript_id_t* ids = ctx->ids;
	for (uint32_t i = *index; i < ctx->n; ++i)
	{
		if (!(ids[i] == GDSCRIPT_LINE_END || ids[i] == GDSCRIPT_SEMICOLON))
		{
			err_len += 1;
		}
		else
		{
			break;
		}
	}
	if (err_len > 0)
	{
		*index += err_len;
	}
}

static bool_t parse_stmt_break(ast_node_t* parent, context_t* ctx, uint32_t* index)
{
	ast_node_t* node = add_child(parent, ctx, AST_STMT_BREAK);
	*index += 1;
	error_til_line_end(node, ctx, index);
}

static bool_t parse_stmt_continue(ast_node_t* parent, context_t* ctx, uint32_t* index)
{
	ast_node_t* node = add_child(parent, ctx, AST_STMT_CONTINUE);
	*index += 1;
	error_til_line_end(node, ctx, index);
}

static bool_t parse_stmt_pass(ast_node_t* parent, context_t* ctx, uint32_t* index)
{
	ast_node_t* node = add_child(parent, ctx, AST_STMT_PASS);
	*index += 1;
	error_til_line_end(node, ctx, index);
}


/***************************/
/* PARSING -- DECLARATIONS */
/***************************/


static bool_t parse_decl_class(ast_node_t* parent, context_t* ctx, uint32_t* index)
{
	ast_node_t* node = add_child(&parent, ctx, AST_DECL_CLASS);
	gdscript_id_t* ids = ctx->ids;
	const uint32_t i = *index + 1;
	if (ids[i] == GDSCRIPT_IDENTIFIER)
	{
		node = add_child(&node, ctx, AST_IDENTIFIER);
		node->symbol = ctx->tokens + i;
		*index = i;
		return 1;
	}
	else
	{
		node->id = AST_ERROR;
		node->err_token = ctx->tokens + i - 1;
		node->err_len = 1;
		// TODO: something about expected to be followed by an identifier
		return 0;
	}
}

static bool_t parse_decl_extends(ast_node_t* parent, context_t* ctx, uint32_t* index)
{
	ast_node_t* node = add_child(&parent, ctx, AST_DECL_EXTENDS);
	const uint32_t i = *index + 1;
	switch (ctx->ids[i])
	{
	case GDSCRIPT_IDENTIFIER:
		node = add_child(&node, ctx, AST_IDENTIFIER);
		node->symbol = ctx->tokens + i;
		break;
	case GDSCRIPT_STRING:
		node = add_child(&node, ctx, AST_EXPR_SYMBOLIC);
		node->symbolic_id = SYMBOL_STRING;
		break;
	default:
		node->id = AST_ERROR;
		node->err_token = ctx->tokens + i - 1;
		node->err_len = 1;
		return 0;
	}
	return 1;
}

static bool_t parse_decl_subclass(ast_node_t* parent, context_t* ctx, uint32_t* index)
{
	ast_node_t* node = add_child(&parent, ctx, AST_DECL_SUBCLASS);
	const gdscript_id_t* ids = ctx->ids;
	const uint32_t i = *index;
	if (ids[i + 1] == GDSCRIPT_IDENTIFIER && ids[i + 2] == GDSCRIPT_COLON)
	{
		ast_node_t* id_node = add_child(&node, ctx, AST_IDENTIFIER);
		id_node->symbol = ctx->tokens + i + 1;
		uint32_t req_lws0 = ctx->req_lws;
		ctx->req_lws++;
		uint32_t j = i + 3;
		process_tokens(node, ctx, &j);
		ctx->req_lws = req_lws0;
		return 1;
	}
	else
	{
		node->id = AST_ERROR;
		node->err_token = ctx->tokens;
		node->err_len = 1;
		return 0;
	}
}


// NOTE: DON'T WORRY ABOUT IT, JUST PARSE IT.  CHECK AFTERWARDS.
static bool_t parse_decl_var(ast_node_t* parent, context_t* ctx, uint32_t* index)
{
	ast_node_t* node = add_child(&parent, ctx, AST_DECL_VAR);
	const uint32_t i = *index;
	const gdscript_id_t* ids = ctx->ids;
	if (ids[i + 1] == GDSCRIPT_IDENTIFIER)
	{
		ast_node_t* identifier = add_child(&node, ctx, AST_IDENTIFIER);
		identifier->symbol = ctx->tokens + i + 1;
		const uint32_t j = i + 2;
		if (parse_expression(node, ctx, &j, 0))
		{
			*index = j;
		}
		else
		{
			*index = i + 1;
		}
		return 1;
		#undef X
		#undef TEMP
	}
	else
	{
		parent->last_child_id = AST_ERROR;
		node->id = AST_ERROR;
		return 0;
	}
}



static bool_t parse_decl_const(ast_node_t* parent, context_t* ctx, uint32_t* index)
{
{
	ast_node_t* node = add_child(&parent, ctx, AST_DECL_CONST);
	const uint32_t i = *index;
	const gdscript_id_t* ids = ctx->ids;
	if (ids[i + 1] == GDSCRIPT_IDENTIFIER)
	{
		ast_node_t* identifier = add_child(&node, ctx, AST_IDENTIFIER);
		identifier->symbol = ctx->tokens + i + 1;
		const uint32_t j = i + 2;
		#define TEMP \
			X(typed_assignment) || \
			X(inferred_assignment) || \
			X(assignment)
		#define X(A) parse_expr_##A(node, ctx, &j, 0)
		if (TEMP)
		{
			*index = j;
		}
		else
		{
			*index = i + 1;
		}
		return 1;
		#undef X
		#undef TEMP
	}
	else
	{
		parent->last_child_id = AST_ERROR;
		node->id = AST_ERROR;
		return 0;
	}
}}

/**************************/
/* PARSING -- ANNOTATIONS */
/**************************/

static bool_t parse_annotation(ast_node_t* parent, context_t* ctx, uint32_t* index, const ast_id_t annotation_id)
{
	const uint32_t i = *index;
	ast_node_t* node = add_child(&parent, ctx, annotation_id);
	node->annotation.id = ctx->ids[i];
	uint32_t temp = *index + 1;
	if (parse_arguments(node, ctx, &temp, 0))
	{
		node->id = node->children->num_children > 0 ? AST_ERROR : annotation_id;
		node->len = 3 + node->children->num_children;
	}
}

static bool_t parse_annotation_stringn(ast_node_t* parent, context_t* ctx, uint32_t* index, const ast_id_t annotation_id, const uint32_t num_strings)
{
	const uint32_t i = *index;
	ast_node_t* node = add_child(&parent, ctx, annotation_id);
	node->annotation_id = ctx->ids[i];
	uint32_t temp = *index + 1;
	if (parse_arg_list(node, ctx, &temp))
	{
		node->id = node->children->num_children > 0 ? AST_ERROR : annotation_id;
		node->len = 3 + node->children->num_children;
	}
	else
	{
		node->id = AST_ERROR;
	}
}



/********************/
/* TOKEN PROCESSING */
/********************/

static bool_t process_tokens(ast_node_t* parent, context_t* ctx, uint32_t* index)
{
	uint32_t lws = 0;
	const uint32_t num_tokens = ctx->n;
	const gdscript_id_t* ids = ctx->ids;
	for (uint32_t i = *index; i < num_tokens; ++i)
	{
		if (lws == ctx->req_lws)
		{
			ctx->req_lws = process_line(parent, ctx, &i) * (ctx->req_lws > 0);
			lws = 0;
		}
		else if (ids[i] == GDSCRIPT_LEADING_WHITESPACE)
		{
			lws++;
		}
		else if (ids[i] == GDSCRIPT_LINE_END)
		{
			lws = 0;
		}
		else
		{
			*index = i - 1;
			return 1;
			/* shallow_line_error(parent, ctx, &i); */
			/* lws = 0; */
		}
	}
	return 1;
}


static bool_t process_line(ast_node_t* parent, context_t* ctx, uint32_t* index)
{
	const uint32_t num_tokens = ctx->n;
	const gdscript_id_t* ids = ctx->ids;
	for (int i = *index; i < num_tokens; ++i)
	{
		const gdscript_id_t id = ids[i];
		switch (id)
		{
		case GDSCRIPT_LEADING_WHITESPACE:
			ctx->req_lws++;
			break;
		case GDSCRIPT_LINE_END:
			*index = i;
			return ctx->req_lws;
		case GDSCRIPT_SEMICOLON:
			*index = i + 1;
			context_t semicolon_ctx = *ctx;
			semicolon_ctx->req_lws = 0;
			process_line(parent, &semicolon_ctx, index);
			return ctx->req_lws;
		#define X(ENUM, FUNC) case GDSCRIPT_##ENUM: parse_##FUNC(parent, ctx, index); break;
			KEYVALUES
		#undef X
		default:
			parse_expression(parent, ctx, index);
		}
	}
}

void err_unprocessed(uint32_t i, const uint32_t n)
{
	for (; i < n; ++i)
	{
		// TODO: message about how these are too shallowly nested
	}
}


ast_node_t* gdscript_ast_generate(free_list_t* fl, const token_t* tokens, gdscript_id_t* ids, const uint32_t num_tokens)
{
	ast_node_t* root = free_list_calloc(fl, sizeof *root);
	root->id = AST_ROOT;
	root->len = num_tokens;
	context_t ctx =
	{
		.tokens = tokens,
		.ids = ids,
		.n = num_tokens,
		.fl=fl,
		.root = root
	};
	uint32_t index = 0;
	process_tokens(root, &ctx, &index);
	err_unprocessed(index, num_tokens);
	return ctx.root;
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
	for (uint32_t i = 0; i < n; ++i)
	{
		check_ast(node->children + i, flags, i, n);
	}
}
