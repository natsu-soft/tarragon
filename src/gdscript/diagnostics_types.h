#ifndef DIAGNOSTICS_TYPES_H
#define DIAGNOSTICS_TYPES_H

#include <stdint.h>

#define ERROR_DIAGNOSTICS								  \
	X(UNTERMINATED_STRING, "Unterminated string!") \
	X(UNKNOWN_ANNOTATION, "Unknown annotation!") \
	X(LWS_USE_TABS, "Bad leading whitespace. Indent with tabs.") \
	X(LWS_USE_SPACES, "Bad leading whitespace. Indent with spaces.") \
	X(BAD_TOPNODE, "seriously, how did this happen?") \
	X(NON_LEADING_TOOL, "@tool must be at the very start of this script!") \
	X(MULTIPLE_CLASS_NAMES, "'class_name' can only occur once per script")										\
	X(CLASS_NAME_IN_BODY, "'class_name' must be at the top of the script")										\
	X(NO_ABSTRACT_TOOLS_ANNOTATION, "@annotation must be at the very start of this script!") \
	X(BAD_CLASS_NAME, "class name must be an identifier") \
	X(BAD_SUBCLASS_NAME, "subclass name must be an identifier") \
	X(TOKENS_AFTER_CLASS_NAME, "no additional tokens allowed after class identifier") \
	X(PERVERTED_NUMBER, "Malformed number literal.") \
	X(MISSING_EXPONENT, "Missing exponent in number literal.") \
	X(PERVERTED_BINARY, "Malformed binary literal.") \
	X(PERVERTED_HEXADECIMAL, "Malformed hexadecimal literal.") \
	X(MISSING_PAREN_CLOSE, "Missing closing ')'.") \
	X(MISSING_BRACE_CLOSE, "Missing closing '}'.") \
	X(MISSING_BRACKET_CLOSE, "Missing closing ']'.") \
	X(TOKENS_AFTER_CONTINUATION, "Unexpected tokens after line continuation.") \
	X(EXTRA_PAREN_CLOSE, "Extra closing ')'.") \
	X(EXTRA_BRACE_CLOSE, "Extra closing '}'.") \
	X(EXTRA_BRACKET_CLOSE, "Extra closing ']'.") \

#define WARNING_DIAGNOSTICS

#define NOTE_DIAGNOSTICS

enum __attribute__((packed)) diagnostic_msg
{
	DIAGNOSTIC_NONE = 0,
#define X(ID,...) DIAGNOSTIC_##ID,
	ERROR_DIAGNOSTICS
#undef X
#define X(ID,...) DIAGNOSTIC_##ID,
	WARNING_DIAGNOSTICS
#undef X
#define X(ID,...) DIAGNOSTIC_##ID,
	NOTE_DIAGNOSTICS
#undef X
	DIAGNOSTIC_SIZE
};


#endif /* End DIAGNOSTICS_TYPES_H */
