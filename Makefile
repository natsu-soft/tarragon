CC = gcc
CCFLAGS = -Wall -Wextra
TREE_SITTER_INCLUDES = -I tree-sitter/lib/include -I tree-sitter/lib/src

all: test_json_parser

test_json_parser: test_json_parser.o tree_sitter.o json_parser.o
	$(CC_DEBUG) $(TREE_SITTER_INCLUDES) -o test-json-parser $^

tree_sitter.o:
	$(CC) $(CCFLAGS) $(TREE_SITTER_INCLUDES) -c tree-sitter/lib/src/lib.c -o tree_sitter.o
json_parser.o:
	$(CC) $(CCFLAGS) $(TREE_SITTER_INCLDUES) -c tree-sitter-json/src/parser.c -o json_parser.o
test_json_parser.o: tree_sitter.o json_parser.o
	$(CC) $(CCFLAGS) $(TREE_SITTER_INCLUDES) -c test_json_parser.c -o test_json_parser.o

clean:
	rm -rf *.o *.exe

compile_commands:
	make --always-make --dry-run all \
	| rg -w "gcc" \
	| rg -w '\-c' \
	| jq -nR '[inputs|{command:., directory:".", file: match("\\-c [\\w\\-/]+\\.c").string[3:], output: match("\\-o [\\w\\-/]+\\.o").string[3:]}]' > compile_commands.json
