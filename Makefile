CC := gcc
CFLAGS := -Wall -Wextra -ggdb3 -O2
ifdef TEST
	CFLAGS := $(CFLAGS) -DTEST
endif
SRC := $(wildcard src/*.c)
TREE-SITTER_INCLUDES = -I tree-sitter/lib/include -I tree-sitter/lib/src
TREE_SITTER_OBJ := ts_obj/tree_sitter.o ts_obj/cpp_parser.o ts_obj/cpp_scanner.o
OBJ := $(patsubst src/%.c,obj/%.o,$(SRC)) $(TREE_SITTER_OBJ)

.PHONY: default tarragon test clean deep-clean compile_commands obj_dir

default: tarragon

tarragon: $(OBJ)
	$(CC) $(CFLAGS) -ggdb -o $@ $^

all: default test-json-parser

test-json-parser: test_json_parser.o tree_sitter.o json_parser.o
	$(CC) $(TREE-SITTER_INCLUDES) -o $@ $^

ts_obj/tree_sitter.o: | ts_obj_dir
	$(CC) $(CFLAGS) $(TREE-SITTER_INCLUDES) -c tree-sitter/lib/src/lib.c -o $@
ts_obj/%_parser.o: | ts_obj_dir
	$(CC) $(CFLAGS) $(TREE-SITTER_INCLDUES) -c tree-sitter-$*/src/parser.c -o $@ $(TARGET_OPT)
ts_obj/cpp_scanner.o: | ts_obj_dir
	$(CC) $(CFLAGS) $(TREE-SITTER_INCLDUES) -c tree-sitter-cpp/src/scanner.c -o $@ $(TARGET_OPT)

arg-parser: arg_parser.o src/find_file.o
	$(CC) $(CFLAGS) -ggdb -o $@ $^

arg_parser.o:
	$(CC) $(CFLAGS) -c arg_parser.c -o $@

src/tarragon.o: tree_sitter.o json_parser.o src/str_builder.o
	$(CC) $(CFLAGS) $(TREE-SITTER_INCLUDES) -c src/tarragon.c -o $@ $^

# src/%.o: src/%.c
# 	$(CC) $(CFLAGS) -c $^ -o $@

obj/%.o: src/%.c | obj_dir
	$(CC) $(CFLAGS) -c $^ -o $@

obj_dir:
	@mkdir -p obj

ts_obj_dir:
	@mkdir -p ts_obj

test:
	@rm -f obj/main.o
	make tarragon TEST=1
	${CURDIR}/tarragon
	@echo ATTN: TESTS START
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

# | jq -nR '[inputs|{command:., directory:"${CURDIR}", file: match("\\-c [\\w\\-/]+\\.c").string[3:], output: match("\\-o [\\w\\-/]+\\.o").string[3:]}]' > compile_commands.json
