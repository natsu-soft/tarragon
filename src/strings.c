#include "strings.h"
#include <string.h>
/* #include <stdlib.h> */
#include <stdio.h>



uint8_t stringeq(const string a, const string b)
{
	return a.length == b.length && (a.chars == b.chars || strncmp(a.chars, b.chars, a.length) == 0);
}
