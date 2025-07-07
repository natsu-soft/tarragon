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
	X(";",       SEMICOLON) \
	X("\n",      LINE_END) \
	X("\\",      LINE_CONTINUE) \
	X("#",       COMMENT) \
	X(":",       COLON) \
	X(".",       PERIOD) \
	X(",",       COMMA) \
	X("->",      ARROW) \
	X("(",       PAREN_OPEN) \
	X(")",       PAREN_CLOSE) \
	X("{",       BRACE_OPEN) \
	X("}",       BRACE_CLOSE) \
	X("[",       BRACKET_OPEN) \
	X("]",       BRACKET_CLOSE)

	/* GDSCRIPT_X("r'''",    GDSCRIPT_RAW_STRING_TRIPLE_QUOTE) \ */
	/* GDSCRIPT_X("r\"\"\"", GDSCRIPT_RAW_STRING_TRIPLE_DOUBLE_QUOTE) \ */
	/* GDSCRIPT_X("'''",     GDSCRIPT_STRING_TRIPLE_QUOTE) \ */
	/* GDSCRIPT_X("\"\"\"",  GDSCRIPT_STRING_TRIPLE_DOUBLE_QUOTE)\ */
	/* GDSCRIPT_X("r'",      GDSCRIPT_RAW_STRING_QUOTE) \ */
	/* GDSCRIPT_X("r\"",     GDSCRIPT_RAW_STRING_DOUBLE_QUOTE) \ */
	/* GDSCRIPT_X("'",       GDSCRIPT_STRING_QUOTE) \ */
	/* GDSCRIPT_X("\"",      GDSCRIPT_STRING_DOUBLE_QUOTE) */

// NOTE: pair, type, dot, identity, and negation operators never match during tokenization,
// but they're used during ast generation
// after tokenizatin remaining periods get converted to the dot operator


#define GDSCRIPT_BINARY_OPERATORS \
	X("**=", OP_ASSIGNMENT_POWER) \
	X("**",  OP_POWER) \
	X("*=",  OP_ASSIGNMENT_PRODUCT) \
	X("*",   OP_PRODUCT) \
	X("+=",  OP_ASSIGNMENT_SUM) \
	X("+",   OP_SUM) \
	X("-=",  OP_ASSIGNMENT_DIFFERENCE) \
	X("-",   OP_DIFFERENCE) \
	X("/=",  OP_ASSIGNMENT_DIVISION) \
	X("/",   OP_DIVISION) \
	X("%=",  OP_ASSIGNMENT_MODULO) \
	X("%",   OP_MODULO) \
	X("&&",  OP_BOOL_AND) \
	X("&=",  OP_ASSIGNMENT_BITWISE_AND) \
	X("&",   OP_BITWISE_AND) \
	X("||",  OP_BOOL_OR) \
	X("|=",  OP_ASSIGNMENT_BITWISE_OR) \
	X("|",   OP_BITWISE_OR) \
	X("<<=", OP_ASSIGNMENT_BITSHIFT_LEFT) \
	X("<<",  OP_BITSHIFT_LEFT) \
	X(">>=", OP_ASSIGNMENT_BITSHIFT_RIGH) \
	X(">>",  OP_BITSHIFT_RIGHT) \
	X("^=",  OP_ASSIGNMENT_XOR) \
	X("^",   OP_BITWISE_XOR) \
	X("==",  OP_EQUALS) \
	X("!=",  OP_INEQUALS) \
	X("<=",  OP_LESS_THAN_EQUALS) \
	X("<",   OP_LESS_THAN) \
	X(">=",  OP_GREATER_THAN_EQUALS) \
	X(">",   OP_GREATER_THAN) \
	X(":=",  OP_ASSIGNMENT_INFERRED) \
	X("=",   OP_ASSIGNMENT)
	/* X(":",   OP_TYPE) \ */
	/* X(".",   OP_DOT) */
	/* X(":",   OP_PAIR) \ */

