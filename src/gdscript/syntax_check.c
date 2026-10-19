#include "syntax_check.h"
#include "../allocators/workpad.h"
#include "../job.h"

struct syntax_check_args
{
	parse_table_t* parse_table;
	token_table_t* token_table;
	struct syntax_check_args* args;
	int32_t parse_index;
};

static void syntax_check_node(struct syntax_check_args* args, struct thread_context* ctx);

void syntax_check(parse_table_t* parse_table, token_table_t* token_table)
{
	struct syntax_check_args* args = workpad_alloc(parse_table->size * sizeof *args);
	for (int32_t i = 0; parse_table->size; ++i)
	{
		args[i] = (struct syntax_check_args)
		{
			.parse_table = parse_table,
			.token_table = token_table,
			.args = args,
			.parse_index = i
		};
		if (parse_table->depths[i] == 0)
		{
			jobs_add(, )
			syntax_check_node(args + i, )
		}
	}
}
