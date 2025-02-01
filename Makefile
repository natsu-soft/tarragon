CC := clang
CFLAGS := -Wall -Wextra -ggdb -O2
SRC := $(wildcard src/*.c)
DEP := $(patsubst src/%.c,dep/%.d,$(SRC))
CCMD := $(patsubst src/%.c,ccmd/%.json,$(SRC))
OBJ := $(patsubst src/%.c,obj/%.o,$(SRC))
TREE-SITTER_INCLUDES = -I tree-sitter/lib/include -I tree-sitter/lib/src
TREE_SITTER_OBJ := ts_obj/tree_sitter.o ts_obj/json_parser.o ts_obj/cpp_parser.o ts_obj/cpp_scanner.o
OBJ += $(TREE_SITTER_OBJ)
JSON_OBJ := obj/default_config.o
OBJ += $(JSON_OBJ)

.PHONY: default test dependencies clean deep-clean dep_dir obj_dirs

default: tarragon

tarragon: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

test-json-parser: obj/test_json_parser.o ts_obj/tree_sitter.o ts_obj/json_parser.o
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

obj/%.o: src/%.c dep/%.d compile_commands.json | obj_dirs
	$(CC) $(CFLAGS) -c $< -o $@

obj/tests.o: src/tests.c dep/tests.d | obj_dirs
	$(CC) $(CFLAGS) -c $< -o $@

dependencies: $(DEP)

dep/%.d: src/%.c | dep_dir
	$(CC) -MM -MP -MT obj/$*.o -MF $@ $<

include $(wildcard dep/*.d)

$(JSON_OBJ): obj/%.o: %.json | obj_dirs
	xxd -i $< | $(CC) $(CFLAGS) -xc -c -o $@ -

dep_dir:
	@mkdir -p dep

obj_dirs:
	@mkdir -p obj
	@mkdir -p ts_obj

clean:
	rm -rf obj/*.o dep/*.d *.exe tarragon

deep-clean: clean
	rm -rf ts_obj/*.o

clean-emacs:
	rm -rf *~ src/*~

compile_commands.json: $(SRC) $(JSON_OBJ)
	$(CC) -MJ temp $(CFLAGS) $(TREE-SITTER_INCLUDES) -c $^
	python -c 'with open("temp", "r") as f:\
    print("[\n{}\n]".format(f.read()[:-2]))' > compile_commands.json
	rm temp

# compile_commands.json:
# make --always-make --dry-run --keep-going \
# | rg -w $(CC) \
# | rg -w '\-[co]' \
# | jq -nR '[inputs|{command:., directory:".", file: match("\\-c [\\w\\-/]+\\.c").string[3:], output: match("\\-o [\\w\\-/]+\\.o").string[3:]}]' > compile_commands.json

test: tarragon
	@touch ../../bruh
	@mkdir -p temp_testing
	@echo "CREATED: temporary testing files"
	@${CURDIR}/tarragon -t
	@rm ../../bruh ../../tarragon_config.json test_stuff/rpc_encode
	@echo "CLEANED: temporary testing files"
