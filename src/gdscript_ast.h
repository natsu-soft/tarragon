#ifndef GDSCRIPT_AST_H
#define GDSCRIPT_AST_H

#include <stddef.h>
#include <stdint.h>
#include "gdscript_parser.h"

typedef struct free_list_t free_list_t;
typedef struct token_t token_t;

#define TODO_EXPR_TYPE /* TODO: expressions should include what type they evaluate to */

/* #define X_EXPRESSIONS	   \ */
/* 	X(EXPR_CONST, constant) \ */
/* 	X(EXPR_GROUP, group_expr) \ */
/* 	X(EXPR_UNARY, unary_expr) \ */
/* 	X(EXPR_BINARY, binary_expr) \ */
/* 	X(EXPR_FUNC, func_expr) \ */
/* 	X(EXPR_ARRAY, array_expr) \ */


/* NOTE: NO SYMBOLS IN AST--JUST IDENTIFIERS */
#define NODES \
	X(ERROR,		0x0100) \
	X(ROOT,  		0x0200) \
	X(IDENTIFIER,	0x0400) \

// NOTE: symbolic expressions are expressions that are a SINGLE SYMBOL(token).
// can be variables, strings, numbers, or literals.
#define EXPRESSIONS								\
	X(SYMBOLIC)								\
	X(UNARY)								\
	X(BINARY)								\
	X(FUNC)									\
	X(ARRAY)								\
	X(DICT)									\
	X(GROUP)								\
	X(TERNARY) \
	X(PARAM)

#define STATEMENTS							\
	X(IF)										\
	X(ELIF)									\
	X(ELSE)									\
	X(FOR)									\
	X(WHILE)						    		\
	X(MATCH)								  \
	X(PATTERN)								\
	X(SIGNATURE)							\
	X(GET)									\
	X(SET)									\
	X(RETURN)								\
	X(BREAKPOINT)\
	X(BREAK) \
	X(CONTINUE) \
	X(PASS)

// parameters in signature statement
	/* X(PARAMETERS)							\ */

// NOTE: decalarations are also statements
#define DECLARATIONS	\
	X(CLASS)		\
	X(EXTENDS)		\
	X(VAR)			\
	X(CONST)		\
	X(FUNC)			\
	X(ENUM)			\
	X(SIGNAL)		\
	X(SUBCLASS)		\
	X(STATIC)

// NOTE: warnings appear inside of method bodies
#define ANNOTATIONS	\
	X(CLASS)	\
	X(VAR)		\
	X(FUNC)		\
	X(WARN)

typedef enum __attribute__((packed)) ast_id_t
{
	#define X(ID, VALUE) AST_##ID = VALUE,
	NODES
	#undef X
	/* EXPRESSIONS */
	#define X(ID) AST_EXPR_##ID,
	AST_EXPRESSION = 0x0800,
	EXPRESSIONS
	AST_EXPRESSION_END,
	#undef X
	#define X(ID) AST_STMT_##ID,
	AST_STATEMENT = 0x1000,
	STATEMENTS
	AST_STATEMENT_END,
	#undef X
	#define X(ID) AST_DECL_##ID,
	AST_DECLARATION = 0x2000,
	DECLARATIONS
	AST_DECLARTION_END,
	#undef X
	#define X(ID) AST_ANNO_##ID,
	AST_ANNOTATION = 0x4000,
	ANNOTATIONS
	AST_ANNOTATION_END,
	#undef X
} ast_id_t;


typedef enum __attribute__((packed)) ast_symbol_t
{
	SYMBOL_NONE		 = 0x00,
	SYMBOL_IDENTIFIER = 0x01,
	SYMBOL_STRING	 = 0x02,
	SYMBOL_NUMBER	 = 0x04,
	SYMBOL_LITERAL	 = 0x08,
} ast_symbol_t;

