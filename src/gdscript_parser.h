#ifndef GDSCRIPT_PARSER_H
#define GDSCRIPT_PARSER_H

#include <stdint.h>

/* #define GDSCRIPT_TRIPLE_STRING_DELIMITERS \ */
/* 	GDSCRIPT_X("'''",    GDSCRIPT_TRIPLE_QUOTE) \ */
/* 	GDSCRIPT_X("\"\"\"", GDSCRIPT_TRIPLE_DOUBLE_QUOTE) */

/* #define GDSCRIPT_STRING_DELIMITERS \ */
/* 	GDSCRIPT_X("'",      GDSCRIPT_QUOTE) \ */
/* 	GDSCRIPT_X("\"",     GDSCRIPT_DOUBLE_QUOTE) */

/* #define GDSCRIPT_RAW_TRIPLE_STRING_STARTERS \ */
/* 	GDSCRIPT_X("r'''",    GDSCRIPT_RAW_TRIPLE_QUOTE) \ */
/* 	GDSCRIPT_X("r\"\"\"", GDSCRIPT_RAW_TRIPLE_DOUBLE_QUOTE) */

/* #define GDSCRIPT_RAW_STRING_STARTERS \ */
/* 	GDSCRIPT_X("r'",      GDSCRIPT_RAW_QUOTE) \ */
/* 	GDSCRIPT_X("r\"",     GDSCRIPT_RAW_DOUBLE_QUOTE) */

/* #define GDSCRIPT_TRIPLE_STRING_STARTERS \ */
/* 	GDSCRIPT_RAW_STRING_STARTERS \ */
/* 	GDSCRIPT_TRIPLE_STRING_DELIMIERS */

/* #define GDSCRIPT_STRING_STARTERS \ */
/* 	GDSCRIPT_RAW_STRING_STARTERS \ */
/* 	GDSCRIPT_STRING_DELIMITERS */

/* #define GDSCRIPT_DELETE_THIS \ */
/* 	GDSCRIPT_X("@",       GDSCRIPT_ANNOTATION) \ */

#define GDSCRIPT_STRUCTURE \
	GDSCRIPT_X("\n",      GDSCRIPT_LINE_END) \
	GDSCRIPT_X("\\",      GDSCRIPT_LINE_CONTINUE) \
	GDSCRIPT_X("#",       GDSCRIPT_COMMENT) \
	GDSCRIPT_X(":",       GDSCRIPT_COLON) \
	GDSCRIPT_X(";",       GDSCRIPT_SEMICOLON) \
	GDSCRIPT_X(".",       GDSCRIPT_PERIOD) \
	GDSCRIPT_X(",",       GDSCRIPT_COMMA) \
	GDSCRIPT_X("->",      GDSCRIPT_ARROW) \
	GDSCRIPT_X("(",       GDSCRIPT_PAREN_OPEN) \
	GDSCRIPT_X(")",       GDSCRIPT_PAREN_CLOSE) \
	GDSCRIPT_X("{",       GDSCRIPT_BRACE_OPEN) \
	GDSCRIPT_X("}",       GDSCRIPT_BRACE_CLOSE) \
	GDSCRIPT_X("[",       GDSCRIPT_BRACKET_OPEN) \
	GDSCRIPT_X("]",       GDSCRIPT_BRACKET_CLOSE)

	/* GDSCRIPT_X("r'''",    GDSCRIPT_RAW_STRING_TRIPLE_QUOTE) \ */
	/* GDSCRIPT_X("r\"\"\"", GDSCRIPT_RAW_STRING_TRIPLE_DOUBLE_QUOTE) \ */
	/* GDSCRIPT_X("'''",     GDSCRIPT_STRING_TRIPLE_QUOTE) \ */
	/* GDSCRIPT_X("\"\"\"",  GDSCRIPT_STRING_TRIPLE_DOUBLE_QUOTE)\ */
	/* GDSCRIPT_X("r'",      GDSCRIPT_RAW_STRING_QUOTE) \ */
	/* GDSCRIPT_X("r\"",     GDSCRIPT_RAW_STRING_DOUBLE_QUOTE) \ */
	/* GDSCRIPT_X("'",       GDSCRIPT_STRING_QUOTE) \ */
	/* GDSCRIPT_X("\"",      GDSCRIPT_STRING_DOUBLE_QUOTE) */

