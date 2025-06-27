#include "gdscript_parser.h"
#include <stdlib.h>
#include "gdscript_tokenizer.h"
#include "benchmark.h"
#include "file_sys.h"
#include "scratch.h"
#include "string.h"
#include "mem_sys.h"
#include "token.h"
#include "arena.h"

int gdscript_parser_test(int argc, char** argv)
{
	arena_t* arena = scratch_checkout();
	/* arena_init(&arena, 22000); */
	token_t* tokens;
	gdscript_id_t* ids;
	int src_len = 0;
	char* src = fs_read("RewindAbility.gd", &src_len);
	uint32_t num_tokens;
	/* uint32_t num_tokens = gdscript_tokenize(&arena, &tokens, &ids, src, src_len); */
	MICROTIME(num_tokens = gdscript_tokenize(arena, &tokens, &ids, src, src_len);)
/* 	const uint32_t n0 = num_tokens; */
/* 	for (uint32_t i = 0; i < num_tokens; ++i) */
/* 	{ */
/* 		token_t t = tokens[i]; */
/* 		gdscript_id_t id = ids[i]; */
/* 		char* str = alloca(t.len + 1); */
/* 		memcpy(str, t.string, t.len); */
/* 		str[t.len] = '\0'; */
/* 		uint32_t line = 1; */
/* 		uint32_t col = 1; */
/* 		for (int j = 0; j < t.pos; ++j) */
/* 		{ */
/* 			const uint8_t newline_flag = src[j] == '\n'; */
/* 			line += newline_flag; */
/* 			col = col * (1 - newline_flag) + 1; */
/* 		} */
/* 		printf("%d:%d:(%#06x) %s\n", line, col, id, str); */
/* 	} */
/* 	printf("SIZE: %zu, CAP: %zu\n", arena->size, arena->cap); */
/* #define PRINT(A) printf("%#04x: " #A "\n", A) */
/* 	PRINT(GDSCRIPT_COMMENT); */
/* 	PRINT(GDSCRIPT_OP_DOT); */
/* 	PRINT(GDSCRIPT_PERIOD); */
/* 	PRINT(GDSCRIPT_EOF); */
/* 	PRINT(GDSCRIPT_NUMBER); */
/* 	PRINT(GDSCRIPT_IDENTIFIER); */
/* 	PRINT(GDSCRIPT_NUMBER_CANDIDATE); */
/* 	PRINT(GDSCRIPT_LEADING_WHITESPACE); */
/* 	PRINT(GDSCRIPT_WHITESPACE); */
/* 	PRINT(GDSCRIPT_SCOPE_START); */
/* 	PRINT(GDSCRIPT_SCOPE_END); */
/* 	#undef PRINT */
	gdscript_clarity(tokens, ids, num_tokens);
	return 0;
}
