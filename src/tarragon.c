#include "tarragon.h"
#include <stdio.h>
#include <tree_sitter/api.h>
/* #include "str_builder.h" */
#include "file_sys.h"

const TSLanguage *tree_sitter_json(void);

void init(FILE* config_fp, tarragon_t* t)
{
	// TSParser *parser = ts_parser_new();
	// ts_parser_set_language(parser, tree_sitter_json());
	// ts_parser_delete(parser);
	/* int config_len; */
	/* char* str = str_file(config_fp, &config_len); */
	/* printf("%s\n", str); */
	/* free(str); */
}

void scan(tarragon_t* t)
{
}
