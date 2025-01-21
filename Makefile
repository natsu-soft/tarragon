CC := gcc
CFLAGS := -Wall -Wextra -ggdb3 -O2
SRC := $(wildcard src/*.c)
TREE-SITTER_INCLUDES = -I tree-sitter/lib/include -I tree-sitter/lib/src
TREE_SITTER_OBJ := ts_obj/tree_sitter.o ts_obj/json_parser.o ts_obj/cpp_parser.o ts_obj/cpp_scanner.o
OBJ := $(patsubst src/%.c,obj/%.o,$(SRC)) $(TREE_SITTER_OBJ)

.PHONY: default test clean deep-clean obj_dirs compile_commands

default: tarragon

tarragon: $(OBJ)
	$(CC) $(CFLAGS) -ggdb -o $@ $^

test-json-parser: test_json_parser.o tree_sitter.o json_parser.o
	$(CC) $(TREE-SITTER_INCLUDES) -o $@ $^

ts_obj/tree_sitter.o: | obj_dirs
	$(CC) $(CFLAGS) $(TREE-SITTER_INCLUDES) -c tree-sitter/lib/src/lib.c -o $@

ts_obj/%_parser.o: | obj_dirs
	$(CC) $(CFLAGS) $(TREE-SITTER_INCLDUES) -c tree-sitter-$*/src/parser.c -o $@ $(TARGET_OPT)

ts_obj/cpp_scanner.o: | obj_dirs
	$(CC) $(CFLAGS) $(TREE-SITTER_INCLDUES) -c tree-sitter-cpp/src/scanner.c -o $@ $(TARGET_OPT)

arg-parser: arg_parser.o src/find_file.o
	$(CC) $(CFLAGS) -ggdb -o $@ $^

arg_parser.o:
	$(CC) $(CFLAGS) -c arg_parser.c -o $@

src/tarragon.o: tree_sitter.o json_parser.o src/str_builder.o
	$(CC) $(CFLAGS) $(TREE-SITTER_INCLUDES) -c src/tarragon.c -o $@ $^

obj/%.o: src/%.c | obj_dirs
	$(CC) $(CFLAGS) -c $^ -o $@

obj_dirs:
	@mkdir -p obj
	@mkdir -p ts_obj

test: tarragon
	@echo ATTN: TESTS START
	@${CURDIR}/tarragon -t
	@echo ATTN: TESTS SUCCEEDED

clean:
	rm -rf obj/*.o *.exe

deep-clean: clean
	rm -rf ts_obj/*.o

clean-emacs:
	rm -rf *~ src/*~

compile_commands:
	make --always-make --dry-run all \
	| rg -w gcc \
	| rg -w '\-[co]' \
	| jq -nR '[inputs|{command:., directory:".", file: match("\\-c [\\w\\-/]+\\.c").string[3:], output: match("\\-o [\\w\\-/]+\\.o").string[3:]}]' > compile_commands.json
