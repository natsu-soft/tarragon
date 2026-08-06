#ifndef GDSCRIPT_PARSE_H
#define GDSCRIPT_PARSE_H

/* #include "../cursor.h" */
#include "parse_types.h"
#include "tokens_types.h"
#include "../allocators/arena_types.h"

struct parse_node* parse_node(arena_t* backing_arena, const parse_table_t* parse_table, const int32_t idx);

void parse_node_sexp(struct parse_node* node, const token_table_t* token_table);

void print_postorder(struct parse_node* node, const int16_t depth, token_table_t* token_table);

int32_t shunt(parse_table_t* p_table, token_table_t* token_table, arena_t* backing_arena, arena_t* work_arena);

int32_t parse(parse_table_t* p_table, token_table_t* token_table, arena_t* backing_arena, arena_t* work_arena);

int32_t parse_backing_size(const int32_t num_tokens);

#endif /* End GDSCRIPT_PARSE_H */
