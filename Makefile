CC := clang -std=gnu99
ifeq ($(OS),Windows_NT)
	# CC := clang -std=gnu99
	RE := rg
else
	# CC := gcc -std=gnu99
	RE := grep
endif
CFLAGS := -Wall -Wextra -Wno-int-conversion -fpermissive -fdiagnostics-show-option -ggdb -O0
ifeq ($(OS),Windows_NT)
LIBS :=
LDFLAGS := -fuse-ld=lld -flto -static
else
# LIBS := -L /home/jon/Repos/llvm-project/build/libc/lib -lllvmlibc
LIBS :=
LDFLAGS := -fuse-ld=mold -flto -static
endif
MAINS := src/tarragon.c src/tests.c
IGNORE := src/gdscript_ast.c
ALL_SRC := $(wildcard src/*.c)
SRC := $(filter-out $(IGNORE), $(filter-out $(MAINS), $(ALL_SRC)))
DEP := $(patsubst src/%.c,dep/%.d,$(ALL_SRC))
OBJ := $(patsubst src/%.c,obj/%.o,$(SRC))
HEX_OBJS := obj/supported_godot.o
# API_OBJS := obj/api/global_constants.o obj/api/global_enums.o obj/api/utility_functions.o obj/api/builtin_classes.o obj/api/classes.o obj/api/singletons.o obj/api/native_structures.o
JSON_OBJS :=  obj/default_config.o
ALL_OBJ := $(patsubst src/%.c,obj/%.o,$(ALL_SRC))
OBJ += $(HEX_OBJS) $(JSON_OBJS)

.PHONY: default dependencies clean clean-dep clean-emacs clean-api clean-all dep_dir obj_dirs lsp-test jansson-test test test2 test/%

default: tarragon

test/%:
	cat src/$*.c | sed 's/$*_test/main/' | $(CC) $(CFLAGS) -o test_$(patsubst %.c,%,$*) -Isrc -x c -
	${CURDIR}/test_$*
	@echo "TESTED: src/$*.c"

tarragon: obj/tarragon.o $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS) $(LDFLAGS)
	file $@

tests: obj/tests.o $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS) $(LDFLAGS)

test: tests
	${CURDIR}/$<

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

compile_commands.json: $(ALL_SRC)
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
