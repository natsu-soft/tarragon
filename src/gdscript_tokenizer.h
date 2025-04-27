#ifndef GDSCRIPT_TOKENIZER_H
#define GDSCRIPT_TOKENIZER_H

#include <stdint.h>
#include "gdscript_parser.h"

typedef struct arena_t arena_t;
typedef struct token_t token_t;


uint32_t gdscript_tokenize(arena_t* arena, token_t** tokens, gdscript_id_t** ids, const char* str, const uint32_t n);

void preprocess_tokens(token_t* tokens, gdscript_id_t* ids, uint32_t* num_tokens);

#endif /* End GDSCRIPT_TOKENIZER_H */