#define GDSCRIPT_UNARY_OPERATORS \
	X("~", OP_BITWISE_NOT) \
	X("+", OP_IDENTITY) \
	X("-", OP_NEGATION) \
	X("!", OP_BOOL_NOT)

#define NUM_ASSIGNMENT_OPERATORS 13
#define GDSCRIPT_ASSIGNMENT_OPERATORS \
	X(OP_ASSIGNMENT_POWER) \
	X(OP_ASSIGNMENT_PRODUCT) \
	X(OP_ASSIGNMENT_SUM) \
	X(OP_ASSIGNMENT_DIFFERENCE) \
	X(OP_ASSIGNMENT_DIVISION) \
	X(OP_ASSIGNMENT_MODULO) \
	X(OP_ASSIGNMENT_BITWISE_AND) \
	X(OP_ASSIGNMENT_BITWISE_OR) \
	X(OP_ASSIGNMENT_BITSHIFT_LEFT) \
	X(OP_ASSIGNMENT_BITSHIFT_RIGH) \
	X(OP_ASSIGNMENT_XOR) \
	X(GDSCRIPT_OP_ASSIGNMENT_INFERRED) \
	X(OP_ASSIGNMENT)

#define GDSCRIPT_ANNOTATIONS \
	X("@export_category",            EXPORT_CATEGORY) \
	X("@export_color_no_alpha",      EXPORT_COLOR_NO_ALPHA) \
	X("@export_custom",              EXPORT_CUSTOM) \
	X("@export_dir",                 EXPORT_DIR) \
	X("@export_enum",                EXPORT_ENUM) \
	X("@export_exp_easing",          EXPORT_EXP_EASING) \
	X("@export_file",                EXPORT_FILE) \
	X("@export_flags_2d_navigation", EXPORT_FLAGS_2D_NAVIGATION) \
	X("@export_flags_2d_physics",    EXPORT_FLAGS_2D_PHYSICS) \
	X("@export_flags_2d_render",     EXPORT_FLAGS_2D_RENDER) \
	X("@export_flags_3d_navigation", EXPORT_FLAGS_3D_NAVIGATION) \
	X("@export_flags_3d_physics",    EXPORT_FLAGS_3D_PHYSICS) \
	X("@export_flags_3d_render",     EXPORT_FLAGS_3D_RENDER) \
	X("@export_flags_avoidance",     EXPORT_FLAGS_AVOIDANCE) \
	X("@export_flags",               EXPORT_FLAGS) \
	X("@export_global_dir",          EXPORT_GLOBAL_DIR) \
	X("@export_global_file",         EXPORT_GLOBAL_FILE) \
	X("@export_group",               EXPORT_GROUP) \
	X("@export_multiline",           EXPORT_MULTILINE) \
	X("@export_node_path",           EXPORT_NODE_PATH) \
	X("@export_placeholder",         EXPORT_PLACEHOLDER) \
	X("@export_range",               EXPORT_RANGE) \
	X("@export_storage",             EXPORT_STORAGE) \
	X("@export_subgroup",            EXPORT_SUBGROUP) \
	X("@export_tool_button",         EXPORT_TOOL_BUTTON) \
	X("@export",                     EXPORT) \
	X("@icon",                       ICON) \
	X("@onready",                    ONREADY) \
	X("@rpc",                        RPC) \
	X("@static_unload",              STATIC_UNLOAD) \
	X("@tool",                       TOOL) \
	X("@warning_ignore_restore",     WARNING_IGNORE_RESTORE) \
	X("@warning_ignore_stop",        WARNING_IGNORE_STOP) \
	X("@warning_ignore",             WARNING_IGNORE)

