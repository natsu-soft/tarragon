ifeq ($(OS),Windows_NT)
	CC := clang -std=gnu99
	RE := rg
else
	CC := gcc -std=gnu99
	RE := grep
endif
# CFLAGS := -Wall -Wextra -Wno-incompatible-pointer-types -ggdb -O0
CFLAGS := -Wall -Wextra  -fpermissive -fdiagnostics-show-option -ggdb
ifeq ($(OS),Windows_NT)
CFLAGS += -Wno-int-conversion -O0
else
CFLAGS += -Og
endif
# LDFLAGS := -fuse-ld=lld -flto -ljansson ./libarchive/libarchive/libarchive.a -lz -static
# LDFLAGS := -fuse-ld=lld -flto -ljansson -larchive -lz -static
LIBS := -ljansson
ifeq ($(OS),Windows_NT)
LDFLAGS := -fuse-ld=lld -flto $(LIBS) -static
else
LDFLAGS := -fuse-ld=mold -flto $(LIBS) -static
endif
MAINS := src/tarragon.c src/unit_tests.c src/gdscript_parser.c
ALL_SRC := $(wildcard src/*.c)
SRC := $(filter-out $(MAINS), $(ALL_SRC))
DEP := $(patsubst src/%.c,dep/%.d,$(ALL_SRC))
OBJ := $(patsubst src/%.c,obj/%.o,$(SRC))
# TREE-SITTER_INCLUDES = -I tree-sitter/lib/include -I tree-sitter/lib/src
# TREE_SITTER_OBJ := obj/ts/tree_sitter.o obj/ts/json_parser.o obj/ts/cpp_parser.o obj/ts/cpp_scanner.o
# OBJ += $(TREE_SITTER_OBJ)
HEX_OBJS := obj/supported_godot.o
# API_OBJS := obj/api/global_constants.o obj/api/global_enums.o obj/api/utility_functions.o obj/api/builtin_classes.o obj/api/classes.o obj/api/singletons.o obj/api/native_structures.o
JSON_OBJS :=  obj/default_config.o
# TSQUERY_OBJS := obj/client_tsquery.o obj/params_tsquery.o
# OBJ += $(HEX_OBJS) $(API_OBJS) $(JSON_OBJS) $(TSQUERY_OBJS)
ALL_OBJ := $(patsubst src/%.c,obj/%.o,$(ALL_SRC))
OBJ += $(HEX_OBJS) $(JSON_OBJS)

.PHONY: default dependencies clean clean-dep clean-emacs clean-api clean-all dep_dir obj_dirs lsp-test jansson-test test test2

default: gdscript-parser

gdscript-parser: obj/gdscript_parser.o $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS) $(LDFLAGS)


test: gdscript-parser
	@${CURDIR}/$<

tarragon: obj/tarragon.o $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS) $(LDFLAGS)
	file $@

unit-tests: obj/unit_tests.o $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS) $(LDFLAGS)

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

# arg-parser: arg_parser.o src/find_file.o
# 	$(CC) $(CFLAGS) -ggdb -o $@ $^

# arg_parser.o:
# 	$(CC) $(CFLAGS) -c arg_parser.c -o $@

obj/%.o: src/%.c dep/%.d | obj_dirs
	$(CC) $(CFLAGS) -c $< -o $@ $(LIBS)

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

dep_dir:
	@mkdir -p dep

obj_dirs:
	@mkdir -p obj/api

clean:
	rm -rf *.o obj/*.o *.exe tarragon

clean-dep:
	rm -rf dep/*.d

clean-emacs:
	rm -rf *~ src/*~

clean-all: clean clean-dep clean-ts clean-emacs clean-api

# compile_commands.json: $(SRC) $(JSON_OBJ)
# 	$(CC) -MJ temp $(CFLAGS) $(TREE-SITTER_INCLUDES) -c $^ $(LDFLAGS)
# 	python -c 'with open("temp", "r") as f:\
#     print("[\n{}\n]".format(f.read()[:-2]))' > compile_commands.json
# 	rm temp *.o

compile_commands.json:
	make $(ALL_OBJ) --always-make --dry-run \
	| $(RE) -w '$(CC)' \
	| $(RE) -w '\-[co]' \
	| jq -nR '[inputs|{command:., directory:".", file: match("\\-c [\\w\\-/]+\\.c").string[3:], output: match("\\-o [\\w\\-/]+\\.o").string[3:]}]' > compile_commands.json

test2: unit-tests
	@touch ../../bruh
	@mkdir -p temp_testing
	@cat src/file_sys.c | gzip -c - > file_sys.gz
	@echo "CREATED: temporary testing files"
	@${CURDIR}/$< -l tarragon.log
	@echo "tests successful!"
	@rm -f ../../bruh ../../tarragon_config.json test_stuff/rpc_encode file_sys.gz
	@echo "CLEANED: temporary testing files"

lsp-test: tarragon
	@python test-tarragon.py
