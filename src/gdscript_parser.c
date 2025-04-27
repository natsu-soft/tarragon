#include "gdscript_parser.h"
/* #include <stdlib.h> */
#include "gdscript_tokenizer.h"
#include "benchmark.h"
#include "file_sys.h"
#include "mem_sys.h"
#include "token.h"
#include "arena.h"

int main(int argc, char** argv)
{
	arena_t arena = {0};
	/* arena_init(&arena, 22000); */
	token_t* tokens;
	gdscript_id_t* ids;
	int src_len = 0;
	char* src = fs_read("RewindAbility.gd", &src_len);
	uint32_t num_tokens;
	/* uint32_t num_tokens = gdscript_tokenize(&arena, &tokens, &ids, src, src_len); */
	MICROTIME(num_tokens = gdscript_tokenize(&arena, &tokens, &ids, src, src_len);)
	const uint32_t n0 = num_tokens;
	printf("num_tokens0: %d\n", num_tokens);
	MICROTIME(preprocess_tokens(tokens, ids, &num_tokens);)
	printf("num_tokens1: %d\n", num_tokens);
	for (uint32_t i = 0; i < num_tokens; ++i)
	{
		token_t t = tokens[i];
		gdscript_id_t id = ids[i];
		/* printf("%d:%d:(%#06x) %s\n", t.line, t.col, id, t.string); */
	}
	printf("SIZE: %zu\n", arena.cap);
	printf("GDSCRIPT_EOF: %#06x\n", GDSCRIPT_EOF);
	printf("GDSCRIPT_LEADING_WHITESPACE: %#06x\n", GDSCRIPT_LEADING_WHITESPACE);
	printf("GDSCRIPT_WHITESPACE: %#06x\n", GDSCRIPT_WHITESPACE);
	/* printf("GDSCRIPT_SCOPE_START: %#06x\n", GDSCRIPT_SCOPE_START); */
	/* printf("GDSCRIPT_SCOPE_END: %#06x\n", GDSCRIPT_SCOPE_END); */
	return 0;
}
