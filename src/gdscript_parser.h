#ifndef GDSCRIPT_PARSER_H
#define GDSCRIPT_PARSER_H

#include <stdint.h>

#define GDSCRIPT_STRUCTURE		\
	X(SEMICOLON,	 ";",	1)	\
	X(LINE_END,		 "\n",	1)	\
	X(LINE_CONTINUE, "\\",	1)	\
	X(COMMENT,		 "#",	1)	\
	X(PERIOD,		 ".",	1)	\
	X(COMMA,		 ",",	1)	\
	X(ARROW,		 "->",	2)	\
	X(COLON,		 ":",	1)	\
	X(PAREN_CLOSE,	 ")",	1)	\
	X(BRACE_CLOSE,	 "}",	1)	\
	X(BRACKET_CLOSE, "]",	1)  \
	X(PAREN_OPEN,	 "(",	1)	\
	X(BRACE_OPEN,	 "{",	1)	\
	X(BRACKET_OPEN,	 "[",	1)

#define GDSCRIPT_BINARY_OPERATORS  					   \
	X(OP_ASSIGNMENT_POWER,		    "**=",			3) \
	X(OP_POWER,					    "**",			2) \
	X(OP_ASSIGNMENT_PRODUCT,	    "*=",			2) \
	X(OP_PRODUCT,				    "*",			1) \
	X(OP_ASSIGNMENT_SUM,		    "+=",			2) \
	X(OP_ASSIGNMENT_DIFFERENCE,	    "-=",			2) \
	X(OP_SUM,					    "+",			1) \
	X(OP_DIFFERENCE,			    "-",			1) \
	X(OP_ASSIGNMENT_DIVISION,	    "/=",			2) \
	X(OP_DIVISION,				    "/",			1) \
	X(OP_ASSIGNMENT_MODULO,		    "%=",			2) \
	X(OP_MODULO,				    "%",			1) \
	X(OP_BOOL_AND,				    "&&",			2) \
	X(OP_ASSIGNMENT_BITWISE_AND,    "&=",			2) \
	X(OP_BITWISE_AND,			    "&",			1) \
	X(OP_BOOL_OR,				    "||",			2) \
	X(OP_ASSIGNMENT_BITWISE_OR,	    "|=",			1) \
	X(OP_BITWISE_OR,				"|",			1) \
	X(OP_ASSIGNMENT_BITSHIFT_LEFT,	"<<=",			3) \
	X(OP_BITSHIFT_LEFT,				"<<",			2) \
	X(OP_ASSIGNMENT_BITSHIFT_RIGH,	">>=",			3) \
	X(OP_BITSHIFT_RIGHT,			">>",			2) \
	X(OP_ASSIGNMENT_XOR,			"^=",			2) \
	X(OP_BITWISE_XOR,				"^",			1) \
	X(OP_EQUALS,					"==",			2) \
	X(OP_INEQUALS,					"!=",			2) \
	X(OP_LESS_THAN_EQUALS,			"<=",			2) \
	X(OP_LESS_THAN,					"<",			1) \
	X(OP_GREATER_THAN_EQUALS,		">=",			2) \
	X(OP_GREATER_THAN,				">",			1) \
	X(OP_ASSIGNMENT_INFERRED,		":=",			2) \
	X(OP_ASSIGNMENT,			     "=",			1)

#define GDSCRIPT_UNARY_OPERATORS	\
	X(OP_BITWISE_NOT,	 "~",	1)	\
	X(OP_IDENTITY,		 "+",	1)	\
	X(OP_NEGATION,		 "-",	1)	\
	X(OP_BOOL_NOT,		 "!",	1)

#define NUM_ASSIGNMENT_OPERATORS 13
#define GDSCRIPT_ASSIGNMENT_OPERATORS \
	X(OP_ASSIGNMENT_POWER,		    "**=",			3) \
	X(OP_ASSIGNMENT_PRODUCT,	    "*=",			2) \
	X(OP_ASSIGNMENT_SUM,		    "+=",			2) \
	X(OP_ASSIGNMENT_DIFFERENCE,	    "-=",			2) \
	X(OP_ASSIGNMENT_DIVISION,	    "/=",			2) \
	X(OP_ASSIGNMENT_MODULO,		    "%=",			2) \
	X(OP_ASSIGNMENT_BITWISE_AND,    "&=",			2) \
	X(OP_ASSIGNMENT_BITWISE_OR,	    "|=",			1) \
	X(OP_ASSIGNMENT_BITSHIFT_LEFT,	"<<=",			3) \
	X(OP_ASSIGNMENT_BITSHIFT_RIGH,	">>=",			3) \
	X(OP_ASSIGNMENT_XOR,			"^=",			2) \
	X(OP_ASSIGNMENT_INFERRED,		":=",			2) \
	X(OP_ASSIGNMENT,			     "=",			1)

