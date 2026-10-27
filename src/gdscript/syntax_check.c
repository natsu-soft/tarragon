#include "syntax_check.h"
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

// NOTE: passing in ptrs to index because it also doubles as the offset - 1 to script_data
static bool check_annotation_tool(uint32_t* flags, source_data_t* src_data);
static bool check_annotation_abstract(uint32_t* flags);
static void check_top_node(int32_t* idx_ptr, struct thread_context* ctx);
static void check_top_annotation(int32_t* idx_ptr, struct thread_context* ctx);
static void check_annotation_node(int32_t* idx_ptr, struct thread_context* ctx);

enum
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
	_Atomic(uint32_t) flags;
};

void syntax_check(struct bitset* hoist_set, struct bitset* semantic_set, source_data_t* src_data)
{
	arena_t* workpad = workpad_checkout(sizeof(struct metadata_header) + src_data->num_parse_nodes * sizeof(int32_t) + 32);
	struct metadata_header* header = arena_alloc(workpad, sizeof *header);
	*header = (struct metadata_header)
	{
		.src_data = src_data,
		.hoist_set = hoist_set,
		.semantic_set = semantic_set,
	};
	store_ex(&header->flags, NONE, release);
	int32_t* indices = arena_alloc_aligned(workpad, src_data->num_parse_nodes * sizeof *indices, 32);
	assert(((struct metadata_header*)indices) - 1 == header);
	for (int32_t i = 0; i < src_data->num_parse_nodes; ++i)
	{
		indices[i] = i;
	}
	int32_t idx = 1;
	for (; idx < src_data->num_parse_nodes; idx = parse_node_sibling(src_data, idx))
	{
		if (parse_idx_to_token_id(src_data, idx) == GDSCRIPT_LINE_END)
		{
			int32_t child_idx = 0;
			while (parse_node_iterate_children(&child_idx, src_data, idx), child_idx > 0)
			{
				switch(parse_idx_to_token_id(src_data, child_idx))
				{
				case GDSCRIPT_TOOL:
					if (fetch_or_ex(&header->flags, HAS_TOOL, release) != NONE)
					{
						int32_t offset;
						int16_t len;
						parse_subtree_source_offset_and_len(&offset, &len, src_data, child_idx);
						diagnostics_add(DIAGNOSTIC_NON_LEADING_TOOL, src_data->src, src_data->src_len, src_data->path, offset, len);
					}
					// if tool has children it should check them
					bitset_add(semantic_set, child_idx);
					// check tool
					break;
				case GDSCRIPT_ABSTRACT:
					if (fetch_or_ex(&header->flags, HAS_ABSTRACT, acq_rel) & HAS_ABSTRACT)
					{
						int32_t offset;
						int16_t len;
						parse_subtree_source_offset_and_len(&offset, &len, src_data, child_idx);
						/* diagnostics_add(DIAGNOSTIC_MULTIPLE_CLASS_NAMES, src_data->src, src_data->src_len, src_data->path, offset, len); */
					}
					break;
				case GDSCRIPT_CLASS_NAME:
					if (fetch_or_ex(&header->flags, HAS_CLASS_NAME, acq_rel) & HAS_CLASS_NAME)
					{
						int32_t offset;
						int16_t len;
						parse_subtree_source_offset_and_len(&offset, &len, src_data, child_idx);
						diagnostics_add(DIAGNOSTIC_MULTIPLE_CLASS_NAMES, src_data->src, src_data->src_len, src_data->path, offset, len);
						// DO NOT HOIST
					}
					break;
				case GDSCRIPT_EXTENDS:
					if (fetch_or_ex(&header->flags, HAS_EXTENDS, acq_rel) & HAS_EXTENDS)
					{
						int32_t offset;
						int16_t len;
						parse_subtree_source_offset_and_len(&offset, &len, src_data, child_idx);
						diagnostics_add(DIAGNOSTIC_MULTIPLE_CLASS_NAMES, src_data->src, src_data->src_len, src_data->path, offset, len);
					}
					break;
				default:
					goto parallel_check;
				}
			}
		}
		else
		{
			int32_t offset;
			int16_t len;
			parse_subtree_source_offset_and_len(&offset, &len, src_data, idx);
			diagnostics_add(DIAGNOSTIC_BAD_TOPNODE, src_data->src, src_data->src_len, src_data->path, offset, len);
		}
	}
parallel_check:
	for (; idx < src_data->num_parse_nodes; ++idx)
	{
		if (src_data->parse_depths[idx] == 0)
		{
			jobs_add(indices + idx, check_top_node);
		}
	}
	jobs_sync();
}


