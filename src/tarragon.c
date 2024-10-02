#include "tarragon.h"

#include <stdio.h>
#include <tree_sitter/api.h>

#include "str_file.h"

const TSLanguage *tree_sitter_json(void);

void init(FILE* config_fp, tarragon_t* t)
{
	// TSParser *parser = ts_parser_new();
	// ts_parser_set_language(parser, tree_sitter_json());
	// ts_parser_delete(parser);
	char* str = build_string_and_free(config_fp);
	printf("%s\n", str);
	free(str);
}

void scan(tarragon_t* t)
{
}
