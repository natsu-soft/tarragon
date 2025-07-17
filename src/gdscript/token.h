#ifndef TOKEN_H
#define TOKEN_H



#include <stdint.h>
#include "gdscript_ids.h"

typedef struct token_table_t {
	char *src;
	int32_t *offsets;
	int16_t *lens;
	gdscript_id_t *ids;
	int32_t num_tokens;
} token_table_t;

int32_t token_line(const token_table_t* table, const int32_t index);

int32_t token_column(const token_table_t* table, const int32_t index);

#endif /* End TOKEN_H */
