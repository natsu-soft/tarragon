#ifndef STRINGS_H
#define STRINGS_H

#include <stdint.h>
#include "strings_types.h"



// NOTE: use "%.*s", length, value for print formatting

#ifndef STRING
#define STRING(A) (string){ .chars = A, .length=strlen(A) }
#endif
#ifndef STRINGN
#define STRINGN(A, N) (string){.chars = A, .length=N }
#endif
#ifndef STR
#define STR(...)(string){[__VA_ARGS__]}
#endif

uint8_t stringeq(const string a, const string b);


#endif /* End STRINGS_H */
