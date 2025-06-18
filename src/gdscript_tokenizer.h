#ifndef GDSCRIPT_TOKENIZER_H
#define GDSCRIPT_TOKENIZER_H

#include <stdint.h>
#include "gdscript_parser.h"
#include "arena.h"
#include "token.h"


uint32_t gdscript_tokenize(arena_t* arena, token_t** tokens, gdscript_id_t** ids, const char* str, const uint32_t n);


#endif /* End GDSCRIPT_TOKENIZER_H */
