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

// TODO: DeMorgen laws
// TODO: SEMICOLONS?????

#define RESIZE_SCALE 1.618
#define RESIZE_OVERHEAD 4096

typedef unsigned char bool_t;

// these bad boys are just here so I can't lsp auto-complete to expand these
// NOTE: WHY DONT I USE RECORDINGS? Because I don't want to :)
#define F1(NAME) static bool_t parse_##NAME(ast_node_t* parent, parse_args_t* args, uint32_t* index)
#define F2(NAME) static bool_t parse_##NAME(ast_node_t* parent, parse_args_t* args, uint32_t* index, const uint8_t errs)


typedef struct parse_args_t
{
	token_t* tokens;
	gdscript_id_t* ids;
	free_list_t* fl;
	ast_node_t* root;
	uint32_t n;
	uint32_t req_lws;
} parse_args_t;

/* #define	PARSE_PTRS							\ */
/* 	X(NONE,							error)			\ */
/* 	X(ERROR,						error)			\ */
/* 	X(SEMICOLON,					error)			\ */
/* 	X(LINE_END,						error)			\ */
/* 	X(LINE_CONTINUE,				error)			\ */
/* 	X(COMMENT,						error)			\ */
/* 	X(COLON,						colon)			\ */
/* 	X(PERIOD,						error)			\ */
/* 	X(COMMA,						error)			\ */
/* 	X(ARROW,						todo)			\ */
/* 	X(PAREN_OPEN,					paren)		\ */
/* 	X(PAREN_CLOSE,					error)			\ */
/* 	X(BRACE_OPEN,					expr_dict)		\ */
/* 	X(BRACE_CLOSE,					error)			\ */
/* 	X(BRACKET_OPEN,					expr_array)		\ */
/* 	X(BRACKET_CLOSE,				error)			\ */
/* 	X(OP_ASSIGNMENT_POWER,			expr_binary)			\ */
/* 	X(OP_POWER,						expr_binary)			\ */
/* 	X(OP_ASSIGNMENT_PRODUCT,		expr_binary)			\ */
/* 	X(OP_PRODUCT,					expr_binary)			\ */
/* 	X(OP_ASSIGNMENT_SUM,			expr_binary)			\ */
/* 	X(OP_SUM,						plus)			\ */
/* 	X(OP_ASSIGNMENT_DIFFERENCE,		expr_binary)			\ */
/* 	X(OP_DIFFERENCE,				minus)			\ */
/* 	X(OP_ASSIGNMENT_DIVISION,		expr_binary)			\ */
/* 	X(OP_DIVISION,					expr_binary)			\ */
/* 	X(OP_ASSIGNMENT_MODULO,			expr_binary)			\ */
/* 	X(OP_MODULO,					expr_binary)			\ */
/* 	X(OP_BOOL_AND,					expr_binary)			\ */
/* 	X(OP_ASSIGNMENT_BITWISE_AND,	expr_binary)			\ */
/* 	X(OP_BITWISE_AND,				expr_binary)			\ */
/* 	X(OP_BOOL_OR,					expr_binary)			\ */
/* 	X(OP_ASSIGNMENT_BITWISE_OR,		expr_binary)			\ */
/* 	X(OP_BITWISE_OR,				expr_binary)			\ */
/* 	X(OP_ASSIGNMENT_BITSHIFT_LEFT,	expr_binary)			\ */
/* 	X(OP_BITSHIFT_LEFT,				expr_binary)			\ */
/* 	X(OP_ASSIGNMENT_BITSHIFT_RIGH,	expr_binary)			\ */
/* 	X(OP_BITSHIFT_RIGHT,			expr_binary)			\ */
/* 	X(OP_ASSIGNMENT_XOR,			expr_binary)			\ */
/* 	X(OP_BITWISE_XOR,				expr_binary)			\ */
/* 	X(OP_EQUALS,					expr_binary)			\ */
/* 	X(OP_INEQUALS,					expr_binary)			\ */
/* 	X(OP_LESS_THAN_EQUALS,			expr_binary)			\ */
/* 	X(OP_LESS_THAN,					expr_binary)			\ */
/* 	X(OP_GREATER_THAN_EQUALS,		expr_binary)			\ */
/* 	X(OP_GREATER_THAN,				expr_binary)			\ */
/* 	X(OP_ASSIGNMENT_TYPED,			expr_binary)			\ */
/* 	X(OP_ASSIGNMENT,				assignment)			\ */
/* 	X(OP_PAIR,						expr_binary)			\ */
/* 	X(OP_TYPE,						expr_binary)			\ */
/* 	X(OP_DOT,						expr_binary)			\ */
/* 	X(OP_BITWISE_NOT,				expr_unary)			\ */
/* 	X(OP_IDENTITY,					plus)			\ */
/* 	X(OP_NEGATION,					minus)			\ */
/* 	X(OP_BOOL_NOT,					expr_unary)			\ */
/* 	X(EXPORT_CATEGORY,				todo)			\ */
/* 	X(EXPORT_COLOR_NO_ALPHA,		todo)			\ */
/* 	X(EXPORT_CUSTOM,				todo)			\ */
/* 	X(EXPORT_DIR,					todo)			\ */
/* 	X(EXPORT_ENUM,					todo)			\ */
/* 	X(EXPORT_EXP_EASING,			todo)			\ */
/* 	X(EXPORT_FILE,					todo)			\ */
/* 	X(EXPORT_FLAGS_2D_NAVIGATION,	todo)			\ */
/* 	X(EXPORT_FLAGS_2D_PHYSICS,		todo)			\ */
/* 	X(EXPORT_FLAGS_2D_RENDER,		todo)			\ */
/* 	X(EXPORT_FLAGS_3D_NAVIGATION,	todo)			\ */
/* 	X(EXPORT_FLAGS_3D_PHYSICS,		todo)			\ */
/* 	X(EXPORT_FLAGS_3D_RENDER,		todo)			\ */
/* 	X(EXPORT_FLAGS_AVOIDANCE,		todo)			\ */
/* 	X(EXPORT_FLAGS,					todo)			\ */
/* 	X(EXPORT_GLOBAL_DIR,			todo)			\ */
/* 	X(EXPORT_GLOBAL_FILE,			todo)			\ */
/* 	X(EXPORT_GROUP,					todo)			\ */
/* 	X(EXPORT_MULTILINE,				todo)			\ */
/* 	X(EXPORT_NODE_PATH,				todo)			\ */
/* 	X(EXPORT_PLACEHOLDER,			todo)			\ */
/* 	X(EXPORT_RANGE,					todo)			\ */
/* 	X(EXPORT_STORAGE,				todo)			\ */
/* 	X(EXPORT_SUBGROUP,				todo)			\ */
/* 	X(EXPORT_TOOL_BUTTON,			todo)			\ */
/* 	X(EXPORT,						todo)			\ */
/* 	X(ICON,							todo)			\ */
/* 	X(ONREADY,						todo)			\ */
/* 	X(RPC,							todo)			\ */
/* 	X(STATIC_UNLOAD,				todo)			\ */
/* 	X(TOOL,							todo)			\ */
/* 	X(WARNING_IGNORE_RESTORE,		todo)			\ */
/* 	X(WARNING_IGNORE_STOP,			todo)			\ */
/* 	X(WARNING_IGNORE,				todo)			\ */
/* 	X(IF,							stmt_if)		\ */
/* 	X(ELIF,							stmt_elif)		\ */
/* 	X(ELSE,							stmt_else)		\ */
/* 	X(FOR,							stmt_for)		\ */
/* 	X(WHILE,						stmt_while)		\ */
/* 	X(MATCH,						stmt_match)		\ */
/* 	X(BREAK,						stmt_break)		\ */
/* 	X(CONTINUE,						stmt_continue)	\ */
/* 	X(PASS,							stmt_pass)		\ */
/* 	X(RETURN,						stmt_return)	\ */
/* 	X(CLASS_NAME,					decl_class)		\ */
/* 	X(CLASS,						decl_subclass)	\ */
/* 	X(EXTENDS,						decl_extends)	\ */
/* 	X(IS,							todo)			\ */
/* 	X(IN,							todo)			\ */
/* 	X(AS,							todo)			\ */
/* 	X(SELF,							todo)			\ */
/* 	X(SUPER,						todo)			\ */
/* 	X(SIGNAL,						decl_signal)	\ */
/* 	X(FUNC,							decl_func)		\ */
/* 	X(STATIC,						decl_static)			\ */
/* 	X(CONST,						decl_const)			\ */
/* 	X(ENUM,							decl_enum)		\ */
/* 	X(VAR,							decl_var)		\ */
/* 	X(BREAKPOINT,					stmt_breakpoint)			\ */
/* 	X(PRELOAD,						expr_preload)			\ */
/* 	X(AWAIT,						expr_await)			\ */
/* 	X(YIELD,						expr_yield)			\ */
/* 	X(ASSERT,						expr_assert)			\ */
/* 	X(GET,							stmt_get)			\ */
/* 	X(SET,							stmt_set)			\ */
/* 	X(VOID,							literal)		\ */
/* 	X(NULL,							literal)		\ */
/* 	X(TRUE,							literal)		\ */
/* 	X(FALSE,						literal)		\ */
/* 	X(PI,							literal)		\ */
/* 	X(TAU,							literal)		\ */
/* 	X(INF,							literal)		\ */
/* 	X(NAN,							literal)		\ */
/* 	X(STRING, string)								\ */
/* 	X(NUMBER, number)								\ */
/* 	X(IDENTIFIER, identifier) */

