#if defined(TOKEN_IDS_MACROS) || !defined(TOKEN_IDS_TYPES_H)

#define GDSCRIPT_STRUCTURE												\
	X(PERIOD,		 ".",	1)											\
	X(TWO_PERIODS,   "..",  2)											\
	X(ELLIPSES,      "...", 3)											\
	X(COMMA,		 ",",	1)										\
	X(ARROW,		 "->",	2)										\
	X(COLON,		 ":",	1)										\
	X(STRUCTURAL_COLON,	 ":",	1) /* NOTE: you dont really sneed it but u like it */ \
	X(PAREN_OPEN,	 "(",	1)										\
	X(BRACE_OPEN,	 "{",	1)										\
	X(BRACKET_OPEN,	 "[",	1)										\
	X(PAREN_CLOSE,	 ")",	1)										\
	X(BRACE_CLOSE,	 "}",	1)										\
	X(BRACKET_CLOSE, "]",	1)										\
	X(SEMICOLON,	 ";",	1)										\
	X(LINE_END,		 "\n",	1)										\
	X(LINE_CONTINUE, "\\",	1)										\
	X(COMMENT,		 "#",	1)

#ifdef	NO_KEYWORDS
#define GDSCRIPT_BINARY_OPERATORS				\
		X(OP_POWER,					"**",		2)	\
		X(OP_BOOL_AND,				"&&",		2)	\
		X(OP_BOOL_OR,				"||",		2)	\
		X(OP_BITSHIFT_LEFT,			"<<",		2)	\
		X(OP_BITSHIFT_RIGHT,		">>",		2)	\
		X(OP_EQUALS,				"==",		2)	\
		X(OP_INEQUALS,				"!=",		2)	\
		X(OP_LESS_THAN_EQUALS,		"<=",		2)	\
		X(OP_GREATER_THAN_EQUALS,	">=",		2)	\
		X(OP_SUM,					"+",		1)	\
		X(OP_DIFFERENCE,			"-",		1)	\
		X(OP_PRODUCT,				"*",		1)	\
		X(OP_DIVISION,				"/",		1)	\
		X(OP_MODULO,				"%",		1)	\
		X(OP_BITWISE_AND,			"&",		1)	\
		X(OP_BITWISE_OR,			"|",		1)	\
		X(OP_BITWISE_XOR,			"^",		1)	\
		X(OP_LESS_THAN,				"<",		1)	\
		X(OP_GREATER_THAN,			">",		1)
//		NOTE:						REORDER		CAREFULLY!	A switch	in	gdscript_shunt	is		order	dependent
#define GDSCRIPT_UNARY_OPERATORS				\
		X(OP_BITWISE_NOT,			"~",		1)	\
		X(OP_IDENTITY,				"+",		1)	\
		X(OP_NEGATION,				"-",		1)	\
		X(OP_BOOL_NOT,				"!",		1)
#else
#define GDSCRIPT_BINARY_OPERATORS				\
		X(OP_IS,		            "is",		2)	\
		X(OP_IN,		            "in",		2)	\
		X(OP_AS,					"as",		2)	\
		X(OP_XOR,					"xor",		3)	\
		X(OP_OR,					"or",		2)	\
		X(OP_AND,					"and",		3)	\
		X(OP_POWER,					"**",		2)	\
		X(OP_BOOL_AND,				"&&",		2)	\
		X(OP_BOOL_OR,				"||",		2)	\
		X(OP_BITSHIFT_LEFT,			"<<",		2)	\
		X(OP_BITSHIFT_RIGHT,		">>",		2)	\
		X(OP_EQUALS,				"==",		2)	\
		X(OP_INEQUALS,				"!=",		2)	\
		X(OP_LESS_THAN_EQUALS,		"<=",		2)	\
		X(OP_GREATER_THAN_EQUALS,	">=",		2)	\
		X(OP_SUM,					"+",		1)	\
		X(OP_DIFFERENCE,			"-",		1)	\
		X(OP_PRODUCT,				"*",		1)	\
		X(OP_DIVISION,				"/",		1)	\
		X(OP_MODULO,				"%",		1)	\
		X(OP_BITWISE_AND,			"&",		1)	\
		X(OP_BITWISE_OR,			"|",		1)	\
		X(OP_BITWISE_XOR,			"^",		1)	\
		X(OP_LESS_THAN,				"<",		1)	\
		X(OP_GREATER_THAN,			">",		1)	\
		/*							X(OP_TYPE,	":",		1)			*/	//				NOTE:	I hate	this.