#define GDSCRIPT_BINARY_OPERATORS \
	GDSCRIPT_X("**=", GDSCRIPT_OP_ASSIGNMENT_POWER) \
	GDSCRIPT_X("**",  GDSCRIPT_OP_POWER) \
	GDSCRIPT_X("*=",  GDSCRIPT_OP_ASSIGNMENT_PRODUCT) \
	GDSCRIPT_X("*",   GDSCRIPT_OP_MULTIPLY) \
	GDSCRIPT_X("+=",  GDSCRIPT_OP_ASSIGNMENT_SUM) \
	GDSCRIPT_X("+",   GDSCRIPT_OP_SUM) \
	GDSCRIPT_X("-=",  GDSCRIPT_OP_ASSIGNMENT_DIFFERENCE) \
	GDSCRIPT_X("-",   GDSCRIPT_OP_DIFFERENCE) \
	GDSCRIPT_X("/=",  GDSCRIPT_OP_ASSIGNMENT_DIVISION) \
	GDSCRIPT_X("/",   GDSCRIPT_OP_DIVISION) \
	GDSCRIPT_X("%=",  GDSCRIPT_OP_ASSIGNMENT_MODULO) \
	GDSCRIPT_X("%",   GDSCRIPT_OP_MODULO) \
	GDSCRIPT_X("&&",  GDSCRIPT_OP_BOOL_AND) \
	GDSCRIPT_X("&=",  GDSCRIPT_OP_ASSIGNMENT_BITWISE_AND) \
	GDSCRIPT_X("&",   GDSCRIPT_OP_BITWISE_AND) \
	GDSCRIPT_X("||",  GDSCRIPT_OP_BOOL_OR) \
	GDSCRIPT_X("|=",  GDSCRIPT_OP_ASSIGNMENT_BITWISE_OR) \
	GDSCRIPT_X("|",   GDSCRIPT_OP_BITWISE_OR) \
	GDSCRIPT_X("<<=", GDSCRIPT_OP_ASSIGNMENT_BITSHIFT_LEFT) \
	GDSCRIPT_X("<<",  GDSCRIPT_OP_BITSHIFT_LEFT) \
	GDSCRIPT_X(">>=", GDSCRIPT_OP_ASSIGNMENT_BITSHIFT_RIGH) \
	GDSCRIPT_X(">>",  GDSCRIPT_OP_BITSHIFT_RIGHT) \
	GDSCRIPT_X("^=",  GDSCRIPT_OP_ASSIGNMENT_XOR) \
	GDSCRIPT_X("^",   GDSCRIPT_OP_BITWISE_XOR) \
	GDSCRIPT_X("==",  GDSCRIPT_OP_EQUALS) \
	GDSCRIPT_X("!=",  GDSCRIPT_OP_INEQUALS) \
	GDSCRIPT_X("<=",  GDSCRIPT_OP_LESS_THAN_EQUALS) \
	GDSCRIPT_X("<",   GDSCRIPT_OP_LESS_THAN) \
	GDSCRIPT_X(">=",  GDSCRIPT_OP_GREATER_THAN_EQUALS) \
	GDSCRIPT_X(">",   GDSCRIPT_OP_GREATER_THAN) \
	GDSCRIPT_X("=",   GDSCRIPT_OP_ASSIGNMENT)

#define GDSCRIPT_UNARY_OPERATORS \
	GDSCRIPT_X("~", GDSCRIPT_OP_BITWISE_NOT) \
	GDSCRIPT_X("+", GDSCRIPT_OP_IDENTITY) \
	GDSCRIPT_X("-", GDSCRIPT_OP_NEGATION) \
	GDSCRIPT_X("!", GDSCRIPT_OP_BOOL_NOT)