#define GDSCRIPT_KEYWORDS \
	X("if",         IF)	  \
	X("elif",       ELIF) \
	X("else",       ELSE) \
	X("for",        FOR) \
	X("while",      WHILE) \
	X("match",      MATCH) \
	X("break",      BREAK) \
	X("continue",   CONTINUE) \
	X("pass",       PASS) \
	X("return",     RETURN) \
	X("class_name", CLASS_NAME) \
	X("class",      CLASS) \
	X("extends",    EXTENDS) \
	X("is",         IS) \
	X("in",         IN) \
	X("as",         AS) \
	X("self",       SELF) \
	X("super",      SUPER) \
	X("signal",     SIGNAL) \
	X("func",       FUNC) \
	X("static",     STATIC) \
	X("const",      CONST) \
	X("enum",       ENUM) \
	X("var",        VAR) \
	X("breakpoint", BREAKPOINT) \
	X("preload",    PRELOAD) \
	X("await",      AWAIT) \
	X("yield",      YIELD) \
	X("assert",     ASSERT) \
	X("get",        GET) \
	X("set",        SET) \
	X("void",       VOID)

#define GDSCRIPT_LITERALS \
	X("null",  NULL)  \
	X("true",  TRUE)  \
	X("false", FALSE) \
	X("PI",    PI) \
	X("TAU",   TAU) \
	X("INF",   INF) \
	X("NAN",   NAN)

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

typedef enum __attribute__((packed)) gdscript_id_t
/* typedef enum gdscript_id_t */
{
	GDSCRIPT_NONE = 0, // really tho?
	#define X(_, ID) GDSCRIPT_##ID,
	GDSCRIPT_BUILTIN_TOKENS
	/* GDSCRIPT_STRUCTURE */
	/* GDSCRIPT_OPERATORS_START, */
	/* GDSCRIPT_ASSIGNMENT_OPERATORS */
	/* GDSCRIPT_UNARY_OPERATORS */
	/* GDSCRIPT_BINARY_OPERATORS */
	/* GDSCRIPT_OPERATORS */
	/* GDSCRIPT_OPERATORS_END, */
	/* GDSCRIPT_ANNOTATIONS */
	/* GDSCRIPT_KEYWORDS_START, */
	/* GDSCRIPT_KEYWORDS */
	/* GDSCRIPT_KEYWORDS_END, */
	/* GDSCRIPT_LITERALS_START, */
	/* GDSCRIPT_LITERALS */
	/* GDSCRIPT_LITERALS_END, */
	#undef X
	GDSCRIPT_NUM_BUILTIN,
	GDSCRIPT_ERROR,
	GDSCRIPT_STRING,
	GDSCRIPT_NUMBER,
	GDSCRIPT_IDENTIFIER,
	GDSCRIPT_PARSE_TABLE_SIZE, // NOTE: wtf is this for?
	GDSCRIPT_EOF, // NOTE-> USED, BUT DOESN'T HAVE TO BE
	GDSCRIPT_LEADING_WHITESPACE, // gets trimmed out
	GDSCRIPT_WHITESPACE, // also gets trimmed out
	GDSCRIPT_NUMBER_CANDIDATE,
	GDSCRIPT_SCOPE_START,
	GDSCRIPT_SCOPE_END,
	GDSCRIPT_ARGS_START,
	GDSCRIPT_ARGS_END,
} gdscript_id_t;

typedef struct token_table_t
{
	char* src;
	int16_t* offsets;
	int16_t* lens;
	gdscript_id_t* ids;
	int32_t num_tokens;
} token_table_t;

int16_t token_line(const token_table_t* table, const int32_t index);

int16_t token_column(const token_table_t* table, const int32_t index);

void print_tokens(const char* src, const int16_t* offsets, const int16_t* lens, const int32_t n);

void print_allman(const char* src, const int16_t* offsets, const int16_t* lens, const gdscript_id_t* ids, const int32_t num_tokens);

void print_clarity(const char* src, const int16_t* offsets, const int16_t* lens, const gdscript_id_t* ids, const int32_t num_tokens);


#endif /* End GDSCRIPT_PARSER_H */
