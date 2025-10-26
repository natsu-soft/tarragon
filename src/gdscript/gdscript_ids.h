#ifndef GDSCRIPT_IDS_H
#define GDSCRIPT_IDS_H


#define GDSCRIPT_STRUCTURE		\
	X(PERIOD,		 ".",	1)	\
	X(COMMA,		 ",",	1)	\
	X(ARROW,		 "->",	2)	\
	X(COLON,		 ":",	1)	\
	X(NESTED_COLON,	 ":",	1)	 /* NOTE: you SNEED it if you want to scope oneliners properly */ \
	X(PAREN_CLOSE,	 ")",	1)	\
	X(BRACE_CLOSE,	 "}",	1)	\
	X(BRACKET_CLOSE, "]",	1)  \
	X(PAREN_OPEN,	 "(",	1)	\
	X(BRACE_OPEN,	 "{",	1)	\
	X(BRACKET_OPEN,	 "[",	1) \
	X(SEMICOLON,	 ";",	1)	\
	X(LINE_END,		 "\n",	1)	\
	X(LINE_CONTINUE, "\\",	1)	\
	X(COMMENT,		 "#",	1)

// NOTE: REORDER CAREFULLY! A switch in gdscript_shunt is order dependent
#define GDSCRIPT_BINARY_OPERATORS  					   \
	X(OP_ASSIGNMENT_POWER,		    "**=",			3) \
	X(OP_ASSIGNMENT_BITSHIFT_LEFT,	"<<=",			3) \
	X(OP_ASSIGNMENT_BITSHIFT_RIGHT,	">>=",			3) \
	X(OP_POWER,					    "**",			2) \
	X(OP_ASSIGNMENT_PRODUCT,	    "*=",			2) \
	X(OP_ASSIGNMENT_SUM,		    "+=",			2) \
	X(OP_ASSIGNMENT_DIFFERENCE,	    "-=",			2) \
	X(OP_ASSIGNMENT_DIVISION,	    "/=",			2) \
	X(OP_ASSIGNMENT_MODULO,		    "%=",			2) \
	X(OP_BOOL_AND,				    "&&",			2) \
	X(OP_ASSIGNMENT_BITWISE_AND,    "&=",			2) \
	X(OP_BOOL_OR,				    "||",			2) \
	X(OP_ASSIGNMENT_BITWISE_OR,	    "|=",			2) \
	X(OP_BITSHIFT_LEFT,				"<<",			2) \
	X(OP_BITSHIFT_RIGHT,			">>",			2) \
	X(OP_ASSIGNMENT_XOR,			"^=",			2) \
	X(OP_EQUALS,					"==",			2) \
	X(OP_INEQUALS,					"!=",			2) \
	X(OP_LESS_THAN_EQUALS,			"<=",			2) \
	X(OP_GREATER_THAN_EQUALS,		">=",			2) \
	X(OP_ASSIGNMENT_INFERRED,		":=",			2) \
	X(OP_SUM,					    "+",			1) \
	X(OP_DIFFERENCE,			    "-",			1) \
	X(OP_PRODUCT,				    "*",			1) \
	X(OP_DIVISION,				    "/",			1) \
	X(OP_MODULO,				    "%",			1) \
	X(OP_BITWISE_AND,			    "&",			1) \
	X(OP_BITWISE_OR,				"|",			1) \
	X(OP_BITWISE_XOR,				"^",			1) \
	X(OP_LESS_THAN,					"<",			1) \
	X(OP_GREATER_THAN,				">",			1) \
	X(OP_ASSIGNMENT,			     "=",			1) \
	X(OP_TYPE,			     ":",			1) // NOTE: I hate this.

// NOTE: REORDER CAREFULLY! A switch in gdscript_shunt is order dependent
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
	X(OP_ASSIGNMENT_BITSHIFT_RIGHT,	">>=",			3) \
	X(OP_ASSIGNMENT_XOR,			"^=",			2) \
	X(OP_ASSIGNMENT_INFERRED,		":=",			2) \
	X(OP_ASSIGNMENT,			     "=",			1)

#define NUM_NON_ASSIGNMENT_OPERATORS 27
#define GDSCRIPT_REGULAR_OPERATORS \
	X(IS,		  "is",				2) \
	X(IN,		  "in",				2) \
	X(AS,		  "as",				2) \
	X(NOT,		  "not",				3) \
	X(XOR,		  "xor",				3) \
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