#define GDSCRIPT_ANNOTATIONS \
	GDSCRIPT_X("@export_category",            GDSCRIPT_EXPORT_CATEGORY) \
	GDSCRIPT_X("@export_color_no_alpha",      GDSCRIPT_EXPORT_COLOR_NO_ALPHA) \
	GDSCRIPT_X("@export_custom",              GDSCRIPT_EXPORT_CUSTOM) \
	GDSCRIPT_X("@export_dir",                 GDSCRIPT_EXPORT_DIR) \
	GDSCRIPT_X("@export_enum",                GDSCRIPT_EXPORT_ENUM) \
	GDSCRIPT_X("@export_exp_easing",          GDSCRIPT_EXPORT_EXP_EASING) \
	GDSCRIPT_X("@export_file",                GDSCRIPT_EXPORT_FILE) \
	GDSCRIPT_X("@export_flags_2d_navigation", GDSCRIPT_EXPORT_FLAGS_2D_NAVIGATION) \
	GDSCRIPT_X("@export_flags_2d_physics",    GDSCRIPT_EXPORT_FLAGS_2D_PHYSICS) \
	GDSCRIPT_X("@export_flags_2d_render",     GDSCRIPT_EXPORT_FLAGS_2D_RENDER) \
	GDSCRIPT_X("@export_flags_3d_navigation", GDSCRIPT_EXPORT_FLAGS_3D_NAVIGATION) \
	GDSCRIPT_X("@export_flags_3d_physics",    GDSCRIPT_EXPORT_FLAGS_3D_PHYSICS) \
	GDSCRIPT_X("@export_flags_3d_render",     GDSCRIPT_EXPORT_FLAGS_3D_RENDER) \
	GDSCRIPT_X("@export_flags_avoidance",     GDSCRIPT_EXPORT_FLAGS_AVOIDANCE) \
	GDSCRIPT_X("@export_flags",               GDSCRIPT_EXPORT_FLAGS) \
	GDSCRIPT_X("@export_global_dir",          GDSCRIPT_EXPORT_GLOBAL_DIR) \
	GDSCRIPT_X("@export_global_file",         GDSCRIPT_EXPORT_GLOBAL_FILE) \
	GDSCRIPT_X("@export_group",               GDSCRIPT_EXPORT_GROUP) \
	GDSCRIPT_X("@export_multiline",           GDSCRIPT_EXPORT_MULTILINE) \
	GDSCRIPT_X("@export_node_path",           GDSCRIPT_EXPORT_NODE_PATH) \
	GDSCRIPT_X("@export_placeholder",         GDSCRIPT_EXPORT_PLACEHOLDER) \
	GDSCRIPT_X("@export_range",               GDSCRIPT_EXPORT_RANGE) \
	GDSCRIPT_X("@export_storage",             GDSCRIPT_EXPORT_STORAGE) \
	GDSCRIPT_X("@export_subgroup",            GDSCRIPT_EXPORT_SUBGROUP) \
	GDSCRIPT_X("@export_tool_button",         GDSCRIPT_EXPORT_TOOL_BUTTON) \
	GDSCRIPT_X("@export",                     GDSCRIPT_EXPORT) \
	GDSCRIPT_X("@icon",                       GDSCRIPT_ICON) \
	GDSCRIPT_X("@onready",                    GDSCRIPT_ONREADY) \
	GDSCRIPT_X("@rpc",                        GDSCRIPT_RPC) \
	GDSCRIPT_X("@static_unload",              GDSCRIPT_STATIC_UNLOAD) \
	GDSCRIPT_X("@tool",                       GDSCRIPT_TOOL) \
	GDSCRIPT_X("@warning_ignore_restore",     GDSCRIPT_WARNING_IGNORE_RESTORE) \
	GDSCRIPT_X("@warning_ignore_stop",        GDSCRIPT_WARNING_IGNORE_STOP) \
	GDSCRIPT_X("@warning_ignore",             GDSCRIPT_WARNING_IGNORE)

