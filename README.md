# Tarragon

A standalone GDScript LSP

MAKE SURE YOU `git clone --recursive` all them SUBMODULES, breh

### PLS TEST

tarragon on windows OUTSIDE of emacs/eglot.  Eglot is not sending `\r\n` on Windows.  Is this an eglot specific probelm?  I would test with `nvim` but I truly am a lazy bastard.

### NOTA BENE

You might want to use brotli for binary-ized json string compression.

_BUT_ you really should bake the API trie--then you could get rid of all zip nonsense.  It'll be worth it (wow I love things living in the stack).  THEN you could stop statically linking libarchive (wow slightly smaller binary wowowow).

## LEARN YE C
- [[https://beej.us/guide/bgc/html/split-wide/][Beej's Guide to C Programming]]
- [[https://beej.us/guide/bgclr/html/split-wide/][Beej's C Library Reference]]
- [[https://beej.us/guide/bggdb/][Beej's Guide to `gdb`]]
- [[https://en.cppreference.com/w/c][C reference]]
- [[https://floooh.github.io/2019/09/27/modern-c-for-cpp-peeps.html][C for C++ catechites]]

## NOTES
use the following to get the string of `default_config.json` inside of source files

``` c
// *.c
extern unsigned char default_config_json[];
extern unsigned int default_config_json_len;
```

- https://stackoverflow.com/questions/33588288/what-does-setmode-actually-translate - seriously why is windows stdio trash?
