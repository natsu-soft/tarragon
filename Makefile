CC = gcc
CFLAGS = -Wall -Wextra
TREE-SITTER_INCLUDES = -I tree-sitter/lib/include -I tree-sitter/lib/src

all: test-json-parser arg-parser

test-json-parser: test_json_parser.o tree_sitter.o json_parser.o
	$(CC) $(CFLAGS) $(TREE-SITTER_INCLUDES) -o $@ $^
tree_sitter.o:
	$(CC) $(CFLAGS) $(TREE-SITTER_INCLUDES) -c tree-sitter/lib/src/lib.c -o $@
json_parser.o:
	$(CC) $(CFLAGS) $(C_INCLDUES) -c tree-sitter-json/src/parser.c -o $@
test_json_parser.o: tree_sitter.o json_parser.o
	$(CC) $(CFLAGS) $(TREE-SITTER_INCLUDES) -c test_json_parser.c $< -o $@

arg-parser: arg_parser.o src/find_file.o
	$(CC) $(CFLAGS) -ggdb -o $@ $^

arg_parser.o:
	$(CC) $(CFLAGS) -c arg_parser.c -o $@

# src/%.o: src/%.c
# 	$(CC) $(CFLAGS) -c $^ -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $^ -o $@

test: arg-parser
	${CURDIR}/arg-parser.exe

clean:
	rm -rf *.o src/*.o *.exe

compile_commands:
	make --always-make --dry-run all \
	| rg -w "gcc" \
	| rg -w '\-[co]' \
	| jq -nR '[inputs|{command:., directory:"${CURDIR}", file: match("\\-c [\\w\\-/]+\\.c").string[3:], output: match("\\-o [\\w\\-/]+\\.o").string[3:]}]' > compile_commands.json