#define GDSCRIPT_KEYWORDS \
	GDSCRIPT_X("if",         GDSCRIPT_KEYWORD_IF)	  \
	GDSCRIPT_X("elif",       GDSCRIPT_KEYWORD_ELIF) \
	GDSCRIPT_X("else",       GDSCRIPT_KEYWORD_ELSE) \
	GDSCRIPT_X("for",        GDSCRIPT_KEYWORD_FOR) \
	GDSCRIPT_X("while",      GDSCRIPT_KEYWORD_WHILE) \
	GDSCRIPT_X("match",      GDSCRIPT_KEYWORD_MATCH) \
	GDSCRIPT_X("break",      GDSCRIPT_KEYWORD_BREAK) \
	GDSCRIPT_X("continue",   GDSCRIPT_KEYWORD_CONTINUE) \
	GDSCRIPT_X("pass",       GDSCRIPT_KEYWORD_PASS) \
	GDSCRIPT_X("return",     GDSCRIPT_KEYWORD_RETURN) \
	GDSCRIPT_X("class_name", GDSCRIPT_KEYWORD_CLASS_NAME) \
	GDSCRIPT_X("class",      GDSCRIPT_KEYWORD_CLASS) \
	GDSCRIPT_X("extends",    GDSCRIPT_KEYWORD_EXTENDS) \
	GDSCRIPT_X("is",         GDSCRIPT_KEYWORD_IS) \
	GDSCRIPT_X("in",         GDSCRIPT_KEYWORD_IN) \
	GDSCRIPT_X("as",         GDSCRIPT_KEYWORD_AS) \
	GDSCRIPT_X("self",       GDSCRIPT_KEYWORD_SELF) \
	GDSCRIPT_X("super",      GDSCRIPT_KEYWORD_SUPER) \
	GDSCRIPT_X("signal",     GDSCRIPT_KEYWORD_SIGNAL) \
	GDSCRIPT_X("func",       GDSCRIPT_KEYWORD_FUNC) \
	GDSCRIPT_X("static",     GDSCRIPT_KEYWORD_STATIC) \
	GDSCRIPT_X("const",      GDSCRIPT_KEYWORD_CONST) \
	GDSCRIPT_X("enum",       GDSCRIPT_KEYWORD_ENUM) \
	GDSCRIPT_X("var",        GDSCRIPT_KEYWORD_VAR) \
	GDSCRIPT_X("breakpoint", GDSCRIPT_KEYWORD_BREAKPOINT) \
	GDSCRIPT_X("preload",    GDSCRIPT_KEYWORD_PRELOAD) \
	GDSCRIPT_X("await",      GDSCRIPT_KEYWORD_AWAIT) \
	GDSCRIPT_X("yield",      GDSCRIPT_KEYWORD_YIELD) \
	GDSCRIPT_X("assert",     GDSCRIPT_KEYWORD_ASSERT) \
	GDSCRIPT_X("get",       GDSCRIPT_KEYWORD_GET) \
	GDSCRIPT_X("set",       GDSCRIPT_KEYWORD_SET) \
	GDSCRIPT_X("void",       GDSCRIPT_KEYWORD_VOID)

#define KEYWORD(WORD) GDSCRIPT_KEYWORD_##WORD

#define GDSCRIPT_LITERALS \
	GDSCRIPT_X("null",  GDSCRIPT_NULL)  \
	GDSCRIPT_X("true",  GDSCRIPT_TRUE)  \
	GDSCRIPT_X("false", GDSCRIPT_FALSE) \
	GDSCRIPT_X("PI",    GDSCRIPT_PI) \
	GDSCRIPT_X("TAU",   GDSCRIPT_TAU) \
	GDSCRIPT_X("INF",   GDSCRIPT_INF) \
	GDSCRIPT_X("NAN",   GDSCRIPT_NAN)

#define GDSCRIPT_OPERATORS \
	GDSCRIPT_BINARY_OPERATORS \
	GDSCRIPT_UNARY_OPERATORS

#define GDSCRIPT_DELIMITING_TOKENS \
	GDSCRIPT_STRUCTURE \
	GDSCRIPT_OPERATORS

#define GDSCRIPT_BUILTIN_TOKENS \
	GDSCRIPT_STRUCTURE \
	GDSCRIPT_OPERATORS \
	GDSCRIPT_ANNOTATIONS \
	GDSCRIPT_KEYWORDS \
	GDSCRIPT_LITERALS