#define NUM_NON_ASSIGNMENT_OPERATORS 24
#define GDSCRIPT_NON_ASSIGNMENT_OPERATORS \
	X(OP_POWER,					    "**",			2) \
	X(OP_PRODUCT,				    "*",			1) \
	X(OP_SUM,					    "+",			1) \
	X(OP_DIFFERENCE,			    "-",			1) \
	X(OP_DIVISION,				    "/",			1) \
	X(OP_MODULO,				    "%",			1) \
	X(OP_BOOL_AND,				    "&&",			2) \
	X(OP_BITWISE_AND,			    "&",			1) \
	X(OP_BOOL_OR,				    "||",			2) \
	X(OP_BITWISE_OR,				"|",			1) \
	X(OP_BITSHIFT_LEFT,				"<<",			2) \
	X(OP_BITSHIFT_RIGHT,			">>",			2) \
	X(OP_BITWISE_XOR,				"^",			1) \
	X(OP_EQUALS,					"==",			2) \
	X(OP_INEQUALS,					"!=",			2) \
	X(OP_LESS_THAN_EQUALS,			"<=",			2) \
	X(OP_LESS_THAN,					"<",			1) \
	X(OP_GREATER_THAN_EQUALS,		">=",			2) \
	X(OP_GREATER_THAN,				">",			1) \
	X(OP_BITWISE_NOT,	 "~",	1)	\
	X(OP_IDENTITY,		 "+",	1)	\
	X(OP_NEGATION,		 "-",	1)	\
	X(OP_BOOL_NOT,		 "!",	1)

#define GDSCRIPT_ANNOTATIONS											\
	X(EXPORT_CATEGORY,				"@export_category",				16) \
	X(EXPORT_COLOR_NO_ALPHA,		"@export_color_no_alpha",		22) \
    X(EXPORT_CUSTOM,				"@export_custom",				14) \
	X(EXPORT_DIR,					"@export_dir",					11) \
	X(EXPORT_ENUM,					"@export_enum",					12) \
	X(EXPORT_EXP_EASING,			"@export_exp_easing",			18) \
	X(EXPORT_FILE,					"@export_file",					12) \
	X(EXPORT_FLAGS_2D_NAVIGATION,	"@export_flags_2d_navigation",	27) \
	X(EXPORT_FLAGS_2D_PHYSICS,		"@export_flags_2d_physics",		24) \
	X(EXPORT_FLAGS_2D_RENDER,		"@export_flags_2d_render",		23) \
	X(EXPORT_FLAGS_3D_NAVIGATION,	"@export_flags_3d_navigation",	27) \
	X(EXPORT_FLAGS_3D_PHYSICS,		"@export_flags_3d_physics",		24) \
	X(EXPORT_FLAGS_3D_RENDER,		"@export_flags_3d_render",		23) \
	X(EXPORT_FLAGS_AVOIDANCE,		"@export_flags_avoidance",		23) \
	X(EXPORT_FLAGS,					"@export_flags",				13) \
	X(EXPORT_GLOBAL_DIR,			"@export_global_dir",			18) \
	X(EXPORT_GLOBAL_FILE,			"@export_global_file",			19) \
	X(EXPORT_GROUP,					"@export_group",				13) \
	X(EXPORT_MULTILINE,				"@export_multiline",			17) \
	X(EXPORT_NODE_PATH,				"@export_node_path",			17) \
	X(EXPORT_PLACEHOLDER,			"@export_placeholder",			19) \
	X(EXPORT_RANGE,					"@export_range",				13) \
	X(EXPORT_STORAGE,				"@export_storage",				15) \
	X(EXPORT_SUBGROUP,				"@export_subgroup",				16) \
	X(EXPORT_TOOL_BUTTON,			"@export_tool_button",			18) \
	X(EXPORT,						"@export",						7) \
	X(ICON,							"@icon",						5) \
	X(ONREADY,						"@onready",						7) \
	X(RPC,							"@rpc",							4) \
	X(STATIC_UNLOAD,				"@static_unload",				14) \
	X(TOOL,							"@tool",						5) \
	X(WARNING_IGNORE_RESTORE,		"@warning_ignore_restore",		23) \
	X(WARNING_IGNORE_STOP,			"@warning_ignore_stop",			20) \
	X(WARNING_IGNORE,				"@warning_ignore",				15) \

