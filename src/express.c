#include <stdio.h>
#include <unistd.h>
#define LOG_FILE_HERE
#include "logger.h"
#include "job.h"
#include "file_sys.h"
#include "stdout_ring.h"
#include "allocators/scratch.h"
#include "allocators/workpad.h"
#include "gdscript/source_data.h"
#include "gdscript/token_ids_types.h"
#include "gdscript/lex.h"
#include "gdscript/parse.h"
#include "gdscript/diagnostics.h"

static void print_parse_arrays(const char* src, const source_data_t* src_data);
static void print_parse_table(const char* src, const source_data_t* src_data);

int main(int argc, char** argv)
{
	int32_t ret = 0;
	LOG_INIT("express.log");
	jobs_init(4, 4096);
	jobs_add_initializer(stdout_ring_init);
	jobs_add_initializer(diagnostics_init);
	jobs_add_initializer(scratch_init);
	jobs_add_initializer(workpad_init);
	jobs_add_initializer(lex_init);
	jobs_sync();
	int32_t src_len;
	char* src = fs_read("RewindAbility.gd", &src_len);
	arena_t* lex_arena = workpad_checkout(lex_backing_size(src_len));
	arena_t* lex_work = workpad_checkout(lex_work_size(src_len));
	source_data_t src_data = { .src = src, .path = "RewindAbility.gd", .src_len = src_len };
	int32_t num = lex(&src_data, lex_arena, lex_work, src, src_len);
	flush_diagnostics();
	if (num == 0)
	{
		ret = 1;
		goto exit;
	}
	arena_t* p_arena = workpad_checkout(parse_backing_size(num));
	num = parse(&src_data, p_arena, lex_work);
	print_parse_arrays(src, &src_data);
	print_parse_table(src, &src_data);
exit:
	return ret;
	sleep(1);
	jobs_finish();
	LOG_END();
	// cleanup all the rings PLEASE
	return ret;
}

static void print_parse_arrays(const char* src, const source_data_t* src_data)
{
	int16_t* depths = src_data->parse_depths;
	int32_t* index = src_data->parse_to_token_indices + 1;
	for (int32_t i = 1; i < src_data->num_parse_nodes; ++i, ++index)
	{
		switch(parse_idx_to_token_id(src_data, i))
		{
#define X(ID) \
			case PARSE_##ID:					\
				printf("%d:<" #ID ">, ", depths[i]); \
				break;
			PARSE_IDS
	#undef X
		case GDSCRIPT_LINE_END:
			printf("%d:\n", depths[i]);
			break;
		default:
			printf("%d:%.*s, ", depths[i], parse_idx_to_token_len(src_data, i), src + parse_idx_to_token_offset(src_data, i));
		}
	}
}

static void print_parse_table(const char* src, const source_data_t* src_data)
{
	int16_t* depths = src_data->parse_depths;
	int32_t* index = src_data->parse_to_token_indices + 1;
	for (int32_t i = 1; i < src_data->num_parse_nodes; ++i, ++index)
	{
		if (depths[i] == 0)
		{
			struct parse_node* node = parse_node(scratch_arena(16384), src_data, i);
			/* parse_node_sexp(node, token_table); */
			parse_node_sexp(node, src_data);
			printf("\n\n");
		}
		else if (0)
		{
				/* printf("%d:", depths[i]); */
			switch(src_data->token_ids[*index])
			{
			case GDSCRIPT_NONE: printf("[ NONE ] "); break;
			#define X(A) \
			case PARSE_##A: \
				printf("[ PARSE_" #A " ] "); \
				break;
			PARSE_IDS
			#undef X
			case GDSCRIPT_LINE_END: putc('\n', stdout); break;
			case GDSCRIPT_MISSING_PAREN_CLOSE: printf("missing:) "); break;
			case GDSCRIPT_MISSING_BRACE_CLOSE: printf("missing:} "); break;
			case GDSCRIPT_MISSING_BRACKET_CLOSE: printf("missing:] "); break;
			default:
				printf("%.*s ", src_data->token_lens[index[i]], src + src_data->token_offsets[index[i]]);
			}
		}
	}
}