// NOTE: DONT TOUCH, RETARD.  ORDER MATTERS FOR keep_pair_p IN gdscript_shunt.c
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
	X(IF,         "if",				2) \
	X(ELIF,		  "elif",			4) \
	X(ELSE,		  "else",			4) \
	X(FUNC,		  "func",		 	4) \
	X(FOR,		  "for",			3) \
	X(WHILE,	  "while",			5) \
	X(MATCH,	  "match",			5) \
	X(BREAK,	  "break",			5) \
	X(CONTINUE,	  "continue",		8) \
	X(PASS,		  "pass",			4) \
	X(RETURN,	  "return",			6) \
	X(CLASS_NAME, "class_name",     10) \
	X(CLASS,	  "class",			5) \
	X(IS,		  "is",				2) \
	X(IN,		  "in",				2) \
	X(AS,		  "as",				2) \
	X(OR,		  "or",				2) \
	X(AND,		  "and",				3) \
	X(NOT,		"not",				3) \
	X(XOR,		"xor",				3) \
	X(EXTENDS,	  "extends",		7) \
	X(SELF,		  "self",			4) \
	X(SUPER,	  "super",			5) \
	X(SIGNAL,	  "signal",			6) \
	X(CONST,	  "const",			5) \
	X(VAR,		  "var",		 	3) \
	X(ENUM,		  "enum",		 	4) \
	X(STATIC,	  "static",			6) \
	X(BREAKPOINT, "breakpoint", 	10) \
	X(PRELOAD,	  "preload",	 	7) \
	X(AWAIT,	  "await",			5) \
	X(YIELD,	  "yield",		 	5) \
	X(ASSERT,	  "assert",		 	6) \
	X(VOID,		  "void",		 	4) \
	X(EMIT,		  "emit",		 	4)
	/* X(GET,		  "get",		 	3) \ */
	/* X(SET,		  "set",		 	3) \ */

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

#define GDSCRIPT_WARNINGS \
	X(BULLSHIT) \
	X(THIS_BULLSHIT) \

#define GDSCRIPT_ERRORS \
	X(SEEK_CHRIST) \
	X(IMPROPER_DEPTH) \
	X(MIXED_LWS) \
	X(UNKNOWN_GRAPH) \
	X(PERVERTED_NUMBER) \
	X(PERVERTED_IDENTIFIER) \
	X(NON_UNARY_OPERATOR)	 \
	X(TOKENS_AFTER_CONTINUATION) \
	X(MISSING_PAREN_CLOSE) \
	X(MISSING_BRACE_CLOSE) \
	X(MISSING_BRACKET_CLOSE) \
	X(EXTRA_PAREN_CLOSE) \
	X(EXTRA_BRACE_CLOSE) \
	X(EXTRA_BRACKET_CLOSE) \
	X(EXTRA_COMMA) \
	X(MULTIPLE_ASSIGNMENTS) \

typedef enum __attribute__((packed)) gdscript_id_t
/* typedef enum gdscript_id_t */
{
	GDSCRIPT_NONE = 0,
	#define X(ID, ...) GDSCRIPT_##ID,
	GDSCRIPT_BUILTIN_TOKENS
	#undef X
	GDSCRIPT_NUM_BUILTIN,
	// NOTE: fml I have to promote identifiers to these manually
	GDSCRIPT_GET,
	GDSCRIPT_SET,
	GDSCRIPT_IDENTIFIER,
	GDSCRIPT_NUMBER,
	GDSCRIPT_STRING,
	/* GDSCRIPT_PARSE_TABLE_SIZE, // NOTE: wtf is this for? */
	GDSCRIPT_EOF, // NOTE-> USED, BUT DOESN'T HAVE TO BE
	GDSCRIPT_LEADING_WHITESPACE, // gets trimmed out
	GDSCRIPT_WHITESPACE, // also gets trimmed out
	GDSCRIPT_NUMBER_CANDIDATE, // TODO: comment this out ASAP
	GDSCRIPT_SCOPE_START,
	GDSCRIPT_SCOPE_END,
	GDSCRIPT_FAKE_SCOPE_START,
	GDSCRIPT_FAKE_SCOPE_END,
	GDSCRIPT_CULL_PAREN_OPEN,
	GDSCRIPT_CULL_PAREN_CLOSE,
	// JUST THUG THIS SHIT OUT
	/* GDSCRIPT_PARAMS_START, */
	/* GDSCRIPT_PARAMS_END, */
	/* GDSCRIPT_ARGS_START, */
	/* GDSCRIPT_ARGS_END, */
	GDSCRIPT_WARNING_THRESHOLD,
	#define X(ID) WARN_##ID,
	GDSCRIPT_WARNINGS
	#undef X
	GDSCRIPT_ERROR_THRESHOLD,
	#define X(ID) GDSCRIPT_##ID,
	GDSCRIPT_ERRORS
	#undef X
	GDSCRIPT_TOTAL_NUM
} gdscript_id_t;

#endif /* End GDSCRIPT_IDS_H */
