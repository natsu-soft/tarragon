#ifndef GDSCRIPT_PARSE_H
#define GDSCRIPT_PARSE_H

/* #include "../cursor.h" */
#include "parse_types.h"
#include "source_data_types.h"
#include "tokens_types.h"
#include "../allocators/arena_types.h"

struct parse_node* parse_node(arena_t* backing_arena, const source_data_t* src_data, const int32_t idx);
struct parse_node* shallow_parse_node(arena_t* backing_arena, const source_data_t* src_data, const int32_t idx);

void parse_node_sexp(struct parse_node* node, const source_data_t* src_data);

void print_postorder(struct parse_node* node, const int16_t depth, source_data_t* src_data);

int32_t shunt(source_data_t* src_data, arena_t* backing_arena, arena_t* work_arena);

int32_t parse(source_data_t* src_data, arena_t* backing_arena, arena_t* work_arena);

int32_t parse_backing_size(const int32_t num_tokens);

#endif /* End GDSCRIPT_PARSE_H */
