#ifndef GDSCRIPT_LEXER_H
#define GDSCRIPT_LEXER_H

#include <stdint.h>
#include "gdscript_parser.h"

typedef struct arena_t arena_t;


int32_t gdscript_lex(arena_t* arena, token_table_t* table, const char* src, const int32_t n);

void gdscript_check_pairs(token_table_t* table);

void gdscript_polska(token_table_t* dest_table, const token_table_t *src_table);


#endif /* End GDSCRIPT_LEXER_H */