#define GDSCRIPT_UNARY_OPERATORS	\
	X(OP_NOT,		     "not",	3)  \
	X(OP_BITWISE_NOT,	 "~",	1)	\
	X(OP_IDENTITY,		 "+",	1)	\
	X(OP_NEGATION,		 "-",	1)	\
	X(OP_BOOL_NOT,		 "!",	1)
#endif

#define GDSCRIPT_ASSIGNMENT_OPERATORS                 \
	X(ASSIGNMENT_POWER,		    "**=",			3) \
	X(ASSIGNMENT_PRODUCT,	    "*=",			2) \
	X(ASSIGNMENT_SUM,		    "+=",			2) \
	X(ASSIGNMENT_DIFFERENCE,	    "-=",			2) \
	X(ASSIGNMENT_DIVISION,	    "/=",			2) \
	X(ASSIGNMENT_MODULO,		    "%=",			2) \
	X(ASSIGNMENT_BITWISE_AND,    "&=",			2) \
	X(ASSIGNMENT_BITWISE_OR,	    "|=",			1) \
	X(ASSIGNMENT_BITSHIFT_LEFT,	"<<=",			3) \
	X(ASSIGNMENT_BITSHIFT_RIGHT,	">>=",			3) \
	X(ASSIGNMENT_XOR,			"^=",			2) \
	X(ASSIGNMENT_INFERRED,		":=",			2) \
	X(ASSIGNMENT,			     "=",			1)

#define GDSCRIPT_GRAPHEMES \
	GDSCRIPT_STRUCTURE \
	GDSCRIPT_ASSIGNMENT_OPERATORS \
	X(OP_POWER,					    "**",			2) \
	X(OP_BOOL_AND,				    "&&",			2) \
	X(OP_BOOL_OR,				    "||",			2) \
	X(OP_BITSHIFT_LEFT,				"<<",			2) \
	X(OP_BITSHIFT_RIGHT,			">>",			2) \
	X(OP_EQUALS,					"==",			2) \
	X(OP_INEQUALS,					"!=",			2) \
	X(OP_LESS_THAN_EQUALS,			"<=",			2) \
	X(OP_GREATER_THAN_EQUALS,		">=",			2) \
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
	X(OP_BITWISE_NOT,	 "~",	1)	\
	X(OP_IDENTITY,		 "+",	1)	\
	X(OP_NEGATION,		 "-",	1)	\
	X(OP_BOOL_NOT,		 "!",	1)


#define NUM_NON_ASSIGNMENT_OPERATORS 30
#define GDSCRIPT_REGULAR_OPERATORS \
	X(OP_IS,		  "is",				2) \
	X(OP_IN,		  "in",				2) \
	X(OP_AS,		  "as",				2) \
	X(OP_NOT,		  "not",				3) \
	X(OP_XOR,		  "xor",				3) \
	X(OP_OR,		  "or",				2) \
	X(OP_AND,		  "and",				3) \
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

#define GDSCRIPT_KEYWORD_OPERATORS \
	X(OP_IS,		  "is",				2) \
	X(OP_IN,		  "in",				2) \
	X(OP_AS,		  "as",				2) \
	X(OP_NOT,		  "not",				3) \
	X(OP_XOR,		  "xor",				3) \
	X(OP_OR,		  "or",				2) \
	X(OP_AND,		  "and",				3) \

