#ifndef DIAGNOSTICS_TYPES_H
#define DIAGNOSTICS_TYPES_H

#include <stdint.h>

#define ERROR_DIAGNOSTICS								  \
	X(LWS_USE_TABS, "Bad leading whitespace. Indent with tabs.") \
	X(LWS_USE_SPACES, "Bad leading whitespace. Indent with spaces.") \
	X(BAD_TOPNODE, "seriously, how did this happen?") \
	X(BAD_CLASS_NAME, "class name must be an identifier") \
	X(BAD_SUBCLASS_NAME, "subclass name must be an identifier") \
	X(TOKENS_AFTER_CLASS_NAME, "no additional tokens allowed after class identifier") \

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
};


#endif /* End DIAGNOSTICS_TYPES_H */
