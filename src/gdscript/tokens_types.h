#ifndef TOKEN_TYPES_H
#define TOKEN_TYPES_H
#include <stdint.h>

typedef struct slim_table_t
{
	char* src;
	int32_t length;
	int32_t* offsets;
	int32_t num_tokens;
} slim_table_t;

typedef struct token_table_t {
	char* src;
	int32_t* offsets;
	int16_t* lens;
	enum token_id_t* ids;
	int32_t num_tokens;
} token_table_t;

struct line_col
{
	int32_t line;
	int32_t col;
};


#endif /* End TOKEN_TYPES_H */
