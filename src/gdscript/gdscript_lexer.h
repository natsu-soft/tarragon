#ifndef GDSCRIPT_LEXER_H
#define GDSCRIPT_LEXER_H

#include <stdint.h>
/* #include "gdscript_parser.h" */


typedef struct arena_t arena_t;
typedef struct token_table_t token_table_t;

void init_lexer(void);

int32_t gdscript_lex(arena_t* arena, token_table_t* table, const char* src, const int32_t n);


#endif /* End GDSCRIPT_LEXER_H */