// NOTE: DONT TOUCH, RETARD.  ORDER MATTERS FOR keep_pair_p IN gdscript_shunt.c
#define GDSCRIPT_ANNOTATIONS											\
	X(ABSTRACT, "@abstract", 9) \
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
	X(ONREADY,						"@onready",						8) \
	X(RPC,							"@rpc",							4) \
	X(STATIC_UNLOAD,				"@static_unload",				14) \
	X(TOOL,							"@tool",						5) \
	X(WARNING_IGNORE_RESTORE,		"@warning_ignore_restore",		23) \
	X(WARNING_IGNORE_STOP,			"@warning_ignore_stop",			20) \
	X(WARNING_IGNORE,				"@warning_ignore",				15) \

#define GDSCRIPT_KEYWORDS \
	X(IF,         "if",				2) \
	X(ELSE,		  "else",			4) \
	X(ELIF,		  "elif",			4) \
	X(FUNC,		  "func",		 	4) \
	X(FOR,		  "for",			3) \
	X(WHEN,	  "when",			5) \
	X(WHILE,	  "while",			5) \
	X(MATCH,	  "match",			5) \
	X(BREAK,	  "break",			5) \
	X(CONTINUE,	  "continue",		8) \
	X(PASS,		  "pass",			4) \
	X(RETURN,	  "return",			6) \
	X(CLASS_NAME, "class_name",     10) \
	X(CLASS,	  "class",			5) \
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
	/* X(IS,		  "is",				2) \ */
	/* X(IN,		  "in",				2) \ */
	/* X(AS,		  "as",				2) \ */
	/* X(OR,		  "or",				2) \ */
	/* X(AND,		  "and",				3) \ */
	/* X(NOT,		"not",				3) \ */
	/* X(XOR,		"xor",				3) \ */
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
	GDSCRIPT_GRAPHEMES

#define GDSCRIPT_BUILTIN_TOKENS \
	GDSCRIPT_STRUCTURE \
	GDSCRIPT_ASSIGNMENT_OPERATORS \
	GDSCRIPT_OPERATORS \
	GDSCRIPT_ANNOTATIONS \
	GDSCRIPT_KEYWORDS \
	GDSCRIPT_LITERALS

#define GDSCRIPT_NOT_OPERATORS \
	GDSCRIPT_STRUCTURE \
	GDSCRIPT_ASSIGNMENT_OPERATORS \
	GDSCRIPT_ANNOTATIONS \
	GDSCRIPT_KEYWORDS \
	GDSCRIPT_LITERALS

#define GDSCRIPT_WARNINGS \
	X(BULLSHIT) \
	X(THIS_BULLSHIT) \

#define GDSCRIPT_ERRORS \
	X(SEEK_CHRIST) \
	X(UNTERMINATED_STRING) \
	X(IMPROPER_DEPTH) \
	X(MIXED_LWS) \
	X(UNKNOWN_ANNOTATION) \
	X(UNKNOWN_GRAPH) \
	X(PERVERTED_NUMBER) \
	X(PERVERTED_BINARY) \
	X(PERVERTED_HEXADECIMAL) \
	X(PERVERTED_IDENTIFIER) \
	X(NON_UNARY_OPERATOR)	 \
	X(TOKENS_AFTER_CONTINUATION) \
	X(MISSING_PAREN_CLOSE) \
	X(MISSING_BRACE_CLOSE) \
	X(MISSING_BRACKET_CLOSE) \
	X(MISSING_EXPONENT) \
	X(EXTRA_PAREN_CLOSE) \
	X(EXTRA_BRACE_CLOSE) \
	X(EXTRA_BRACKET_CLOSE) \
	X(EXTRA_COMMA) \
	X(MULTIPLE_ASSIGNMENTS) \

#define GDSCRIPT_NONKEYWORD_LITERALS			\
	X(INTEGER)									\
	X(FLOAT)								\
	X(STRING)								\
	X(STRING_NAME)							\
	X(RESOURCE)								\
	X(NODE_PATH)
#endif