// NOTE: think about checkig if unary ops are binary
#define LINE_CASES						\
		X(PAREN_OPEN,					expr_group)			\
		X(BRACE_OPEN,					expr_dict)			\
		X(BRACKET_OPEN,					expr_array)			\
		X(OP_IDENTITY,					expr_unary)			\
		X(OP_NEGATION,					expr_unary)			\
		X(OP_BITWISE_NOT,				expr_unary)			\
		X(OP_BOOL_NOT,					expr_unary)			\
		X(EXPORT_CATEGORY,				todo)				\
		X(EXPORT_COLOR_NO_ALPHA,		todo)				\
		X(EXPORT_CUSTOM,				todo)				\
		X(EXPORT_DIR,					todo)				\
		X(EXPORT_ENUM,					todo)				\
		X(EXPORT_EXP_EASING,			todo)				\
		X(EXPORT_FILE,					todo)				\
		X(EXPORT_FLAGS_2D_NAVIGATION,	todo)				\
		X(EXPORT_FLAGS_2D_PHYSICS,		todo)				\
		X(EXPORT_FLAGS_2D_RENDER,		todo)				\
		X(EXPORT_FLAGS_3D_NAVIGATION,	todo)				\
		X(EXPORT_FLAGS_3D_PHYSICS,		todo)				\
		X(EXPORT_FLAGS_3D_RENDER,		todo)				\
		X(EXPORT_FLAGS_AVOIDANCE,		todo)				\
		X(EXPORT_FLAGS,					todo)				\
		X(EXPORT_GLOBAL_DIR,			todo)				\
		X(EXPORT_GLOBAL_FILE,			todo)				\
		X(EXPORT_GROUP,					todo)				\
		X(EXPORT_MULTILINE,				todo)				\
		X(EXPORT_NODE_PATH,				todo)				\
		X(EXPORT_PLACEHOLDER,			todo)				\
		X(EXPORT_RANGE,					todo)				\
		X(EXPORT_STORAGE,				todo)				\
		X(EXPORT_SUBGROUP,				todo)				\
		X(EXPORT_TOOL_BUTTON,			todo)				\
		X(EXPORT,						todo)				\
		X(ICON,							todo)				\
		X(ONREADY,						todo)				\
		X(RPC,							todo)				\
		X(STATIC_UNLOAD,				todo)				\
		X(TOOL,							todo)				\
		X(WARNING_IGNORE_RESTORE,		todo)				\
		X(WARNING_IGNORE_STOP,			todo)				\
		X(WARNING_IGNORE,				todo)				\
		X(IF,							stmt_if)			\
		X(ELIF,							stmt_elif)			\
		X(ELSE,							stmt_else)			\
		X(FOR,							stmt_for)			\
		X(WHILE,						stmt_while)			\
		X(MATCH,						stmt_match)			\
		X(BREAK,						stmt_break)			\
		X(CONTINUE,						stmt_continue)		\
		X(PASS,							stmt_pass)			\
		X(RETURN,						stmt_return)		\
		X(CLASS_NAME,					decl_class)			\
		X(CLASS,						decl_subclass)		\
		X(EXTENDS,						decl_extends)		\
		X(SELF,							todo)				\
		X(SUPER,						todo)				\
		X(SIGNAL,						decl_signal)		\
		X(FUNC,							decl_func)			\
		X(STATIC,						decl_static)		\
		X(CONST,						decl_const)			\
		X(ENUM,							decl_enum)			\
		X(VAR,							decl_var)			\
		X(BREAKPOINT,					stmt_breakpoint)	\
		X(PRELOAD,						expr_preload)		\
		X(AWAIT,						expr_await)			\
		X(YIELD,						expr_yield)			\
		X(ASSERT,						expr_assert)		\
		X(GET,							stmt_get)			\
		X(SET,							stmt_set)			\
		X(VOID,							expr_symbolic)			\
		X(NULL,							expr_symbolic)			\
		X(TRUE,							expr_symbolic)			\
		X(FALSE,						expr_symbolic)			\
		X(PI,							expr_symbolic)			\
		X(TAU,							expr_symbolic)			\
		X(INF,							expr_symbolic)			\
		X(NAN,							expr_symbolic)			\
		X(STRING,						expr_symbolic)				\
		X(NUMBER,						expr_symbolic)				\
		X(IDENTIFIER,					identifier)

