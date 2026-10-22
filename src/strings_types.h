#ifndef STRINGS_TYPES_H
#define STRINGS_TYPES_H

#include <stdint.h>

typedef struct string
{
	char* chars;
	int32_t length;
} string;

typedef struct string_narray
{
	char** chars;
	int32_t* lengths;
	int32_t size;
} string_narray;

#ifndef STRING_ARRAY
#define STRING_ARRAY(STRING_NAME, LENGTH_NAME)	\
	char** STRING_NAME;							\
	int32_t* LENGTH_NAME;
#endif

typedef struct string_array { STRING_ARRAY(values, lengths) } string_array;


#endif /* End STRINGS_TYPES_H */
