#include "syntax_check.h"
#include <string.h>
#include <stdatomic.h>
#define DEFINE_TOKEN_PREDICATES
#include "token_ids_types.h"
#undef DEFINE_TOKEN_PREDICATES
#include "source_data.h"
#include "diagnostics.h"
#include "../allocators/arena.h"
#include "../allocators/scratch.h"
#include "../allocators/workpad.h"
#include "../job.h"

// NOTE: passing in ptrs to index because it also doubles as the offset - 1 to script_data
static void check_top_node(int32_t* idx_ptr, struct thread_context* ctx);
static void check_top_annotation(int32_t* idx_ptr, struct thread_context* ctx);
static void check_annotation_node(int32_t* idx_ptr, struct thread_context* ctx);

enum
{
	NONE			= 0x00,
	HAS_CLASS_NAME	= 0x01,
	HAS_EXTENDS		= 0x02,
	HAS_ABSTRACT	= 0x04,
	HAS_TOOL		= 0x08,
	BELOW_TOP		= 0x80
};

struct metadata_header
{
	source_data_t src_data;
	_Atomic(uint8_t) src_flags;
};

void syntax_check(source_data_t* src_data)
{
	struct metadata_header* metadata;
	arena_t* workpad = workpad_checkout(sizeof *metadata + src_data->num_parse_nodes * sizeof(int32_t));
	metadata = arena_alloc(workpad, sizeof *metadata);
	*metadata = (struct metadata_header){ .src_data = *src_data };
	int32_t* indices = arena_alloc(workpad, src_data->num_parse_nodes * sizeof *indices);
	for (int32_t i = 0; src_data->num_parse_nodes; ++i)
	{
		indices[i] = i;
	}
	for (int32_t i = 1; i < src_data->num_parse_nodes; ++i)
	{
		if (src_data->parse_depths[i] == 0)
		{
			jobs_add(indices + i, check_top_node);
		}
	}
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
