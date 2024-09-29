#!/usr/bin/env python

import os #NOTE: NEEEEEEED THIS
# import sys
# import platform


env = Environment(tools=['mingw'], CCFLAGS=[
    "-Wall", "-Wextra",
    "-I", "tree-sitter/lib/include",
    "-I", "tree-sitter/lib/src",
])
env.Tool('compilation_db')


test_json_parser = env.Program("test-json-parser", [
    "test_json_parser.c",
    "tree-sitter/lib/src/lib.c",
    "tree-sitter-json/src/parser.c"
])
env.Alias("test1", test_json_parser)

arg_parser = env.Program("arg-parser", "arg_parser.c")
env.Alias("args", arg_parser)

cdb = env.CompilationDatabase()
env.Alias("compilation_database", cdb)

Default(arg_parser)
