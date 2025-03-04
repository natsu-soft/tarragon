#ifndef TOKENS_H
#define TOKENS_H

#include "list.h"

typedef struct tokens_t
{
	struct list_t list;
	char* token;
} tokens_t;

#endif /* End TOKENS_H */
