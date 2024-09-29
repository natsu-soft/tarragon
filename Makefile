test_json_parser:
	gcc                                   \
	-I tree-sitter/lib/src                \
	-I tree-sitter/lib/include            \
	test_json_parser.c                    \
	tree-sitter/lib/src/lib.c             \
	tree-sitter-json/src/parser.c         \
	-o test-json-parser

# ./test-json-parser