// TODO expand then delete the redundant forward declarations
#define X(_, FUNC) \
	static bool_t parse_##FUNC(ast_node_t* parent, parse_args_t* args, uint32_t* index, const uint8_t errs);
/* PARSE_PTRS */
LINE_CASES
#undef X


/*****************************/
/* CONVENIENT MACROS & FUNCS */
/*****************************/


#define BITFLAGS(DEST, BIT_ARRAY, N) \
	for (uint32_t i = 0; i < N; ++i) \
		DEST |= BIT_ARRAY[i] << i;

#define ERROR_AT_INDEX									\
	ast_node_t* error_node = add_error(&parent, args);	\
	error_node->err_token = args->tokens + *index; \
	error_node->err_len = 1;

static bool_t match(uint8_t* matches, const gdscript_id_t* seq, const uint32_t len, const gdscript_id_t* ground_truth, const uint32_t n)
{
	uint8_t res = 1;
	for (uint32_t i = 0; i < len && i < n; ++i)
	{
		const uint8_t temp = seq[i] == ground_truth[i];
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

static ast_node_t* add_blank(ast_node_t** parent, parse_args_t* args)
{
	const size_t num = (*parent)->num_children + 1;
	point_relative_offset(parent, args->fl->memory);
	ast_node_t* children = ast_alloc(args->fl, &args->root, num * sizeof **parent);
	point_absolute_offset(parent, args->fl->memory);
	ast_node_t* p = *parent;
	if (num > 1)
	{
		memcpy(children, p->children, (num - 1) * sizeof *p);
		free_list_free(args->fl, p->children);
	}
	memset(children + num - 1, 0x00, sizeof *p);
	p->children = children;
	p->num_children = num;
	ast_node_t* last_child = children + num - 1;
	last_child->len = 1;
	last_child->parent = p;
	return last_child;
}

static ast_node_t* add_child(ast_node_t** parent, parse_args_t* args, const ast_id_t id)
{
	ast_node_t* last_child = add_blank(parent, args);
	last_child->id = id;
	(*parent)->last_child_id = id;
	return last_child;
}

static ast_node_t* add_error(ast_node_t** parent, parse_args_t* args)
{
	return add_child(parent, args, AST_ERROR);
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

static bool_t parse_error(ast_node_t* parent, parse_args_t* args, uint32_t* index)
{
	ERROR_AT_INDEX
}


static bool_t parse_symbol(ast_node_t* parent, parse_args_t* args, uint32_t* index)
{
	const uint32_t i = *index;
	ast_node_t* n = add_child(&parent, args, AST_IDENTIFIER);
	n->token_id = args->ids[i];
	return 1;
}

static bool_t parse_identifier(ast_node_t* parent, parse_args_t* args, uint32_t* index, const uint8_t errs)
{
	const uint32_t i = *index;
	if (args->ids[i] == GDSCRIPT_IDENTIFIER)
	{
		ast_node_t* n = add_child(&parent, args, AST_IDENTIFIER);
		n->symbol = args->tokens + i;
		return 1;
	}
	else if (errs)
	{
		ERROR_AT_INDEX
	}
	return 0;
}

static bool_t parse_string(ast_node_t* parent, parse_args_t* args, uint32_t* index, const uint8_t errs)
{
	const uint32_t i = *index;
	if (args->ids[i] == GDSCRIPT_STRING)
	{
		ast_node_t* n = add_child(&parent, args, AST_EXPR_SYMBOLIC);
		n->symbolic_id = AST_SYMBOL_NUMBER;
		return 1;
	}
	else if (errs)
	{
		ERROR_AT_INDEX
	}
	return 0;
}

static bool_t parse_number(ast_node_t* parent, parse_args_t* args, uint32_t* index, const uint8_t errs)
{
	const uint32_t i = *index;
	if (args->ids[i] == GDSCRIPT_NUMBER)
	{
		ast_node_t* n = add_child(&parent, args, AST_EXPR_SYMBOLIC);
		n->symbolic_id = AST_SYMBOL_NUMBER;
		return 1;
	}
	else if (errs)
	{
		ERROR_AT_INDEX
	}
	return 0;
}

static bool_t parse_literal(ast_node_t* parent, parse_args_t* args, uint32_t* index, const uint8_t errs)
{

	const uint32_t i = *index;
	ast_symbol_t symbol;
	switch (args->ids[i])
	{
	#define X(_, VALUE) case GDSCRIPT_##VALUE:
	GDSCRIPT_LITERALS
		symbol = AST_SYMBOL_LITERAL;
		break;
	#undef X
	default:
		symbol = AST_SYMBOL_NONE;
	}
	if (symbol)
	{
		ast_node_t* node = add_child(&parent, args, AST_EXPR_SYMBOLIC);
		node->symbolic_id = symbol;
		return 1;
	}
	else if (errs)
	{
		ERROR_AT_INDEX
	}
	return 0;
}


/**************************/
/* PARSING -- EXPRESSIONS */
/**************************/

static uint8_t parse_expr_symbolic(ast_node_t* parent, parse_args_t* args, uint32_t* index, const uint8_t errs);
static uint8_t parse_expr_unary(ast_node_t* parent, parse_args_t* args, uint32_t* index, const uint8_t errs);
static uint8_t parse_expr_binary(ast_node_t* parent, parse_args_t* args, uint32_t* index, const uint8_t errs);
static uint8_t parse_expr_operation(ast_node_t* parent, parse_args_t* args, uint32_t* index, const uint8_t errs);
static uint8_t parse_expr_func(ast_node_t* parent, parse_args_t* args, uint32_t* index, const uint8_t errs);
static uint8_t parse_expr_array(ast_node_t* parent, parse_args_t* args, uint32_t* index, const uint8_t errs);
static uint8_t parse_expr_dict(ast_node_t* parent, parse_args_t* args, uint32_t* index, const uint8_t errs);
static uint8_t parse_expr_group(ast_node_t* parent, parse_args_t* args, uint32_t* index, const uint8_t errs);
static uint8_t parse_expr_ternary(ast_node_t* parent, parse_args_t* args, uint32_t* index, const uint8_t errs);

static bool_t parse_expression(ast_node_t* parent, parse_args_t* args, uint32_t* index, const uint8_t errs)
{
	const gdscript_id_t* ids = args->ids;
	for (uint32_t i = *index; i < args->n; ++i)
	{
		switch (ids[i])
		{
		case GDSCRIPT_IF:
			parse_expr_ternary(parent, args, index, errs);
			break;
		case GDSCRIPT_IDENTIFIER:
			parse_expr_symbolic(parent, args, index, errs);
			break;
		case GDSCRIPT_PAREN_OPEN:
			parse_expr_func(parent, args, index, errs) || parse_expr_group(parent, args, index, errs);
			break;
		case GDSCRIPT_BRACE_OPEN:
			parse_expr_dict(parent, args, index, errs);
			break;
		case GDSCRIPT_BRACKET_OPEN:
			parse_expr_array(parent, args, index, errs);
			break;
		#define X(_, VALUE) case GDSCRIPT_##VALUE: parse_expr_operation(parent, args, index, errs); break;
			GDSCRIPT_OPERATORS
		#undef X
		default:
			if (errs)
			{
				ERROR_AT_INDEX
			}
			return 0;
		}
	}
	return 1;
}

static uint8_t parse_expr_group(ast_node_t* parent, parse_args_t* args, uint32_t* index, const uint8_t errs)
{
	const uint32_t i = *index;
	gdscript_id_t* ids = args->ids;
	const uint32_t n = args->n;
	uint32_t count;
	uint8_t flag = ids[i] == GDSCRIPT_PAREN_OPEN;
	flag &= (count = dist_pair(GDSCRIPT_PAREN_OPEN, i, ids, n)) > 0;
	if (flag)
	{
		ast_node_t* node = add_child(&parent, args, AST_EXPR_GROUP);
		/* node->pos = i; */
		uint32_t j = i + 1;
		parse_args_t sub_args = *args;
		sub_args.n = i + count;
		parse_expression(node, &sub_args, &j, 1);
		*index = j;
		return 1;
	}
	else if (errs)
	{
		ERROR_AT_INDEX
	}
	return 0;
}

static uint8_t parse_expr_symbolic(ast_node_t* parent, parse_args_t* args, uint32_t* index, const uint8_t errs)
{
	const uint32_t i = *index;
	ast_symbol_t symbol;
	switch (args->ids[i])
	{
	case GDSCRIPT_IDENTIFIER:
		symbol = AST_SYMBOL_IDENTIFIER;
		break;
	case GDSCRIPT_NUMBER:
		symbol = AST_SYMBOL_NUMBER;
		break;
	case GDSCRIPT_STRING:
		symbol = AST_SYMBOL_STRING;
		break;
	#define X(_, VALUE) case GDSCRIPT_##VALUE:
	GDSCRIPT_LITERALS
		symbol = AST_SYMBOL_LITERAL;
		break;
	#undef X
	default:
		symbol = AST_SYMBOL_NONE;
	}
	if (symbol)
	{
		ast_node_t* node = add_child(&parent, args, AST_EXPR_SYMBOLIC);
		node->symbolic_id = symbol;
		return 1;
	}
	else if (errs)
	{
		ERROR_AT_INDEX
	}
	return 0;
}

static uint8_t parse_expr_operation(ast_node_t* parent, parse_args_t* args, uint32_t* index, const uint8_t errs)
{
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
	const uint32_t n = args->n;
	gdscript_id_t* ids = args->ids;
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
	uint8_t binop_flag = parent->num_children > 0;
	binop_flag &= parent->last_child_id & AST_EXPRESSION && // NOTE: bitwise AND-assignment
		(parent->last_child_id == AST_EXPR_SYMBOLIC ||
		 parent->last_child_id == AST_EXPR_UNARY ||
		 parent->last_child_id == AST_EXPR_GROUP ||
		 parent->last_child_id == AST_EXPR_FUNC);
	uint32_t j = i + 1;
	uint8_t res;
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
		ast_node_t* lhs = add_child(&last_child, args, temp->id);
		last_child->lhs = lhs; // redundant -- look at the structs
		res = parse_expression(last_child, args, &j, 1);
		last_child->rhs = last_child->children + 1;
	}
	else
	{
		ast_node_t* unary_node = add_child(&parent, args, AST_EXPR_UNARY);
		unary_node->operation = op;
	#define UNARY_RHS \
		X(group) || \
		X(func) || \
		X(symbolic)
	#define X(A) parse_expr_##A(unary_node, args, &j, 0)
		res = UNARY_RHS;
		#undef X
	}
	*index = j;
	return res;
}

static uint8_t parse_expr_unary(ast_node_t* parent, parse_args_t* args, uint32_t* index, const uint8_t errs)
{
	const uint32_t i = *index;
	const uint32_t n = args->n;
	gdscript_id_t* ids = args->ids;
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
		ast_node_t* unary_node = add_child(&parent, args, AST_EXPR_UNARY);
		unary_node->operation = op;
	#define UNARY_RHS \
		X(group) || \
		X(func) || \
		X(symbolic)
	#define X(A) parse_expr_##A(unary_node, args, &j, 0)
		return UNARY_RHS;
		#undef X
	}
	else if (errs)
	{
		ERROR_AT_INDEX
	}
	return 0;
}

static uint8_t parse_expr_binary(ast_node_t* parent, parse_args_t* args, uint32_t* index, const uint8_t errs)
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
	const uint32_t n = args->n;
	gdscript_id_t* ids = args->ids;
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
	uint8_t flag = op > 0;
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
		ast_node_t* lhs_node = add_blank(&node, args);
		node->last_child_id = temp->id;
		memcpy(lhs_node, temp, sizeof *temp);
		lhs_node->parent = node;
		node->pos = node->children->pos;
		node->len = node->children->len + 1;
		parent->last_child_id = AST_EXPR_BINARY;
		uint32_t j = i + 1;
		parse_expr_operand(node, args, &j, 1);
		*index = j;
		node->len += node->children[1].len;
		return 1;
	}
	else if (errs)
	{
		ERROR_AT_INDEX;
	}
	return 0;
}

