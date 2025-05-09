#ifndef TOKEN_H
#define TOKEN_H

#include "stdint.h"

typedef struct  token_t
{
	const char* string;
	uint32_t pos;
	uint32_t len;
	/* uint32_t line; */
	/* uint32_t col; */
} token_t;


#endif /* End TOKEN_H */
