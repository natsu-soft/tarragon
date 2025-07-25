#include "gdscript_parser.h"
#include <stdio.h>
#include "gdscript_ids.h"
#include "token.h"
#include <stdlib.h>
#include "gdscript_lexer.h"
#include "flexer.h"
#include "diagnostics.h"
#include "gdscript_shunt.h"
#include "../benchmark.h"
#include "../file_sys.h"
#include "../allocators/scratch.h"
#include "string.h"
#include "../mem_sys.h"
/* #include "token.h" */
#include "../allocators/arena.h"



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
			for (int32_t j = 0; j < depth; ++j) printf("    ");
			newline = 0;
		}
		const gdscript_id_t id = ids[i];
		switch(id)
		{
		/* case GDSCRIPT_ERROR: */
		/* 	printf("$ERR "); */
		/* 	break; */
		case GDSCRIPT_LINE_CONTINUE:
			printf("$CONT");
			break;
		case GDSCRIPT_EOF:
			printf("$EOF\n");
			break;
		case GDSCRIPT_FAKE_SCOPE_START: __attribute__((fallthrough));
		case GDSCRIPT_SCOPE_START:
			/* printf("%d\n", depth); */
			printf("{\n");
			/* printf("{"); */
			++depth;
			newline = 1;
			break;
		case GDSCRIPT_FAKE_SCOPE_END: __attribute__((fallthrough));
		case GDSCRIPT_SCOPE_END:
			--depth;
			/* printf("\b\b\b\b%d", depth); */
			printf("\b\b\b\b}");
			/* printf("}"); */
			newline = 1;
			break;
		case GDSCRIPT_LINE_END:
			printf("\n");
			newline = 1;
			break;
		default:
			if (id < GDSCRIPT_NUM_BUILTIN)
			{
				switch (id)
				{
#define X(ID, STR,...) case GDSCRIPT_##ID: printf(STR); break;
					GDSCRIPT_BUILTIN_TOKENS
#undef X
				}
			}
			else if (id < GDSCRIPT_ERROR_THRESHOLD)
			{
				for (int32_t j = 0; j < lens[i]; ++j)
				{
					printf("%c", token[j]);
				}
			}
			else
			{
				switch (id)
				{
#define X(ID, ...) case GDSCRIPT_##ID: printf("#ERR_" #ID); break;
					GDSCRIPT_ERRORS
#undef X
				}
			}
			printf(" ");
		}
	}
}

void print_ids(const char* src, const int32_t* offsets, const int16_t* lens, const gdscript_id_t* ids, const int32_t num_tokens)
{
	int8_t newline = 1;
	int32_t depth = 0;
	for (int32_t i = 0; i < num_tokens; ++i)
	{
		if (newline)
		{
			for (int32_t j = 0; j < depth; ++j) printf("    ");
			newline = 0;
		}
		const gdscript_id_t id = ids[i];
		if (id == GDSCRIPT_LINE_END)
		{
			printf("\n");
			newline = 1;
			continue;
		}
		switch(id)
		{
			#define X(ID,...) case GDSCRIPT_##ID: printf(#ID " "); break;
			GDSCRIPT_BUILTIN_TOKENS
			#undef X
			/* case GDSCRIPT_ERROR: */
			/* 	printf("$ERR "); */
			/* 	break; */
			case GDSCRIPT_EOF:
				printf("EOF\n");
				break;
			case GDSCRIPT_SCOPE_START:
				++depth;
				printf("{\n");
				newline = 1;
				break;
			case GDSCRIPT_SCOPE_END:
				--depth;
				printf("\b\b\b\b}\n");
				newline = 1;
				break;
			default:
				{
					const int16_t len = lens[i];
					const char* token = src + offsets[i];
					if (id == GDSCRIPT_IDENTIFIER)
					{
						printf("(var ");
					}
					else if (id == GDSCRIPT_NUMBER)
					{
						printf("(num ");
					}
					else if (id == GDSCRIPT_STRING)
					{
						printf("(str ");
					}
					#define X(ID) else if (id == GDSCRIPT_##ID) { printf("(error " #ID " "); }
					GDSCRIPT_ERRORS
					#undef X
					for (int16_t j = 0; j < len; ++j)
					{
						printf("%c", token[j]);
					}
					printf(") ");
				}
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
	arena_t* flex_arena = scratch_checkout();
	/* arena_init(&arena, 22000); */
	token_table_t table = {0};
	int src_len = 0;
	/* char* src = fs_read("ShuntTest.gd", &src_len); */
	char* src = fs_read("RewindAbility.gd", &src_len);
	int32_t num_tokens;
	arena_grow(flex_arena, 8 * src_len + 4096);
	MICROTIME(num_tokens = gdscript_lex(flex_arena, &table, src, src_len);)
	MICROTIME(num_tokens = gdscript_flex(flex_arena, &table, src, src_len);)
	printf("tokens: %d, src size: %d, token_size: %d, max size: %d\n", num_tokens, src_len, num_tokens * 7, 8 * src_len + 4096);
	/* print_ids(src, table.offsets, table.lens, table.ids, table.num_tokens); */
	MICROTIME(num_tokens = diagnostics_pre_analysis(&table, src_len, "RewindAbility.gd");)
	arena_t* shunt_arena = scratch_checkout_size(7 * num_tokens);
	/* print_allman(src, table.offsets, table.lens, table.ids, table.num_tokens); */
	token_table_t shunt_table = {};
	MICROTIME(num_tokens = gdscript_shunt(shunt_arena, &shunt_table, &table, num_tokens);)
	scratch_checkin(flex_arena);
	printf("tokens: %d, src size: %d, token_size: %d, max size: %d\n", num_tokens, src_len, num_tokens * 7, 8 * src_len + 4096);
	MICROTIME(num_tokens = diagnostics_pre_analysis(&shunt_table, src_len, "RewindAbility.gd");)
	printf("tokens: %d, src size: %d, token_size: %d, max size: %d\n", num_tokens, src_len, num_tokens * 7, 8 * src_len + 4096);
	print_allman(src, shunt_table.offsets, shunt_table.lens, shunt_table.ids, num_tokens);
	return 0;
}
