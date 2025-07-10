#include "gdscript_parser.h"
#include <stdlib.h>
#include "gdscript_lexer.h"
#include "benchmark.h"
#include "file_sys.h"
#include "scratch.h"
#include "string.h"
#include "mem_sys.h"
/* #include "token.h" */
#include "arena.h"

int16_t token_line(const token_table_t* table, const int32_t index)
{
	int16_t line = 1;
	const int32_t offset = table->offsets[index];
	const char* src = table->src;
	for (int32_t i = 0; i < offset; ++i)
	{
		line += src[i] == '\n';
	}
	return line;
}

int16_t token_column(const token_table_t* table, const int32_t index)
{
	int col = 1;
	const int32_t offset = table->offsets[index];
	const char* src = table->src;
	for (int32_t i = 0; i < offset; ++i)
	{
		const int8_t k = src[i] != '\n';
		col = k * (col + 1) + (1 - k);
	}
	return col;
}


void print_tokens(const char* src, const int32_t* offsets, const int16_t* lens, const int32_t n)
{
	for (int32_t i = 0; i < n; ++i)
	{
		const char* token = src + offsets[i];
		for (int16_t j = 0; j < lens[i]; ++j)
		{
			printf("%c", token[j]);
		}
		printf(" ");
	}
}

void print_allman(const char* src, const int32_t* offsets, const int16_t* lens, const gdscript_id_t* ids, const int32_t num_tokens)
{
	int8_t newline = 1;
	int32_t depth = 0;
	for (int32_t i = 0; i < num_tokens; ++i)
	{
		const char* token = src + offsets[i];
		if (newline)
		{
			for (int32_t j = 0; j < depth; ++j) printf("  ");
			newline = 0;
		}
		switch(ids[i])
		{
			case GDSCRIPT_ERROR:
				printf("$ERR ");
				break;
			case GDSCRIPT_LINE_CONTINUE:
				printf("$CONT");
				break;
			case GDSCRIPT_EOF:
				printf("$EOF\n");
				break;
			case GDSCRIPT_SCOPE_START:
				++depth;
				printf("{\n");
				newline = 1;
				break;
			case GDSCRIPT_SCOPE_END:
				--depth;
				printf("\b\b}\n");
				newline = 1;
				break;
			case GDSCRIPT_LINE_END:
				printf("\n");
				newline = 1;
				break;
			default:
				for (int32_t j = 0; j < lens[i]; ++j)
				{
					printf("%c", token[j]);
				}
				printf(" ");
		}
	}
}

void print_clarity(const char* src, const int32_t* offsets, const int16_t* lens, const gdscript_id_t* ids, const int32_t num_tokens)
{
	/* uint8_t newline = 1; */
	/* uint32_t depth = 0; */
	for (uint32_t i = 0; i < num_tokens; ++i)
	{
		const char* token = src + offsets[i];
		/* if (newline) */
		/* { */
		/* 	for (uint32_t j = 0; j < depth; ++j) printf("  "); */
		/* 	newline = 0; */
		/* } */
		switch(ids[i])
		{
			case GDSCRIPT_SCOPE_START:
				/* ++depth; */
				printf("++  ");
				/* newline = 1; */
				break;
			case GDSCRIPT_SCOPE_END:
				/* --depth; */
				printf("--  ");
				/* newline = 1; */
				break;
			/* case GDSCRIPT_LINE_END: */
			/* 	printf("\n"); */
			/* 	newline = 1; */
			/* 	break; */
			default:
				for (int32_t j = 0; j < lens[i]; ++j)
				{
					printf("%c", token[j]);
				}
				printf(" ");
		}
	}
}

int gdscript_parser_test(int argc, char** argv)
{
	arena_t* arena = scratch_checkout();
	/* arena_init(&arena, 22000); */
	token_table_t table = {0};
	int src_len = 0;
	char* src = fs_read("RewindAbility.gd", &src_len);
	int32_t num_tokens;
	arena_grow(arena, 8 * src_len);
	/* uint32_t num_tokens = gdscript_tokenize(&arena, &tokens, &ids, src, src_len); */
	MICROTIME(num_tokens = gdscript_lex(arena, &table, src, src_len);)
	/* for (uint32_t i = 0; i < num_tokens; ++i) */
	/* { */
	/* 	const char* token = src + table.offsets[i]; */
	/* 	gdscript_id_t id = table.ids[i]; */
	/* 	/\* char* str = alloca(t.len + 1); *\/ */
	/* 	/\* memcpy(str, t.string, t.len); *\/ */
	/* 	/\* str[t.len] = '\0'; *\/ */
	/* 	/\* uint32_t line = 1; *\/ */
	/* 	/\* uint32_t col = 1; *\/ */
	/* 	/\* for (int j = 0; j < t.pos; ++j) *\/ */
	/* 	/\* { *\/ */
	/* 	/\* 	const uint8_t newline_flag = src[j] == '\n'; *\/ */
	/* 	/\* 	line += newline_flag; *\/ */
	/* 	/\* 	col = col * (1 - newline_flag) + 1; *\/ */
	/* 	/\* } *\/ */
	/* 	/\* printf("%d:%d:(%#06x) \n", line, col, id); *\/ */
	/* 	printf("%d:%d:(%#06x) ", token_line(&table, i), token_column(&table, i), id); */
	/* 	for (int j = 0; j < table.lens[i]; ++j) */
	/* 	{ */
	/* 		printf("%c", token[j]); */
	/* 	} */
	/* 	printf("\n"); */
	/* } */
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
	/* print_allman(src, table.offsets, table.lens, table.ids, table.num_tokens); */
	/* print_tokens(src, table.offsets, table.lens, table.num_tokens); */
	return 0;
}