#define GDSCRIPT_KEYWORDS \
	X(IF,         "if",				6) \
	X(ELIF,		  "elif",			4) \
	X(ELSE,		  "else",			4) \
	X(FOR,		  "for",			3) \
	X(WHILE,	  "while",			5) \
	X(MATCH,	  "match",			5) \
	X(BREAK,	  "break",			5) \
	X(CONTINUE,	  "continue",		8) \
	X(PASS,		  "pass",			4) \
	X(RETURN,	  "return",			6) \
	X(CLASS_NAME, "class_name",     10) \
	X(CLASS,	  "class",			5) \
	X(EXTENDS,	  "extends",		7) \
	X(IS,		  "is",				2) \
	X(IN,		  "in",				2) \
	X(AS,		  "as",				2) \
	X(SELF,		  "self",			4) \
	X(SUPER,	  "super",			5) \
	X(SIGNAL,	  "signal",			6) \
	X(FUNC,		  "func",		 	4) \
	X(STATIC,	  "static",			6) \
	X(CONST,	  "const",			5) \
	X(ENUM,		  "enum",		 	4) \
	X(VAR,		  "var",		 	3) \
	X(BREAKPOINT, "breakpoint", 	10) \
	X(PRELOAD,	  "preload",	 	7) \
	X(AWAIT,	  "await",			5) \
	X(YIELD,	  "yield",		 	5) \
	X(ASSERT,	  "assert",		 	6) \
	X(GET,		  "get",		 	3) \
	X(SET,		  "set",		 	3) \
	X(VOID,		  "void",		 	4)

#define GDSCRIPT_LITERALS						\
	X(NULL, "null",   4)							\
	X(TRUE, "true",   4)							\
	X(FALSE, "false", 5)							\
	X(PI,    "PI",    2)							\
	X(TAU,   "TAU",   3)							\
	X(INF,   "INF",   3)							\
	X(NAN,   "NAN",   3)

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
	GDSCRIPT_NONE = 0,
	#define X(ID, ...) GDSCRIPT_##ID,
	GDSCRIPT_BUILTIN_TOKENS
	#undef X
	GDSCRIPT_NUM_BUILTIN,
	GDSCRIPT_ERROR,
	GDSCRIPT_STRING,
	GDSCRIPT_NUMBER,
	GDSCRIPT_IDENTIFIER,
	/* GDSCRIPT_PARSE_TABLE_SIZE, // NOTE: wtf is this for? */
	GDSCRIPT_EOF, // NOTE-> USED, BUT DOESN'T HAVE TO BE
	GDSCRIPT_LEADING_WHITESPACE, // gets trimmed out
	GDSCRIPT_WHITESPACE, // also gets trimmed out
	GDSCRIPT_NUMBER_CANDIDATE, // partially used
	GDSCRIPT_SCOPE_START,
	GDSCRIPT_SCOPE_END,
	GDSCRIPT_ARGS_START,
	GDSCRIPT_ARGS_END,
	/* GDSCRIPT_MISSING_COLON, */
	GDSCRIPT_MISSING_PAREN_CLOSE,
	GDSCRIPT_MISSING_BRACE_CLOSE,
	GDSCRIPT_MISSING_BRACKET_CLOSE,
	/* GDSCRIPT_MISSING_PAREN_OPEN, // only triggered by func keyword */
	/* GDSCRIPT_MISSING_BRACE_OPEN, // only triggered by enum keyword */
	// NOTE: "extra" openers just create missing closers, so dont bother
	GDSCRIPT_EXTRA_PAREN_CLOSE,
	GDSCRIPT_EXTRA_BRACE_CLOSE,
	GDSCRIPT_EXTRA_BRACKET_CLOSE,
} gdscript_id_t;

/* int16 max is 32,767--better be plenty large*/
/* NOTE: HAHA NO ITS NOT BIG ENOUGH--IM 32-BIT (secretely 31 bits since one is for sign lmao) PILLED NOW */

typedef struct token_table_t {
	char *src;
	int32_t *offsets;
	int16_t *lens;
	gdscript_id_t *ids;
	int32_t num_tokens;
} token_table_t;

int16_t token_line(const token_table_t* table, const int32_t index);

int16_t token_column(const token_table_t* table, const int32_t index);

void print_tokens(const char* src, const int32_t* offsets, const int16_t* lens, const int32_t n);

void print_allman(const char* src, const int32_t* offsets, const int16_t* lens, const gdscript_id_t* ids, const int32_t num_tokens);

void print_clarity(const char* src, const int32_t* offsets, const int16_t* lens, const gdscript_id_t* ids, const int32_t num_tokens);


#endif /* End GDSCRIPT_PARSER_H */
