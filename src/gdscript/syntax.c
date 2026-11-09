#include "syntax.h"
#include <assert.h>
#include <string.h>
#include <stdatomic.h>
#include <stdbool.h>
#define DEFINE_TOKEN_PREDICATES
#include "token_ids_types.h"
#undef DEFINE_TOKEN_PREDICATES
#include "../atomics.h"
#include "source_data.h"
#include "diagnostics.h"
#include "../allocators/arena.h"
#include "../allocators/scratch.h"
#include "../allocators/workpad.h"
#include "../job.h"
#include "../bitset.h"

struct metadata_header;

enum __attribute__((packed))
{
	NONE			= 0x00,
	HAS_TOOL		= 0x01,
	HAS_ABSTRACT	= 0x02,
	HAS_CLASS_NAME	= 0x04,
	HAS_EXTENDS		= 0x08,
};

struct metadata_header
{
	source_data_t* src_data;
	struct bitset* hoist_set;
	struct bitset* semantic_set;
};

#define nil_index (0)

// NOTE: passing in ptrs to index because it also doubles as the offset - 1 to metadata_header
static void check_parse_header(struct metadata_header* header, const int32_t header_idx);
static void check_root_node(int32_t* idx_ptr);
static void check_all_children(struct metadata_header* header, const int32_t parent_idx);
static void check_annotation_tool(uint8_t* header_flags, struct metadata_header* header, const int32_t tool_idx);
static void check_annotation_abstract(uint8_t* header_flags, struct metadata_header* header, const int32_t tool_idx);

static_assert(sizeof(struct metadata_header) % sizeof(int32_t) == 0, "ERROR: sizeof(struct metadata_header) must be divisible by 32 because of degenerate memes");

void syntax_check(struct bitset* hoist_set, struct bitset* semantic_set, source_data_t* src_data)
{
	arena_t* workpad = workpad_checkout(sizeof(struct metadata_header) + src_data->num_parse_nodes * sizeof(int32_t) + 32);
	arena_align2(workpad, 32);
	struct metadata_header* header = arena_alloc(workpad, sizeof *header);
	*header = (struct metadata_header)
	{
		.src_data = src_data,
		.hoist_set = hoist_set,
		.semantic_set = semantic_set,
	};
	int32_t* indices = arena_alloc_aligned(workpad, src_data->num_parse_nodes * sizeof *indices, 32);
	assert(((struct metadata_header*)indices) - 1 == header);
	for (int32_t i = 0; i < src_data->num_parse_nodes; ++i)
	{
		indices[i] = i;
	}
	int32_t idx = 1;
	for(; idx < src_data->num_parse_nodes && src_data->parse_depths[idx] > 0; ++idx) { /* FF to next root node */; }
	for (; idx < src_data->num_parse_nodes; ++idx)
	{
		if (src_data->parse_depths[idx] == 0 && parse_idx_to_token_id(src_data, idx) == GDSCRIPT_LINE_END)
		{
			jobs_add(check_root_node, indices + idx);
		}
	}
}

static void check_parse_header(struct metadata_header* header, const int32_t header_idx)
{
	int32_t endl_idx;
	int32_t idx;
	source_data_t* src_data = header->src_data;
	uint8_t flags = NONE;
	int32_t offset;
	int32_t len;
	while(parse_node_iterate_children(&endl_idx, src_data, header_idx), endl_idx)
	{
		if (endl_idx != GDSCRIPT_LINE_END)
		{
			parse_node_iterate_children(&idx, src_data, endl_idx);
			while (idx != nil_index)
			{
				switch(parse_idx_to_token_id(src_data, idx))
				{
				case GDSCRIPT_TOOL:
					check_annotation_tool(&flags, header, idx);
					break;
				case GDSCRIPT_ABSTRACT:
					check_annotation_abstract(&flags, header, idx);
					break;
				case GDSCRIPT_CLASS_NAME:
					if (parse_node_num_children(src_data, idx) != 1)
					{
					}
					else if (flags & HAS_CLASS_NAME)
					{
					}
					else
					{
						bitset_add(header->hoist_set, idx);
						bitset_add(header->semantic_set, idx);
						flags |= HAS_CLASS_NAME;
					}
					idx = parse_node_sibling(src_data, idx);
					continue;
				case GDSCRIPT_EXTENDS:
					if (parse_node_num_children(src_data, idx) != 1)
					{
					}
					else if (flags & HAS_EXTENDS)
					{
					}
					else
					{
						bitset_add(header->hoist_set, idx);
						bitset_add(header->semantic_set, idx);
						flags |= HAS_EXTENDS;
					}
					break;
				default:
					{
						parse_subtree_source_offset_and_len(&offset, &len, src_data, idx);
						diagnostics_add(DIAGNOSTIC_BAD_HEADER_NODE, src_data->src, src_data->src_len, src_data->path, offset, len);
						check_all_children(header, endl_idx);
						int32_t temp = parse_node_sibling(src_data, idx);
						if (temp != nil_index)
						{
							idx = temp;
							continue;
						}
					}
				}
			parse_node_iterate_children(&idx, src_data, endl_idx);
			}
		}
		else
		{
			int32_t offset;
			int32_t len;
			parse_subtree_source_offset_and_len(&offset, &len, src_data, idx);
			diagnostics_add(DIAGNOSTIC_BAD_HEADER_NODE, src_data->src, src_data->src_len, src_data->path, offset, len);
			check_all_children(header, endl_idx);
		}
	}
}


static void check_root_node(int32_t* idx_ptr)
{
	const int32_t idx = *idx_ptr;
	struct metadata_header* metadata = (struct metadata_header*)(idx_ptr - idx) - 1;
	source_data_t* src_data = metadata->src_data;
	/* if (source_data->token_ids[source_data->parse_to_token_indices[parse_idx]] != GDSCRIPT_LINE_END) */
	const token_id_t node_id = parse_idx_to_token_id(src_data, idx);
	if (node_id == GDSCRIPT_LINE_END && parse_node_num_children(src_data, idx) > 0)
	{
		const int32_t child_idx = parse_node_first_child(src_data, idx);
		switch(parse_idx_to_token_id(src_data, child_idx))
		{
			// annotations
			// const
			// var
			// func
			// signal shit
			// class
			// everything else is bad but still check the children kthx
		}
	}
	else if (node_id == PARSE_HEADER && parse_node_num_children(src_data, idx) > 0)
	{
		check_parse_header(metadata, idx);
	}
	else
	{
		int32_t offset;
		int32_t len;
		parse_subtree_source_offset_and_len(&offset, &len, src_data, idx);
		diagnostics_add(DIAGNOSTIC_BAD_TOPNODE, src_data->src, src_data->src_len, src_data->path, offset, len);
		check_all_children(metadata, idx);
	}
}
static void check_annotation_tool(uint8_t* header_flags, struct metadata_header* header, const int32_t tool_idx)
{
	/* TODO */
}

static void check_annotation_abstract(uint8_t* header_flags, struct metadata_header* header, const int32_t tool_idx)
{
	/* TODO */
}


static void check_all_children(struct metadata_header* header, const int32_t parent_idx)
{
	int32_t i;
	while (parse_node_iterate_children(&i, header->src_data, parent_idx), i)
	{
		switch(parse_idx_to_token_id(header->src_data, i))
		{
			// TODO!!!!
		}
	}
}
