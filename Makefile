# NOTE: for release build use gcc & mold at -O2
# CC := clang
CC := gcc
ifeq ($(OS),Windows_NT)
	RE := rg
else
	RE := grep
endif
CFLAGS := -std=gnu17 -Wall -Wextra -Wno-int-conversion -fpermissive -fdiagnostics-show-option -ggdb -O0
# CFLAGS := -std=gnu17 -Wall -Wextra -Wno-int-conversion -fpermissive -fdiagnostics-show-option -g0 -O2
GODOT :=

-include godot.mine

#LDFLAGS := -flto -static
LDFLAGS := -fuse-ld=mold -flto -static
MAINS := src/tarragon.c src/tests.c src/builtin_generator.c src/massive_test.c
IGNORE := src/gdscript/gdscript_ast.c src/gdscript/gdscript_tokenizer.c src/logger.c src/builtin.c src/allocators/devious.c src/fml.c src/api_generator.c src/godot_api.c src/massive_script_writer.c
ALL_SRC := $(wildcard src/*.c)
ALL_SRC += $(wildcard src/*/*.c)
SRC := $(filter-out $(IGNORE), $(filter-out $(MAINS), $(ALL_SRC)))
DEP := $(patsubst src/%.c,dep/%.d,$(SRC))
OBJ := $(patsubst src/%.c,obj/%.o,$(SRC))
ALL_OBJ := $(patsubst src/%.c,obj/%.o,$(ALL_SRC))

SRC_DIRS := src/ $(wildcard src/*/)
OBJ_DIRS := $(patsubst src/%,obj/%,$(SRC_DIRS))
DEP_DIRS := $(patsubst src/%,dep/%,$(SRC_DIRS))

.PHONY: default dependencies clean clean-dep clean-emacs clean-all lsp-test test test2 massive-test unity-tarragon

default: tarragon

%.includes:
	echo -n "$(shell echo -n '$*' | tr '[:lower:]' '[:upper:]')_INCLUDES=" > $@
	echo -n "$(shell parallel-includes -j8 $(CC) src/$*.c)" >> $@

TARRAGON_INCLUDES:=
include tarragon.includes

tarragon: $(patsubst obj/%.o, src/%.c, $(filter %.c, $(TARRAGON_INCLUDES))) | tarragon.includes
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

unity-tarragon:  $(filter %.c, $(TARRAGON_INCLUDES)) | tarragon.includes
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

src/builtin.gen: builtin-generator extension_api.json
	${CURDIR}/$<

builtin-generator: obj/builtin_generator.o obj/mem_sys.o obj/allocators/arena.o obj/allocators/scratch.o obj/allocators/workpad.o obj/gdscript/hashmaps.o obj/hashmaps/gid_map.o obj/perfect_json.o
	$(CC) $(CFLAGS) -o $@ $^

api-generator: obj/api_generator.o obj/mem_sys.o obj/allocators/arena.o obj/allocators/scratch.o obj/allocators/workpad.o obj/hashmaps/api_map.o obj/perfect_json.o obj/gdscript/token_ids.o obj/gdscript/token_id_map.o obj/api_structs.o obj/strings.o;
	$(CC) $(CFLAGS) -o $@ $^

extension_api.json:
	$(GODOT) --dump-extension-api-with-docs --headless

tests: obj/tests.o $(OBJ) obj/godot_api.o obj/godot_api.gen.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.h: ;

# test: api-generator extension_api.json
test: tests extension_api.json
	${CURDIR}/$<


src/godot_api.gen.c src/goodt_api.gen.h: api-generator extension_api.json
	${CURDIR}/$<

obj/builtin_generator.o: src/builtin_generator.c src/api_structs.h dep/builtin_generator.d | $(DEP_DIRS)
	$(CC) $(CFLAGS) -o $@ -c $<

obj/builtin.o: src/builtin.c src/builtin.gen | $(DEP_DIRS)
	$(CC) $(CFLAGS) -o $@ -c $<

obj/godot_api.gen.o: src/godot_api.gen.c | $(OBJ_DIRS)
	$(CC) $(CFLAGS) -c $< -o $@

# NOTE: you're using objcopy WRONG!
obj/godot_api.o: src/godot_api.c src/godot_api.gen.h obj/godot_api.gen.o dep/godot_api.d | $(DEP_DIRS)
	$(CC) $(CFLAGS) -o $@ -c $<

obj/%.o: src/%.c dep/%.d | $(OBJ_DIRS)
	$(CC) $(CFLAGS) -c $< -o $@

dependencies: $(DEP)

dep/%.d: src/%.c | $(DEP_DIRS)
	$(CC) -MM -MP -MT obj/$*.o -MF $@ $<

-include $(wildcard dep/*.d) $(wildcard dep/**/*.d)

# obj/supported_godot.o: extension_api.json
# 	@jq -r '.header.version_full_name' $< > supported_godot
# 	xxd -i supported_godot | $(CC) $(CFLAGS) -xc -c -o $@ -
# 	@rm supported_godot

# extesnion_api.json:;

$(JSON_OBJS): obj/%.o: %.json | obj_dirs
	xxd -i $< | $(CC) $(CFLAGS) -xc -c -o $@ -

$(API_OBJS): obj/api/%.o: extension_api.json | obj_dirs
	jq -r '.$*' $< | gzip --stdout - > $*.gz
	xxd -i $*.gz | $(CC) $(CFLAGS) -xc -c -o $@ -
	rm $*.gz

$(OBJ_DIRS) $(DEP_DIRS): %:
	mkdir -p $@

clean:
	rm -rf *.o obj/ dep/ *.exe tarragon tests extension_api.json src/godot_api.gen.c src/goodt_api.gen.h

clean-dep:
	rm -rf dep/*.d

clean-emacs:
	rm -rf *~ src/*~

clean-all: clean clean-dep clean-emacs

compile_commands.json: $(ALL_SRC)
	make $(ALL_OBJ) --always-make --dry-run \
	| $(RE) -w '$(CC)' \
	| $(RE) -w '\-[co]' \
	| jq -nR '[inputs|{command:., directory:".", file: match("\\-c [\\w\\-/]+\\.c").string[3:], output: match("\\-o [\\w\\-/]+\\.o").string[3:]}]' > compile_commands.json

test2: tests
	@touch ../../bruh
	@mkdir -p temp_testing
	@cat src/file_sys.c | gzip -c - > file_sys.gz
	@echo "CREATED: temporary testing files"
	@${CURDIR}/$< -l tarragon.log
	@echo "tests successful!"
	@rm -f ../../bruh ../../tarragon_config.json test_stuff/rpc_encode file_sys.gz
	@echo "CLEANED: temporary testing files"

lsp-test: tarragon
	@echo TODO w/ SBCL


massive-script-writer: src/massive_script_writer.c
	$(CC) $(CFLAGS) -o $@ $^

Massive.gd: massive-script-writer
	${CURDIR}/$<

massive-tester: obj/massive_test.o obj/memery.o obj/mem_sys.o obj/allocators/arena.o obj/gdscript/diagnostics.o obj/gdscript/print_diagnostics.o obj/gdscript/lex.o obj/gdscript/parse.o  obj/gdscript/token_id_map.o
	$(CC) $(CFLAGS) -o $@ $^

massive-test: massive-tester Massive.gd
	${CURDIR}/$<
