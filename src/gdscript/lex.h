#ifndef GDSCRIPT_LEX_AND_SHUNT_H
#define GDSCRIPT_LEX_AND_SHUNT_H

#include <stdint.h>
#include "source_data_types.h"
#include "../allocators/arena_types.h"

void lex_init(void);

// NOTE: passing in the backing & work arenas for the eventual multithreadening
int32_t lex(source_data_t* src_data, arena_t* backing_arena, arena_t* work_arena, const char* src, const int32_t length);

const int32_t lex_backing_size(const int32_t length);

const int32_t lex_work_size(const int32_t length);


/* int32_t slim_lex(slim_table_t* slim_table, arena_t* backing_arena, arena_t* work_arena, char* const src, const int32_t length); */

/* const int32_t slim_lex_backing_size(const int32_t length); */

/* const int32_t slim_lex_work_size(const int32_t length); */

#endif /* End GDSCRIPT_LEX_AND_SHUNT_H */