static uint8_t parse_expr_func(ast_node_t* parent, parse_args_t* args, uint32_t* index, const uint8_t errs)
{
	const uint32_t i = *index;
	if (errs)
	{
		// oh shid error handling aaaaAAAAAAAAAAAaaaaaaaaaa
		ERROR_AT_INDEX
	}
	return 0;
}


static uint8_t parse_typed_assignment(ast_node_t* parent, parse_args_t* args, uint32_t *index)
{
	// identifier : indetifier = non-assignning expression
}

/*************************/
/* PARSING -- STATEMENTS */
/*************************/

static uint8_t parse_stmt_arguments(ast_node_t* parent, parse_args_t* args, uint32_t* index, const uint8_t errs)
{
	uint32_t i = *index;
	gdscript_id_t* ids = args->ids;
	uint8_t res = 1;
	uint32_t n;
	if (ids[i] == GDSCRIPT_PAREN_OPEN && (n = dist_pair(GDSCRIPT_PAREN_OPEN, i, ids, args->n)) > 0)
	{
		ast_node_t* node = add_child(&parent, args, AST_STMT_ARGUMENTS);
		node->pos = i;
		node->len = n;
		parse_args_t local_args = *args;
		local_args.n = n;
		uint32_t j = i + 1;
		parse_arguments(node, local_args, &j, 1);
		*index = i + n;
		return 1;
	}
	else if (errs)
	{
		ERROR_AT_INDEX
	}
	return res;
}

