#ifndef SOURCE_TYPES_H
#define SOURCE_TYPES_H

#include <stdint.h>

typedef struct source_data_t
{
	char* src;
	char* path; // TODO
	int32_t* token_offsets;
	int16_t* token_lens; // TODO: int32_t*
	enum token_id_t* token_ids;
	int32_t* parse_to_token_indices;
	int16_t* parse_depths;
	int32_t src_len;
	int32_t num_tokens;
	int32_t num_parse_nodes;
} source_data_t;

struct line_col
{
	int32_t line;
	int32_t col;
};


#endif /* End SOURCE_TYPES_H */