static void check_top_node(int32_t* idx_ptr, struct thread_context* ctx)
{
	const int32_t idx = *idx_ptr;
	struct metadata_header* metadata = (struct metadata_header*)(idx_ptr - idx) - 1;
	source_data_t* src_data = &metadata->src_data;
	/* if (source_data->token_ids[source_data->parse_to_token_indices[parse_idx]] != GDSCRIPT_LINE_END) */
	if (parse_idx_to_token_id(src_data, idx) != GDSCRIPT_LINE_END)
	{
		int32_t offset;
		int16_t len;
		parse_subtree_source_offset_and_len(&offset, &len, src_data, idx);
		diagnostics_add(DIAGNOSTIC_BAD_TOPNODE, src_data->src, src_data->src_len, src_data->path, offset, len);
	}
	int32_t child_idx = parse_node_first_child(src_data, idx);
	token_id_t id = parse_idx_to_token_id(src_data, child_idx);
	if (annotation_p(id))
	{
	}
	else if (id == GDSCRIPT_VAR || id == GDSCRIPT_CONST)
	{
	}
	else if (id == GDSCRIPT_FUNC)
	{
	}
	else if (id == GDSCRIPT_EXTENDS)
	{
		int32_t num_children = parse_node_num_children(src_data, idx);
		// check if either either an identifer or a string
	}
	else if (id == GDSCRIPT_CLASS_NAME || id == GDSCRIPT_CLASS)
	{
		child_idx = parse_node_sibling(src_data, child_idx);
		if (parse_idx_to_token_id(src_data, child_idx) != GDSCRIPT_IDENTIFIER)
		{
			int32_t offset;
			int16_t len;
			parse_subtree_source_offset_and_len(&offset, &len, src_data, child_idx);
			diagnostics_add(DIAGNOSTIC_BAD_CLASS_NAME,src_data->src, src_data->src_len, src_data->path, offset, len);
		}
		if ((child_idx = parse_node_sibling(src_data, child_idx)) != 0)
		{
			const int32_t offset0 = parse_subtree_source_offset0(src_data, child_idx);
			const int32_t offset1 = parse_idx_to_token_offset(src_data, idx);
			diagnostics_add(DIAGNOSTIC_TOKENS_AFTER_CLASS_NAME,src_data->src, src_data->src_len, src_data->path, offset0, offset1 - offset0);
		}
	}
	else
	{
		int32_t offset;
		int16_t len;
		parse_subtree_source_offset_and_len(&offset, &len, src_data, idx);
		switch(id)
		{
#define X(ID,...)				\
			case GDSCRIPT_##ID: \
				diagnostics_add(DIAGNOSTIC_BAD_TOPNODE, src_data->src, src_data->src_len, src_data->path, offset, len); \
				break;
			GDSCRIPT_BUILTIN_TOKENS
			GDSCRIPT_NONKEYWORD_LITERALS
#undef X
		case GDSCRIPT_NUMBER:
				diagnostics_add(DIAGNOSTIC_BAD_TOPNODE, src_data->src, src_data->src_len, src_data->path, offset, len);
			break;
		case GDSCRIPT_IDENTIFIER:
				diagnostics_add(DIAGNOSTIC_BAD_TOPNODE, src_data->src, src_data->src_len, src_data->path, offset, len);
			break;
		}
	}
}
