#ifndef GDSCRIPT_TOKEN_H
#define GDSCRIPT_TOKEN_H

#include <stdint.h>
#include "tokens_types.h"

int32_t token_line(const token_table_t* table, const int32_t index);

int32_t token_column(const token_table_t* table, const int32_t index);

/* struct line_col token_position(const token_table_t* table, const int32_t index); */

int32_t token_len(int32_t i, const slim_table_t* table);

#endif /* End GDSCRIPT_TOKEN_H */
