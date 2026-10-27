#ifndef SOURCE_DATA_H
#define SOURCE_DATA_H

#include <stdint.h>
#include "source_data_types.h"
#include "token_ids_types.h"

// just some flywheel utils for converting from parse data to token data tbqh


const int32_t parse_node_num_children(const source_data_t* source_data, const int32_t idx);

// flywheel iterators
const int32_t parse_node_first_child(const source_data_t* source_data, const int32_t idx);
const int32_t parse_node_sibling(const source_data_t* source_data, const int32_t idx);
void parse_node_iterate_children(int32_t* child_idx, const source_data_t* src_data, const int32_t idx);

// flywheel converters
const token_id_t parse_idx_to_token_id(const source_data_t* source_data, const int32_t idx);
const int32_t parse_idx_to_token_offset(const source_data_t* source_data, const int32_t idx);
const int16_t parse_idx_to_token_len(const source_data_t*, const int32_t idx);

const int32_t parse_subtree_source_offset0(const source_data_t* source_data, const int32_t idx);
void parse_subtree_source_offset_and_len(int32_t* offset, int16_t* len, const source_data_t* source_data, const int32_t idx);
void parse_subtree_source_offset_and_len_to_eol(int32_t* offset, int16_t* len, const source_data_t* source_data, const int32_t idx);

#endif /* End SOURCE_DATA_H */
