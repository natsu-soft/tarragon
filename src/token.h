#ifndef TOKEN_H
#define TOKEN_H

#include "stdint.h"

/* typedef struct  token_t */
typedef struct __attribute__((packed)) token_t // KINDA HOT TBQH
{
	char* string;
	uint16_t line;
	uint16_t col;
} token_t;

#endif /* End TOKEN_H */
