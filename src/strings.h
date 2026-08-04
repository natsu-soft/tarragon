#ifndef STRINGS_H
#define STRINGS_H

#include <stdint.h>


typedef struct string
{
	char* chars;
	int32_t length;
} string;

// NOTE: use "%.*s", length, value for print formatting

#ifndef STRING
#define STRING(A) (string){ .chars = A, .length=strlen(A) }
#endif
#ifndef STRINGN
#define STRINGN(A, N) (string){.chars = A, .length=N }
#endif
#ifndef STR
#define STR(...)(string){__VA_ARGS__}
#endif

#ifndef STRING_ARRAY
#define STRING_ARRAY(STRING_NAME, LENGTH_NAME)	\
	char** STRING_NAME;							\
	int32_t* LENGTH_NAME;
#endif

typedef struct string_array { STRING_ARRAY(values, lengths) } string_array;

typedef struct string_narray
{
	char** chars;
	int32_t* lengths;
	int32_t size;
} string_narray;

uint8_t stringeq(const string a, const string b);


#endif /* End STRINGS_H */