static uint8_t parse_arguments(ast_node_t* parent, parse_args_t* args, uint32_t* index, const uint8_t errs)
{
	const uint32_t n = args->n;
	uint8_t arg_flag = 1;
	for (int i = *index; i < n; ++i)
	{
		if (arg_flag)
		{
			parse_expression(parent, args, &i, errs);
			arg_flag = 0;
		}
		else if (args->ids[i] == GDSCRIPT_COMMA)
		{
			arg_flag = 1;
		}
		else if (errs)
		{
			// rest of the tokens are errors
			ast_node_t* error_node = add_error(&parent, args);
			error_node->pos = i;
			error_node->len = n - i - 1;
			return 0;
		}
	}
	return 1;
}

static uint8_t parse_stmt_if(ast_node_t* parent, parse_args_t* args, uint32_t* index, const uint8_t errs)
{
	// keyword 'if' at index
	ast_node_t* if_node = add_child(&parent, args, AST_STMT_IF);
	uint32_t j = *index + 1;
	parse_expression(if_node, args, &j, 1);
	if (args->ids[j + 1] == GDSCRIPT_COLON)
	{
		uint32_t req_lws0 = args->req_lws;
		args->req_lws++;
		j += 2;
		process_tokens(if_node, args, &j);
		args->req_lws = req_lws0;
	}
	else if (errs)
	{
		if_node->id = AST_ERROR;
		if_node->err_token = args->tokens + *index;
		if_node->err_len = j - *index + 1;
	}
	*index = j
	return 0;
}


