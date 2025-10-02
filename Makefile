# NOTE: for release build use gcc & mold at -O2
CC := clang -std=gnu99
ifeq ($(OS),Windows_NT)
	RE := rg
else
	RE := grep
endif
CFLAGS := -Wall -Wextra -Wno-int-conversion -fpermissive -fdiagnostics-show-option -ggdb -O2
# CFLAGS := -Wall -Wextra -Wno-int-conversion -fpermissive -fdiagnostics-show-option -O2
LIBS :=
LDFLAGS := -fuse-ld=lld -flto -static
MAINS := src/tarragon.c src/tests.c src/builtin_generator.c
IGNORE := src/gdscript/gdscript_ast.c src/gdscript/gdscript_tokenizer.c src/logger.c src/builtin.c src/allocators/devious.c
ALL_SRC := $(wildcard src/*.c)
ALL_SRC += $(wildcard src/*/*.c)
SRC := $(filter-out $(IGNORE), $(filter-out $(MAINS), $(ALL_SRC)))
DEP := $(patsubst src/%.c,dep/%.d,$(SRC))
OBJ := $(patsubst src/%.c,obj/%.o,$(SRC))
HEX_OBJS := obj/supported_godot.o
# API_OBJS := obj/api/global_constants.o obj/api/global_enums.o obj/api/utility_functions.o obj/api/builtin_classes.o obj/api/classes.o obj/api/singletons.o obj/api/native_structures.o
JSON_OBJS :=  obj/default_config.o
ALL_OBJ := $(patsubst src/%.c,obj/%.o,$(ALL_SRC))
OBJ += $(HEX_OBJS) $(JSON_OBJS)
GODOT := godot

.PHONY: default dependencies clean clean-dep clean-emacs clean-alllsp-test test test2 test/% test_gen ci/cd

default: tarragon

ci/cd: tests tarragon

test/%:
	cat src/$*.c | sed 's/$*_test/main/' | $(CC) $(CFLAGS) -o test_$(patsubst %.c,%,$*) -Isrc -x c -
	${CURDIR}/test_$*
	@echo "TESTED: src/$*.c"

tarragon: obj/tarragon.o obj/builtin.o $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS) $(LDFLAGS)

obj/builtin.o: src/builtin.c src/builtin.gen | dep_dirs
	$(CC) $(CFLAGS) -o $@ $<

src/builtin.gen: builtin-generator extension_api.json
	${CURDIR}/$<

builtin-generator: obj/builtin_generator.o $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS) $(LDFLAGS)

extension_api.json:
	$(GODOT) --dump-extension-api-with-docs

tests: obj/tests.o $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS) $(LDFLAGS)

test: tests
	${CURDIR}/$<

# test: builtin-generator
# 	${CURDIR}/$<

obj/%.o: src/%.c dep/%.d | obj_dirs
	$(CC) $(CFLAGS) -c $< -o $@ $(LIBS)

dependencies: $(DEP)

dep/%.d: src/%.c | dep_dirs
	$(CC) -MM -MP -MT obj/$*.o -MF $@ $< $(LIBS)

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

dep_dirs:
	@mkdir -p dep/
	@mkdir -p dep/gdscript
	@mkdir -p dep/allocators
	@mkdir -p dep/hashmaps

obj_dirs:
	@mkdir -p obj/
	@mkdir -p obj/gdscript
	@mkdir -p obj/allocators
	@mkdir -p obj/hashmaps

clean:
	rm -rf *.o obj/*.o *.exe tarragon

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

lsp-test: tarragoen
	@cho TODO w/ SBCL
