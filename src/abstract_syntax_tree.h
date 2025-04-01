#ifndef ABSTRACT_SYNTAX_TREE_H
#define ABSTRACT_SYNTAX_TREE_H

#include <stdint.h>

#define AST_EXPRESSIONS \
	X(AST_CONST, constant) \
	X(AST_GROUP_EXPR, group_expr) \
	X(AST_UNEXP, unary_expr) \
	X(AST_BINEXP, binary_expr) \
	X(AST_FUNC_EXPR, func_expr) \
	X(AST_ARRAY_EXPR, array_expr) \

#define AST_DECLARATIONS \
	X(AST_DECL_VAR, var_decl) \
	X(AST_DECL_FUNC, func_decl) \
	X(AST_DECL_ENUM, enum_decl) \
	X(AST_DECL_SIGNAL, signal_decl) \
	X(AST_DECL_CLASS, class_decl)

#define AST_NODES \
	X(AST_IDEINTIFIER, identifier) \
	X(AST_FUNC_CALL, func_call) \
	AST_EXPRESSIONS \
	AST_DECLARATIONS \


// forward declare everything
#define X(ENUM, NAME) typedef struct ast_##NAME##_t ast_##NAME##_t;
AST_NODES
#undef X

typedef union ast_expr_t ast_expr_t;

typedef struct ast_identifier_t
{
	struct token_t* token;
} ast_identifier_t;

typedef struct ast_func_call_t
{
	struct token_t* name;
	ast_expr_t * args;
	uint32_t num_args;
} ast_func_call_t;

/***************/
/* EXPRESSIONS */
/***************/


typedef struct ast_constant_t
{
	ast_identifier_t* identifier;
} ast_constant_t;

typedef struct ast_func_expr_t
{
	ast_func_call_t* func_call;
} ast_func_expr_t;

typedef struct ast_array_t
{
	ast_expr_t* items;
	uint32_t num_items;
} ast_array_t;

typedef struct ast_binary_expr_t
{
	gdscript_id_t operation;
	ast_expr_t* left_arg;
	ast_expr_t* right_arg;
} ast_binary_expr_t;

typedef struct ast_unary_expr_t
{
	gdscript_id_t operation;
	ast_expr_t* arg;
} ast_unary_expr_t;

typedef struct ast_group_expr_t
{
	ast_expr_t* expr;
} ast_group_expr_t;


/****************/
/* DECLARATIONS */
/****************/

typedef struct ast_var_decl_t
{

} ast_var_decl_t;

typedef union ast_expr_t
{
	union
	{
		#define X(ENUM, NAME) ast_##NAME##_t NAME;
		AST_EXPRESSIONS
		#undef X
	} expression;
	enum
	{
		#define X(ENUM, NAME) ENUM,
		AST_EXPRESSIONS
		#undef X
	} id;
} ast_expr_t;




typedef struct ast_node_t
{
	union
	{
		#define X(ENUM, NAME) ast_##NAME##_t NAME;
		AST_NODES
		#undef X
		/* } node __attribute__((__transparent_union__)); */ // NOTE: I PROBABLY want this
	} node;
	enum
	{
		#define X(ENUM, NAME) ENUM,
		AST_NODES
		#undef X
	} id;
} ast_node_t;

#endif /* End ABSTRACT_SYNTAX_TREE_H */
