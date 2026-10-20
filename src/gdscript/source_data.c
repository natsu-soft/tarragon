#include "source_data.h"


const int32_t parse_node_num_children(const source_data_t* source_data, const int32_t idx)
{
	int32_t res = 0;
	if (0 < idx && idx < source_data->num_parse_nodes)
	{
		const int16_t* depths = source_data->parse_depths;
		const int16_t child_depth = depths[idx] + 1;
		for (int32_t i = idx - 1; i > 0 && depths[i] >= child_depth; --i)
		{
			res += depths[i] == child_depth;
		}
	}
	return res;
}

const int32_t parse_node_first_child(const source_data_t* source_data, const int32_t idx)
{
	int32_t res = 0;
	if (0 < idx && idx < source_data->num_parse_nodes)
	{
		const int16_t* depths = source_data->parse_depths;
		const int16_t child_depth = depths[idx] + 1;
		int32_t i = idx - 1;
		for (; i > 0 && depths[i] >= child_depth; --i) {;}
		for (; i < idx; ++i)
		{
			if (depths[i] == child_depth)
			{
				res = i;
				break;
			}
		}
	}
	return res;
}

const int32_t parse_node_sibling(const source_data_t* source_data, const int32_t idx)
{
	int32_t res = 0;
	if (0 < idx && idx < source_data->num_parse_nodes)
	{
		const int16_t* depths = source_data->parse_depths;
		const int16_t depth = depths[idx];
		for (int32_t i = idx + 1; depths[i] <= depth && source_data->num_parse_nodes; ++i)
		{
			if (depths[i] == depth)
			{
				res = i;
				break;
			}
		}
	}
	return res;
}

const token_id_t parse_idx_to_token_id(const source_data_t* source_data, const int32_t idx)
{
	token_id_t token_id = GDSCRIPT_NONE;
	if (0 < idx && idx < source_data->num_parse_nodes)
	{
		token_id = source_data->token_ids[source_data->parse_to_token_indices[idx]];
	}
	return token_id;
}

const int32_t parse_idx_to_token_offset(const source_data_t* source_data, const int32_t idx)
{
	int32_t token_offset = 0;
	if (0 < idx && idx < source_data->num_parse_nodes)
	{
		token_offset = source_data->token_offsets[source_data->parse_to_token_indices[idx]];
	}
	return token_offset;
}

const int16_t parse_idx_to_token_len(const source_data_t* source_data, const int32_t idx)
{
	int16_t token_len = 0;
	if (0 < idx && idx < source_data->num_parse_nodes)
	{
		token_len = source_data->token_lens[source_data->parse_to_token_indices[idx]];
	}
	return token_len;
}

const int32_t parse_subtree_source_offset0(const source_data_t* source_data, const int32_t idx)
{
	int32_t offset = 0;
	if (0 < idx && idx < source_data->num_parse_nodes)
	{
		const int16_t* depths = source_data->parse_depths;
		const int16_t depth = depths[idx];
		offset = parse_idx_to_token_offset(source_data, idx);
		for (int32_t i = idx - 1; i > 0 && depths[i] > depth; --i)
		{
			int32_t temp = parse_idx_to_token_offset(source_data, i);
			if(temp < offset)
			{
				offset = temp;
			}
		}
	}
	return offset;
}


void parse_subtree_source_offset_and_len(int32_t* offset, int16_t* len, const source_data_t* source_data, const int32_t idx)
{
	*offset = 0;
	*len = 0;
	if (0 < idx && idx < source_data->num_parse_nodes)
	{
		const int16_t* depths = source_data->parse_depths;
		const int16_t depth = depths[idx];
		int32_t offset0 = parse_idx_to_token_offset(source_data, idx);
		int32_t offset1 = offset0;
		int16_t len1 = parse_idx_to_token_len(source_data, idx);
		for (int32_t i = idx - 1; i > 0 && depths[i] > depth; --i)
		{
			const int32_t offset = parse_idx_to_token_offset(source_data, i);
			if (offset < offset0)
			{
				offset0 = offset;
			}
			else if (offset1 < offset)
			{
				offset1 = offset;
				len1 = parse_idx_to_token_len(source_data, i);
			}
		}
		*offset = offset0;
		*len = offset1 - offset0 + len1;
	}
}
