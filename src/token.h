#ifndef TOKEN_H
#define TOKEN_H

// NOTE: unused atm

typedef struct token_t
{
	char* string;
	unsigned int line;
	unsigned int col;
} token_t;

#endif /* End TOKEN_H */
