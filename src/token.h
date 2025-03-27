#ifndef TOKEN_H
#define TOKEN_H

#include "stdint.h"

typedef struct  token_t
{
	char* string;
	uint32_t line;
	uint32_t col;
} token_t;

/* typedef enum __attribute__((packed)) token_id_t */
/* { */
/* 	TOKEN_NONE = 0x00, */
/* 	TOKEN_SPACING = 0x01, */
/* 	TOKEN_KEYWORD = 0x02, */
/* 	TOKEN_OPERATOR = 0x04, */
/* 	TOKEN_CONSTANT = 0x08, // constants are just literal values */
/* 	TOKEN_IDENTIFIER = 0x10, // types, class names (which are also types), function names, variable names (and is that it frfr?) */
/* 	/\* >:^) *\/ */
/* 	TOKEN_ID_VAR = 0x20 | TOKEN_IDENTIFIER, */
/* 	TOKEN_ID_FUNC = 0x40 | TOKEN_IDENTIFIER, */
/* 	TOKEN_ID_TYPE = 0x80 | TOKEN_IDENTIFIER, */
/* } token_id_t; */

#endif /* End TOKEN_H */
