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
	int32_t src_len;
} token_table_t;

#endif /* End TOKEN_TYPES_H */
