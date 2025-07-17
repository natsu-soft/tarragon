#ifndef GDSCRIPT_PARSER_H
#define GDSCRIPT_PARSER_H

#include <stdint.h>
#include "gdscript_ids.h"

/* int16 max is 32,767--better be plenty large*/
/* NOTE: HAHA NO ITS NOT BIG ENOUGH--IM 32-BIT (secretely 31 bits since one is for sign lmao) PILLED NOW */


void print_tokens(const char* src, const int32_t* offsets, const int16_t* lens, const int32_t n);

void print_allman(const char* src, const int32_t* offsets, const int16_t* lens, const gdscript_id_t* ids, const int32_t num_tokens);

void print_clarity(const char* src, const int32_t* offsets, const int16_t* lens, const gdscript_id_t* ids, const int32_t num_tokens);

void print_ids(const char* src, const int32_t* offsets, const int16_t* lens, const gdscript_id_t* ids, const int32_t num_tokens);


#endif /* End GDSCRIPT_PARSER_H */