#define PARSE_IDS \
	X(SCRIPT) \
	X(SCOPE) \
	X(ANNOTATION_PARAMS) \
	X(LAMBDA) \
	X(VARARG) \
	X(TERNARY) \
	X(FUNC) \
	X(GROUP) \
	X(DICT) \
	X(ARRAY) \
	X(INDEXED_ARRAY)

#if !defined(TOKEN_IDS_MACROS) && !defined(TOKEN_IDS_TYPES_H)
#define TOKEN_IDS_TYPES_H


typedef enum __attribute__((packed)) token_id_t
/* typedef enum gdscript_id_t */
{
	GDSCRIPT_NONE = 0,
	// NOTE: these are S P E C I A L
	#define X(ID) PARSE_##ID,
	PARSE_IDS
	#undef X
	TOKEN_START,
	#define X(ID, ...) GDSCRIPT_##ID,
	GDSCRIPT_BUILTIN_TOKENS
	#undef X
	GDSCRIPT_NUM_BUILTIN,
	// NOTE: fml I have to promote identifiers to these manually
	GDSCRIPT_TERNARY_IF,
	GDSCRIPT_TERNARY_ELSE,
	/* GDSCRIPT_GET, */
	/* GDSCRIPT_SET, */
	GDSCRIPT_NUMBER,
	GDSCRIPT_IDENTIFIER,
	#define X(ID) GDSCRIPT_##ID,
	GDSCRIPT_NONKEYWORD_LITERALS
	#undef X
	/* GDSCRIPT_PARSE_TABLE_SIZE, // NOTE: wtf is this for? */
	GDSCRIPT_EOF,
	GDSCRIPT_LEADING_WHITESPACE, // TODO: remove this
	GDSCRIPT_WHITESPACE, // TODO: remove this
	GDSCRIPT_NUMBER_CANDIDATE, // TODO: remove this
	GDSCRIPT_SCOPE_OPEN,
	GDSCRIPT_SCOPE_CLOSE,
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
} token_id_t;


#ifdef DEFINE_TOKEN_PREDICATES

#ifndef TOKEN_ID_PREDICATES
#define TOKEN_ID_PREDICATES
static inline uint8_t assignment_p(const token_id_t id)
{
	return GDSCRIPT_ASSIGNMENT_POWER <= id && id <= GDSCRIPT_ASSIGNMENT;
}

static inline uint8_t binary_operator_p(const token_id_t id)
{
	return (GDSCRIPT_OP_IS <= id && id <= GDSCRIPT_OP_GREATER_THAN) ||
		id == GDSCRIPT_COLON;
}

static inline uint8_t unary_operator_p(const token_id_t id)
{
	return GDSCRIPT_OP_NOT <= id && id <= GDSCRIPT_OP_BOOL_NOT;
}

static inline uint8_t identifier_or_literal_p(const token_id_t id)
{
	return id == GDSCRIPT_IDENTIFIER ||
		(GDSCRIPT_NULL <= id && id <= GDSCRIPT_NAN) ||
		(GDSCRIPT_INTEGER <= id && id <= GDSCRIPT_NODE_PATH);
}

static inline uint8_t annotation_p(const token_id_t id)
{
	return GDSCRIPT_ABSTRACT <= id && id <= GDSCRIPT_WARNING_IGNORE;
}

static inline uint8_t operator_arg_p(const token_id_t id)
{
	return identifier_or_literal_p(id) || id == GDSCRIPT_PERIOD || (PARSE_TERNARY <= id && id <= PARSE_INDEXED_ARRAY);
}

static inline uint8_t expression_p(const token_id_t id)
{
	return operator_arg_p(id) || (GDSCRIPT_OP_BITWISE_NOT <= id && id <= GDSCRIPT_OP_BOOL_NOT) ||
		(GDSCRIPT_OP_IS <= id && id <= GDSCRIPT_OP_GREATER_THAN);
}


#endif /* End TOKEN_ID_PREDICATES */

#endif /* End DEFINE_TOKEN_PREDICATES */

#endif /* End TOKEN_IDS_TYPES_H */