static uint8_t parse_stmt_elif(ast_node_t* parent, parse_args_t* args, uint32_t* index, const uint8_t errs)
{
	// keyword 'if' at index
	ast_node_t* if_node = add_child(&parent, args, AST_STMT_ELIF);
	uint32_t j = *index + 1;
	parse_expression(if_node, args, &j, 1);
	if (args->ids[j + 1] == GDSCRIPT_COLON)
	{
		uint32_t req_lws0 = args->req_lws;
		args->req_lws++;
		j += 2;
		process_tokens(if_node, args, &j);
		args->req_lws = req_lws0;
	}
	else if (errs)
	{
		if_node->id = AST_ERROR;
		if_node->err_token = args->tokens + *index;
		if_node->err_len = j - *index + 1;
	}
	*index = j
	return 0;
}

static uint8_t parse_stmt_else(ast_node_t* parent, parse_args_t* args, uint32_t* index, const uint8_t errs)
{
	// keyword 'if' at index
	ast_node_t* if_node = add_child(&parent, args, AST_STMT_ELSE);
	uint32_t j = *index + 1;
	/* parse_expression(if_node, args, &j, 1); */
	if (args->ids[j] == GDSCRIPT_COLON)
	{
		uint32_t req_lws0 = args->req_lws;
		args->req_lws++;
		j++;
		process_tokens(if_node, args, &j);
		args->req_lws = req_lws0;
	}
	else if (errs)
	{
		if_node->id = AST_ERROR;
		if_node->err_token = args->tokens + *index;
		if_node->err_len = j - *index + 1;
	}
	*index = j
	return 0;
}

