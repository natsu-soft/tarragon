# Tarragon

A standalone GDScript LSP

MAKE SURE YOU `git clone --recursive` all them SUBMODULES, breh

### NOTA BENE

You might want to use brotli for binary-ized json string compression.

_BUT_ you really should bake the API trie--then you could get rid of all zip nonsense.  It'll be worth it (wow I love things living in the stack).  THEN you could stop statically linking libarchive (wow slightly smaller binary wowowow).

## libarchive

used for unarchiving baked strings at runtime, have to build it from source otherwise you can't statically link it

``` sh
# build instructions
cd libarchive
cmake .
make archive_static -j 8
```

## LEARN YE C
- [[https://beej.us/guide/bgc/html/split-wide/][Beej's Guide to C Programming]]
- [[https://beej.us/guide/bgclr/html/split-wide/][Beej's C Library Reference]]
- [[https://beej.us/guide/bggdb/][Beej's Guide to `gdb`]]
- [[https://en.cppreference.com/w/c][C reference]]
- [[https://floooh.github.io/2019/09/27/modern-c-for-cpp-peeps.html][C for C++ catechites]]

## Tree-sitter
> All of Tree-sitter’s parsing functionality is exposed through C APIs.
- [[https://tree-sitter.github.io/tree-sitter/using-parsers][Using Parsers]]
- [[https://github.com/tree-sitter/tree-sitter/wiki/List-of-parsers][list of parsers]]--includs GDScript & JSON & C++!

## Jannson
- https://github.com/akheron/jansson - repo
- https://jansson.readthedocs.io/en/latest/ - docs


## NOTES
use the following to get the string of `default_config.json` inside of source files

``` c
// *.c
extern unsigned char default_config_json[];
extern unsigned int default_config_json_len;
```

- https://stackoverflow.com/questions/33588288/what-does-setmode-actually-translate - seriously why is windows stdio trash?
