# CC := gcc -std=gnu17
CC := clang -std=gnu17
CFLAGS := -Wall -Wextra -ggdb -O0
# LDFLAGS := -fuse-ld=lld -flto -ljansson ./libarchive/libarchive/libarchive.a -lz -static
# LDFLAGS := -fuse-ld=lld -flto -ljansson -larchive -lz -static
LDFLAGS := -fuse-ld=lld -flto -ljansson -static
SRC := $(wildcard src/*.c)
DEP := $(patsubst src/%.c,dep/%.d,$(SRC))
OBJ := $(patsubst src/%.c,obj/%.o,$(SRC))
# TREE-SITTER_INCLUDES = -I tree-sitter/lib/include -I tree-sitter/lib/src
# TREE_SITTER_OBJ := obj/ts/tree_sitter.o obj/ts/json_parser.o obj/ts/cpp_parser.o obj/ts/cpp_scanner.o
# OBJ += $(TREE_SITTER_OBJ)
HEX_OBJS := obj/supported_godot.o
API_OBJS := obj/api/global_constants.o obj/api/global_enums.o obj/api/utility_functions.o obj/api/builtin_classes.o obj/api/classes.o obj/api/singletons.o obj/api/native_structures.o
JSON_OBJS :=  obj/default_config.o
# TSQUERY_OBJS := obj/client_tsquery.o obj/params_tsquery.o
OBJ += $(HEX_OBJS) $(API_OBJS) $(JSON_OBJS) $(TSQUERY_OBJS)

.PHONY: default test dependencies clean clean-dep clean-ts clean-emacs clean-api clean-all dep_dir obj_dirs lsp-test jansson-test

default: tarragon

tarragon: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	file $@

# test-json-parser: test_json_parser.c ts_obj/tree_sitter.o ts_obj/json_parser.o obj/file_sys.o obj/arena.o $(TSQUERY_OBJS)
	# $(CC) $(CFLAGS) $(TREE-SITTER_INCLUDES) -o $@ $^

# test_json_parser.o: test_json_parser.c
# 	$(CC) $(CFLAGS) $(TREE-SITTER_INCLDUES) -c $< -o $@

# obj/ts/tree_sitter.o: | obj_dirs
# 	$(CC) $(CFLAGS) $(TREE-SITTER_INCLUDES) -c tree-sitter/lib/src/lib.c -o $@

# obj/ts/%_parser.o: | obj_dirs
# 	$(CC) $(CFLAGS) $(TREE-SITTER_INCLDUES) -c tree-sitter-$*/src/parser.c -o $@ $(TARGET_OPT)

# obj/ts/cpp_scanner.o: | obj_dirs
# 	$(CC) $(CFLAGS) $(TREE-SITTER_INCLDUES) -c tree-sitter-cpp/src/scanner.c -o $@ $(TARGET_OPT)

arg-parser: arg_parser.o src/find_file.o
	$(CC) $(CFLAGS) -ggdb -o $@ $^

arg_parser.o:
	$(CC) $(CFLAGS) -c arg_parser.c -o $@

obj/%.o: src/%.c dep/%.d | obj_dirs
	$(CC) $(CFLAGS) -c $< -o $@

dependencies: $(DEP)

dep/%.d: src/%.c | dep_dir
	$(CC) -MM -MP -MT obj/$*.o -MF $@ $<

include $(wildcard dep/*.d)

obj/supported_godot.o: extension_api.json
	@jq -r '.header.version_full_name' $< > supported_godot
	xxd -i supported_godot | $(CC) $(CFLAGS) -xc -c -o $@ -
	@rm supported_godot

# extesnion_api.json:;

$(JSON_OBJS): obj/%.o: %.json | obj_dirs
	xxd -i $< | $(CC) $(CFLAGS) -xc -c -o $@ -

$(API_OBJS): obj/api/%.o: extension_api.json | obj_dirs
	jq -r '.$*' $< | gzip --stdout - > $*.gz
	xxd -i $*.gz | $(CC) $(CFLAGS) -xc -c -o $@ -
	rm $*.gz

$(TSQUERY_OBJS): obj/%_tsquery.o: %_tsquery | obj_dirs
	xxd -i $< | $(CC) $(CFLAGS) -xc -c -o $@ -

dep_dir:
	@mkdir -p dep

obj_dirs:
	@mkdir -p obj/{api,ts}

clean:
	rm -rf *.o obj/*.o *.exe tarragon

clean-dep:
	rm -rf dep/*.d

clean-ts:
	rm -rf obj/ts/*.o

clean-api:
	rm -rf obj/api/*.o

clean-emacs:
	rm -rf *~ src/*~

clean-all: clean clean-dep clean-ts clean-emacs clean-api

compile_commands.json: $(SRC) $(JSON_OBJ)
	$(CC) -MJ temp $(CFLAGS) $(TREE-SITTER_INCLUDES) -c $^ $(LDFLAGS)
	python -c 'with open("temp", "r") as f:\
    print("[\n{}\n]".format(f.read()[:-2]))' > compile_commands.json
	rm temp *.o

# compile_commands.json:
# make --always-make --dry-run --keep-going \
# | rg -w $(CC) \
# | rg -w '\-[co]' \
# | jq -nR '[inputs|{command:., directory:".", file: match("\\-c [\\w\\-/]+\\.c").string[3:], output: match("\\-o [\\w\\-/]+\\.o").string[3:]}]' > compile_commands.json


test: tarragon
	@touch ../../bruh
	@mkdir -p temp_testing
	@cat src/file_sys.c | gzip -c - > file_sys.gz
	@echo "CREATED: temporary testing files"
	@${CURDIR}/tarragon -l tarragon.log -t
	@echo "tests successful!"
	@rm -f ../../bruh ../../tarragon_config.json test_stuff/rpc_encode file_sys.gz
	@echo "CLEANED: temporary testing files"

# @echo $@

# @${CURDIR}/unit/%

unit_dir:
	@mkdir -p unit
	@echo "*" > unit/.gitignore

lsp-test: tarragon
	@python test-tarragon.py

test-parse: test-json-parser
	@$CURDIR/test-json-parser

jansson-test:
	gcc $(CFLAGS) -o jt jansson_test.c  src/file_sys.c src/arena.c $(LIBRARIES)
	@${CURDIR}/jt
