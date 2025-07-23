#ifndef FLEXER_H
#define FLEXER_H

#include <stdint.h>

typedef struct arena_t arena_t;
typedef struct token_table_t token_table_t;


int32_t gdscript_flex(arena_t* arena, token_table_t* table, const char* src, const int32_t n);
int32_t gdscript_shunt_etc(token_table_t* const dest, const token_table_t* src, const int32_t n);

#endif /* End FLEXER_H */