static uint8_t parse_stmt_return(ast_node_t* parent)

		X(RETURN,						stmt_return)		\


/**************************/
/* PARSING -- ANNOTATIONS */
/**************************/

static uint8_t parse_annotation(ast_node_t* parent, parse_args_t* args, uint32_t* index, const ast_id_t annotation_id)
{
	const uint32_t i = *index;
	ast_node_t* node = add_child(&parent, args, annotation_id);
	node->annotation.id = args->ids[i];
	uint32_t temp = *index + 1;
	if (parse_arguments(node, args, &temp, 0))
	{
		node->id = node->children->num_children > 0 ? AST_ERROR : annotation_id;
		node->len = 3 + node->children->num_children;
	}
}

static uint8_t parse_annotation_stringn(ast_node_t* parent, parse_args_t* args, uint32_t* index, const ast_id_t annotation_id, const uint32_t num_strings)
{
	const uint32_t i = *index;
	ast_node_t* node = add_child(&parent, args, annotation_id);
	node->annotation_id = args->ids[i];
	uint32_t temp = *index + 1;
	if (parse_arg_list(node, args, &temp))
	{
		node->id = node->children->num_children > 0 ? AST_ERROR : annotation_id;
		node->len = 3 + node->children->num_children;
	}
	else
	{
		node->id = AST_ERROR;
	}
}

/***************************/
/* PARSING -- DECLARATIONS */
/***************************/

static uint8_t parse_decl_class(ast_node_t* parent, parse_args_t* args, uint32_t* index)
{
	ast_node_t* node = add_child(&parent, args, AST_DECL_CLASS);
	const uint32_t i = *index;
	node->pos = i;
	const uint32_t n = args->n;
	gdscript_id_t* ids = args->ids;
	const token_t* tokens = args->tokens;
	uint32_t j = i + 1;
	if (parse_identifier(node, args, &j, 0))
	{
		node->len = 2;
		*index = j;
		return 1;
	}
	else
	{
		node->id = AST_ERROR;
		// TODO: something about expected to be followed by an identifier
		return 0;
	}
}

static uint8_t parse_decl_extends(ast_node_t* parent, parse_args_t* args, uint32_t* index)
{
	ast_node_t* node = add_child(&parent, args, AST_DECL_EXTENDS);
	node->pos = *index;
	uint32_t i = *index + 1;
	if (parse_identifier(node, args, &i) || parse_string(node, args, &i))
	{
		node->len = 2;
		*index = i;
		return 1;
	}
	else
	{
		node->id = AST_ERROR;
		return 0;
	}
}

static uint8_t parse_expressions(ast_node_t* parent, parse_args_t* args, uint32_t* index, const uint8_t errs)
{
	const uint32_t n = args->n;
	const gdscript_id_t* ids = args->ids;
	ast_node_t* node;
	for (uint32_t i = *index; i < n; ++i)
	{
		switch (ids[i])
		{
		case GDSCRIPT_PAREN_OPEN:
			// check to see if last child is ast_identifier
			// if so, it's a function expression
			if (parent->num_children > 0 && parent->last_child_id == AST_IDENTIFIER)
			{
				node = last_child(parent);
				node->id = AST_EXPR_FUNC;
				return parse_arguments(node, args, &i, errs);
			}
			else
			{
				return parse_expr_group(parent, args, &i, errs);
			}
			// o.w. parse group expression
		case GDSCRIPT_IDENTIFIER:
			parse_identifier(parent, args, &i, errs);
			break;
		case GDSCRIPT_COLON:
			if (parent->id & (AST_EXPRESSION | AST_DECLARATION))
			{
			}
			else if (errs)
			{
				// it's an error, dicts & params have their own custom parsing
			}
		}
	}
}

