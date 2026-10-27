#include "suite.h"
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include "lex.h"
#include "diagnostics.h"
#include "parse.h"
#include "token_ids.h"
#include "token_ids_types.h"
#include "tokens_types.h"
#include "flexer.h"
#include "gdscript_shunt.h"
#include "user_api.h"
#include "../benchmark.h"
#include "../file_sys.h"
#include "../allocators/arena.h"
#include "../allocators/scratch.h"
#include "../allocators/workpad.h"
#include "strings.h"
#include "../memery.h"

static void print_parse_table(const char* src, const source_data_t* src_data)
{
	int16_t* depths = src_data->parse_depths;
	int32_t* index = src_data->parse_to_token_indices + 1;
	for (int32_t i = 1; i < src_data->num_parse_nodes; ++i, ++index)
	{
		if (depths[i] == 0)
		{
			struct parse_node* node = parse_node(scratch_arena(4096), src_data, i);
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

void print_tokens(const char* src, const int32_t* offsets, const int16_t* lens, const int32_t n)
{
	for (int32_t i = TOKEN_START; i < n; ++i)
	{
		printf("%.*s ", lens[i], src + offsets[i]);
	}
}

void print_allman(const char* src, const int32_t* offsets, const int16_t* lens, const token_id_t* ids, const int32_t num_tokens)
{
	int8_t newline = 1;
	int32_t depth = 0;
	for (int32_t i = TOKEN_START; i < num_tokens; ++i)
	{
		if (newline)
		{
			for (int32_t j = 0; j < depth; ++j) printf("    ");
			newline = 0;
		}
		/* const char* token = src + offsets[i]; */
		const token_id_t id = ids[i];
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
		case GDSCRIPT_SCOPE_OPEN:
			/* printf("%d\n", depth); */
			putc('\n', stdout);
			for (int32_t j = 0; j < depth; ++j) printf("    ");
			putc('{', stdout);
			putc('\n', stdout);
			/* printf("{\n"); */
			/* printf("{"); */
			++depth;
			newline = 1;
			break;
		case GDSCRIPT_SCOPE_CLOSE:
			--depth;
			/* printf("\b\b\b\b%d", depth); */
			printf("\b\b\b\b} ");
			/* printf("}"); */
			newline = 0;
			break;
		case GDSCRIPT_LINE_END:
			printf("$ENDL\n");
			newline = 1;
			break;
		case GDSCRIPT_SEMICOLON:
			printf(";\n");
			/* printf("$SEMIC\n"); */
			newline = 1;
			break;
		case GDSCRIPT_STRUCTURAL_COLON:
			printf(":: ");
			break;
		case GDSCRIPT_FAKE_SCOPE_START:
			putc('\n', stdout);
			for (int32_t j = 0; j < depth; ++j) printf("    ");
			printf("[[\n");
			++depth;
			newline = 1;
			break;
		case GDSCRIPT_FAKE_SCOPE_END:
			--depth;
			/* printf("\b\b\b\b%d", depth); */
			printf("\b\b\b\b]] ");
			/* printf("}"); */
			newline = 0;
			break;
		case GDSCRIPT_OP_NEGATION:
			printf("-unary ");
			break;
		case GDSCRIPT_OP_IDENTITY:
			printf("+unary ");
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
				printf("%.*s", lens[i], src + offsets[i]);
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

void print_ids(const char* src, const int32_t* offsets, const int16_t* lens, const token_id_t* ids, const int32_t num_tokens)
{
	int8_t newline = 1;
	int32_t depth = 0;
	for (int32_t i = TOKEN_START; i < num_tokens; ++i)
	{
		if (newline)
		{
			for (int32_t j = 0; j < depth; ++j) printf("    ");
			newline = 0;
		}
		const token_id_t id = ids[i];
		if (id == GDSCRIPT_LINE_END)
		{
			printf("$ENDL\n");
			newline = 1;
			continue;
		}
		else
		{
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
				case GDSCRIPT_SCOPE_OPEN:
					++depth;
					printf("{\n");
					newline = 1;
					break;
				case GDSCRIPT_SCOPE_CLOSE:
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
							printf("(var %.*s) ", lens[i], src + offsets[i]);
						}
						else if (id == GDSCRIPT_NUMBER)
						{
							printf("(num %.*s) ", lens[i], src + offsets[i]);
						}
						else if (id == GDSCRIPT_STRING)
						{
							printf("(str %.*s) ", lens[i], src + offsets[i]);
						}
						#define X(ID) else if (id == GDSCRIPT_##ID) { printf("(error " #ID ")"); }
						GDSCRIPT_ERRORS
						#undef X
					}
			}
		}
	}
}


void print_clarity(const char* src, const int32_t* offsets, const int16_t* lens, const token_id_t* ids, const int32_t num_tokens)
{
	/* uint8_t newline = 1; */
	/* uint32_t depth = 0; */
	for (uint32_t i = TOKEN_START; i < num_tokens; ++i)
	{
		const char* token = src + offsets[i];
		/* if (newline) */
		/* { */
		/* 	for (uint32_t j = 0; j < depth; ++j) printf("  "); */
		/* 	newline = 0; */
		/* } */
		switch(ids[i])
		{
			case GDSCRIPT_SCOPE_OPEN:
				/* ++depth; */
				printf("++  ");
				/* newline = 1; */
				break;
			case GDSCRIPT_SCOPE_CLOSE:
				/* --depth; */
				printf("--  ");
				/* newline = 1; */
				break;
			/* case GDSCRIPT_LINE_END: */
			/* 	printf("\n"); */
			/* 	newline = 1; */
			/* 	break; */
			default:
				printf("%.*s ", lens[i], src + offsets[i]);
				/* for (int32_t j = 0; j < lens[i]; ++j) */
				/* { */
				/* 	printf("%c", token[j]); */
				/* } */
				/* printf(" "); */
		}
	}
}

int gdscript_suite_test(int argc, char** argv)
{
	workpad_checkin_all();
	int src_len = 0;
	char* src = fs_read("RewindAbility.gd", &src_len);
	arena_t* flex_arena = workpad_checkout(8 * src_len + 4096);
	printf("src_len: %d\n", src_len);
	printf("flex size: %d, flex_cap: %d\n", flex_arena->size, flex_arena->cap);
	assert(flex_arena->size == 0 && flex_arena->cap >= 8 * src_len + 4096 && "OOPSIES!");
	token_table_t table = {0};
	int32_t num_tokens;
	token_table_t new_table = {};
	arena_t* lex_arena = workpad_checkout(lex_backing_size(src_len));
	arena_t* lex_work = workpad_checkout(lex_work_size(src_len));
	source_data_t source_data = {.src = src, .path = "RewindAbility.gd", .src_len = src_len };
	virtual_buffer vb = memalloc(10 * MB);
	MICROTIME(num_tokens = lex(&source_data, lex_arena, lex_work, src, src_len);)
	if(num_tokens == 0)
	{
		flush_diagnostics();
		return 0;
	}
	printf("tokens: %d, src size: %d, token_size: %d, max size: %d\n", num_tokens, src_len, num_tokens * 7, 8 * src_len + 4096);
	/* print_tokens(src, source_data.token_offsets, source_data.token_lens, source_data.num_tokens); */
	scratch_alloc(4 * src_len);
	MICROTIME(num_tokens = gdscript_flex(flex_arena, &table, src, src_len);)
	printf("tokens: %d, src size: %d, token_size: %d, max size: %d\n", num_tokens, src_len, num_tokens * 7, 8 * src_len + 4096);
	/* print_tokens(src, table.offsets, table.lens, table.num_tokens); */
	arena_t* p_arena = workpad_checkout(parse_backing_size(num_tokens));
	MICROTIME(num_tokens = parse(&source_data, p_arena, lex_work);)
	printf("parse num tokens: %d\n", num_tokens);
	print_parse_table(src, &source_data);
	return 0;
}
