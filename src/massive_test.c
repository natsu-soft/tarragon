#include <stdlib.h>
#include <stdio.h>
#include "mem_sys.h"
#include "gdscript/suite.h"
#include "benchmark.h"
#include "allocators/arena.h"

int main(int arc, char** argv)
{
	ms_handle_t src_handle;
	int src_len = 0;
	char* src = ms_read_file("Massive.gd", &src_len, &src_handle);
	arena_t* lex_arena = alloca(sizeof *lex_arena);
	arena_init(lex_arena, 8 * src_len + 4096);
	int num_tokens;
	token_table_t table = {0};
	CLOCKTIME(num_tokens = gdscript_flex(lex_arena, &table, src, src_len);)
	printf("tokens: %d, src size: %d, token_size: %d, max size: %d\n", num_tokens, src_len, num_tokens * 7, 8 * src_len + 4096);
	ms_munmap_file(src, src_len, src_handle);
	CLOCKTIME(num_tokens = diagnostics_pre_analysis(&table, src_len, "Massive.gd");)
	arena_t* shunt_arena = alloca(sizeof *shunt_arena);
	arena_init(shunt_arena, 7 * num_tokens);
	token_table_t shunt_table = {0};
	CLOCKTIME(num_tokens = gdscript_shunt(shunt_arena, &shunt_table, &table, num_tokens);)
	CLOCKTIME(num_tokens = diagnostics_pre_analysis(&shunt_table, src_len, "Massive.gd");)
	printf("tokens: %d, src size: %d, token_size: %d, max size: %d\n", num_tokens, src_len, num_tokens * 7, 8 * src_len + 4096);
	return 0;
}