/* typedef enum __attribute__((packed)) ast_op_t */
/* { */
/* 	OP_NONE				   = 0x00000000, */
/* 	OP_ASSIGNMENT		   = 0x80000000, */
/* 	OP_POWER			   = 0x00000001, */
/* 	OP_PRODUCT			   = 0x00000002, */
/* 	OP_SUM				   = 0x00000004, */
/* 	OP_IDENTITY			   = 0x00000008, */
/* 	OP_DIFFERENCE		   = 0x00000010, */
/* 	OP_NEGATION   		   = 0x00000020, */
/* 	OP_DIVISION			   = 0x00000040, */
/* 	OP_MODULO			   = 0x00000080, */
/* 	OP_BOOL_AND			   = 0x00000100, */
/* 	OP_BOOL_OR			   = 0x00000200, */
/* 	OP_BOOL_NOT			   = 0x00000400, */
/* 	OP_BITSHIFT_LEFT	   = 0x00000800, */
/* 	OP_BITSHIFT_RIGHT	   = 0x00001000, */
/* 	OP_BITWISE_AND		   = 0x00002000, */
/* 	OP_BITWISE_OR		   = 0x00004000, */
/* 	OP_BITWISE_XOR		   = 0x00008000, */
/* 	OP_BITWISE_NOT		   = 0x00010000,	// doesn't assign--wtf? */
/* 	OP_TYPE				   = 0x00020000,	// combines with assignment to make inferred (type) assignment */
/* 	// these operators do NOT compare with assignment */
/* 	OP_EQUALS			   = 0x00040000, */
/* 	OP_INEQUALS			   = 0x00080000, */
/* 	OP_LESS_THAN_EQUALS	   = 0x00100000, */
/* 	OP_LESS_THAN		   = 0x00200000, */
/* 	OP_GREATER_THAN_EQUALS = 0x00400000, */
/* 	OP_GREATER_THAN		   = 0x00800000, */
/* 	OP_PAIR				   = 0x01000000,	// used in dicts.  Either : or = if a lua-styled dict. */
/* 	OP_DOT				   = 0x02000000, */
/* } ast_op_t; */

typedef enum	__attribute__((packed)) ast_op_t
{
	OP_NONE					= 0x00,
	OP_ASSIGNMENT			= 0x80,
	OP_POWER				= 0x01,
	OP_PRODUCT			    ,
	OP_SUM				    ,
	OP_IDENTITY			    ,
	OP_DIFFERENCE		    ,
	OP_NEGATION   		    ,
	OP_DIVISION			    ,
	OP_MODULO			    ,
	OP_BOOL_AND			    ,
	OP_BOOL_OR			    ,
	OP_BOOL_NOT			    ,
	OP_BITSHIFT_LEFT	    ,
	OP_BITSHIFT_RIGHT	    ,
	OP_BITWISE_AND		    ,
	OP_BITWISE_OR		    ,
	OP_BITWISE_XOR		    ,
	OP_BITWISE_NOT		    , //	doesn't assign--wtf?
	OP_TYPE				    , //	combines with assignment to make inferred(type) assignment
	// these operators do NOT compare with assignment
	OP_EQUALS				,
	OP_INEQUALS				,
	OP_LESS_THAN_EQUALS		,
	OP_LESS_THAN			,
	OP_GREATER_THAN_EQUALS	,
	OP_GREATER_THAN			,
	OP_PAIR					, // used in dicts. Either : or = if a lua-styled dict.
	OP_DOT					,
}	ast_op_t;

typedef enum __attribute__((packed)) ast_annotation_t
{
	#define X(_, ENUM) AST_##ENUM,
	GDSCRIPT_ANNOTATIONS
	#undef X
} ast_annotation_t;

typedef struct ast_node_t
{
	union {
		// for errors
		struct
		{
			struct token_t* err_token;
			size_t err_len;
		};
		// for regular nodes
		struct
		{
			struct ast_node_t* children;
			size_t num_children;
		};
		// for operator expressions
		struct
		{
			struct ast_node_t* lhs;
			struct ast_node_t* rhs;
		};
		struct ast_node_t* operand[2]; // for unary, can refer to with just operand->
		token_t* symbol;
	};
	struct ast_node_t* parent;
	union
	{
		struct {
			TODO_EXPR_TYPE // 7 bytes to play with WOW!
			ast_op_t operation;
		};
		struct ast_node_t* signature_return; // the return type of a function signature
		gdscript_id_t token_id;
		/* struct ast_node_t* expr_conditional; // DISGUSTING */
		ast_annotation_t annotation_id;
		ast_symbol_t symbolic_id;
		uint8_t lua_dict; // used with AST_EXPR_DICT
	};
	/* uint32_t pos; // i dont think I need this */
	/* uint32_t len; // i MITE need */
	ast_id_t id;
	/* ast_id_t last_child_id; // you shouldn't need this but ur lazy tbqh */
} ast_node_t;

ast_node_t* gdscript_ast_generate(free_list_t* fl, const token_t* tokens, gdscript_id_t* ids, const uint32_t num_tokens);

void gdscript_ast_check(ast_node_t* root);

/* void gdscript_ast_check_expression_types(ast_node_t* root); */

#endif /* End GDSCRIPT_AST_H */
