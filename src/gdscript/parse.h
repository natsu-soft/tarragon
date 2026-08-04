#ifndef GDSCRIPT_PARSE_H
#define GDSCRIPT_PARSE_H

/* #include "../cursor.h" */
#include "parse_types.h"
#include "tokens_types.h"
#include "../allocators/arena_types.h"

void parse_node_tests(void);

int32_t shunt(parse_table_t* p_table, token_table_t* token_table, arena_t* backing_arena, arena_t* work_arena);

int32_t parse(parse_table_t* p_table, token_table_t* token_table, arena_t* backing_arena, arena_t* work_arena);

int32_t parse_backing_size(const int32_t src_length);

#endif /* End GDSCRIPT_PARSE_H */