typedef enum gdscript_id_t
{
	GDSCRIPT_ERROR = -1,
	GDSCRIPT_NONE = 0, // really tho?
	#define GDSCRIPT_X(_, ID) ID,
	GDSCRIPT_BUILTIN_TOKENS
	#undef GDSCRIPT_X
	GDSCRIPT_BUILTIN_COUNT,
	GDSCRIPT_EOF,
	GDSCRIPT_LEADING_WHITESPACE,
	GDSCRIPT_WHITESPACE,
	GDSCRIPT_STRING,
	GDSCRIPT_NUMBER_CANDIDATE,
	GDSCRIPT_NUMBER,
	GDSCRIPT_IDENTIFIER,
	GDSCRIPT_SCOPE_START,
	GDSCRIPT_SCOPE_END,
} gdscript_id_t;

typedef enum syntax_id_t
{
	SYNTAX_ERROR = -1,
	SYNTAX_LITERAL,
	SYNTAX_BINARY,
	SYNTAX_UNARY,
	SYNTAX_CLASS_DECLARATION,
	SYNTAX_FUNCTION_DECLARATION,
	SYNTAX_VARIABLE_DECLARATION,
	SYNTAX_SIGNAL_DECLARATION,
	SYNTAX_ARGUMENT,
	SYNTAX_FUNCTION_CALL,
	SYNTAX_EXPRESSION = SYNTAX_LITERAL | SYNTAX_BINARY | SYNTAX_UNARY | SYNTAX_FUNCTION_CALL,
	SYNTAX_FUNCTION_ARGUMENTS,
	SYNTAX_CONDITION,
	SYNTAX_IF_ELIF_ELSE,
	SYNTAX_IF,
	SYNTAX_ELIF,
	SYNTAX_ELSE,
} syntax_id_t;

/* typedef struct syntax_t */
/* { */
/* 	syntax_id_t id; */
/* 	void* expr; */
/* } syntax_t; */

/* typedef struct decl_class_t */
/* { */
/* 	syntax_t* body; */
/* 	uint32_t num; */
/* } decl_class_t; */

/* typedef struct decl_var_t */
/* { */
/* 	syntax_t* name; */
/* 	char* type; // optional */
/* 	syntax_t* expression; */
/* } decl_var_t; */

/* typedef struct literal_t */
/* { */
/* 	syntax_id_t id; */
/* 	union */
/* 	{ */
/* 		struct literal_t* literal; */
/* 		token_t* token; */
/* 	}; */
/* } literal_t; */

/* typedef struct group_t */
/* { */
/* 	syntax_t* inside; */
/* } group_t; */

/* typedef struct unary_t */
/* { */
/* 	gdscript_id_t operation; */
/* 	literal_t* arg; */
/* } unary_t; */

/* typedef struct binary_t */
/* { */
/* 	gdscript_id_t operation; */
/* 	literal_t* lhs; */
/* 	literal_t* rhs; */
/* } binary_t; */

// bro, you're going to need a resizable free list


/* uint32_t gdscript_tokenize(arena_t* arena, token_t** tokens, gdscript_id_t** ids, const char* str, const uint32_t n); */

/* void gdscript_preprocess(token_t* tokens, gdscript_id_t* ids, uint32_t* num_tokens); */

/* void gdscript_note_token_func(void (*func_ptr)(const token_t*, const char* message)); */

/* void gdscript_warn_token_func(void (*func_ptr)(const token_t*, const char* message)); */

/* void gdscript_error_token_func(void (*func_ptr)(const token_t*, const char* message)); */

/* gdscript_syntax_id syntax_match(const match_arg_t* args, ); */


/* #define MULTIPLE(...) __VA_ARGS__ */

/* #define BODY(LWS_DEPTH) \ */
/* 	MULTIPLE(BINARY(MATCH_DEPTH(LWS_DEPTH), EXPRESSION)) */

#endif /* End GDSCRIPT_PARSER_H */
