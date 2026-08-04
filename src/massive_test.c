#include <stdlib.h>
#include <stdio.h>
#include "mem_sys.h"
#include "gdscript/lex.h"
#include "gdscript/parse.h"
#include "benchmark.h"
#include "allocators/arena.h"

#define KB 1024
#define MB 1048576

int main(int arc, char** argv)
{
	ms_handle_t src_handle;
	int src_len = 0;
	char* src = ms_read_file("Massive.gd", &src_len, &src_handle);
	arena_t* lex_arena = alloca(sizeof *lex_arena);
	arena_t* work = alloca(sizeof *lex_arena);
	arena_init(lex_arena, lex_backing_size(src_len));
	arena_init(work, lex_work_size(src_len));
	int num_tokens;
	token_table_t token_table = {0};
	CLOCKTIME(num_tokens = lex(&token_table, lex_arena, work, src, src_len, NULL);)
		printf("tokens: %d, src size: %dmB, token_size: %dmB, max_size: %dmB\n", num_tokens, src_len / MB, (7 * num_tokens) / MB, lex_arena->cap / MB);
	parse_table_t parse_table = {0};
	arena_t* parse_arena = alloca(sizeof *parse_arena);
	arena_init(parse_arena, parse_backing_size(num_tokens));
	CLOCKTIME(num_tokens = parse(&parse_table, &token_table, parse_arena, work);)
	printf("tokens: %d, parse table size: %dmB, max_size: %dmB\n", num_tokens, 6 * num_tokens / MB, parse_arena->cap / MB);
	ms_munmap_file(src, src_len, src_handle);
	return 0;
}