static uint8_t parse_decl_var(ast_node_t* parent, parse_args_t* args, uint32_t* index)
{
	ast_node_t* node = add_child(&parent, args, AST_DECL_VAR);
	node->pos = *index;
	uint32_t i = *index + 1;
	#define TEMP \
		X(expression) || \
		X(identifier)
	#define X(A) parse_##A(node, args, &i, 0)
	if (TEMP)
	{
		node->len = i - *index + 1;
		*index = i;
		return 1;
	}
	else
	{
		parent->last_child_id = AST_ERROR;
		node->id = AST_ERROR;
		return 0;
	}
	#undef TEMP
}

/* static uint8_t parse_decl_var(ast_node_t* parent, parse_args_t* args, uint32_t* index) */
/* { */
/* 	ast_node_t* node = add_child(&parent, args, AST_DECL_VAR); */
/* 	node->pos = *index; */
/* 	uint32_t i = *index + 1; */
/* 	#define TEMP \ */
/* 		X(typed_assignment) || \ */
/* 		X(inferred_assignment) || \ */
/* 		X(assignment) || \ */
/* 		X(identifier) */
/* 	#define X(A) try_parse_##A(node, args, &i) */
/* 	if (TEMP) */
/* 	{ */
/* 		node->len = i - *index + 1; */
/* 		*index = i; */
/* 		return 1; */
/* 	} */
/* 	else */
/* 	{ */
/* 		node->id = AST_ERROR; */
/* 		return 0 */
/* 	} */
/* 	#undef TEMP */
/* } */

static uint8_t parse_decl_const(ast_node_t* parent, parse_args_t* args, uint32_t* index)
{
	ast_node_t* node = add_child(&parent, args, AST_DECL_CONST);
	node->pos = *index;
	uint32_t i = *index + 1;
	#define TEMP \
		X(typed_assignment) || \
		X(inferred_assignment) || \
		X(assignment) || \
		X(identifier)
	#define X(A) parse_##A(node, args, &i)
	if (TEMP)
	#undef X
	{
		node->len = i - *index + 1;
		*index = i;
		return 1;
	}
	else
	{
		parent->last_child_id = AST_ERROR;
		node->id = AST_ERROR;
		return 0
	}
	#undef TEMP
}


/********************/
/* TOKEN PROCESSING */
/********************/

static uint32_t process_line(ast_node_t* parent, parse_args_t* args, uint32_t* index)
{
	const uint32_t num_tokens = args->n;
	const gdscript_id_t* ids = args->ids;
	for (int i = *index; i < num_tokens; ++i)
	{
		const gdscript_id_t id = ids[i];
		switch (id)
		{
		case GDSCRIPT_LEADING_WHITESPACE:
			args->req_lws++;
			break;
		case GDSCRIPT_LINE_END:
			*index = i;
			return args->req_lws;
		case GDSCRIPT_SEMICOLON:
			*index = i;
			process_line(parent, args, index, 0);
			return req_lws;
	#define X(ENUM, FUNC) case GDSCRIPT_##ENUM: parse_##FUNC(parent, args, index, 0); break;
			LINE_CASES
				#undef X
		default:
			parse_error(parent, args, index);
		/* default: */
		/* 	parse_table[i - GDSCRIPT_ERROR](parent, args, index, 1); // NOTE: this is dumb just throw it into the switch MY GOD, YOU HAVE THE ENUMS & FUNCTIONS IN A MACRO--JUST POP EM IN */
		}
	}
}

static uint8_t process_tokens(ast_node_t* parent, parse_args_t* args, uint32_t* index)
{
	uint32_t lws = 0;
	const uint32_t num_tokens = args->n;
	const gdscript_id_t* ids = args->ids;
	for (uint32_t i = *index; i < num_tokens; ++i)
	{
		if (lws == args->req_lws)
		{
			args->req_lws = process_line(parent, args, &i) * (args->req_lws > 0);
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
			/* shallow_line_error(parent, args, &i); */
			/* lws = 0; */
		}
	}
	return 1;
}

ast_node_t* gdscript_ast_generate(free_list_t* fl, const token_t* tokens, gdscript_id_t* ids, const uint32_t num_tokens)
{
	ast_node_t* root = free_list_calloc(fl, sizeof *root);
	root->id = AST_ROOT;
	root->len = num_tokens;
	parse_args_t args =
	{
		.tokens = tokens,
		.ids = ids,
		.n = num_tokens,
		.fl=fl,
		.root = root
	};
	uint32_t index = 0;
	process_tokens(root, &args, &index, 0);
	return args.root;
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
