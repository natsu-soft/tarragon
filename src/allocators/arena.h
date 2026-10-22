#ifndef ARENA_H
#define ARENA_H

#include <stdint.h>
#include <stddef.h>
#include "arena_types.h"
#include "../strings_types.h"

void arena_init(arena_t* arena, const int32_t bytes);

void arena_grow(arena_t* arena, const int32_t new_size);

void* arena_alloc(arena_t* arena, const int32_t bytes);

void* arena_dynamic_alloc(arena_t* arena, const int32_t bytes);

void* arena_calloc(arena_t* arena, const int32_t bytes);

void* arena_alloc_aligned(arena_t* arena, const int32_t bytes, const size_t alignment);

void arena_align2(arena_t* arena, const int32_t alignment);

void arena_free(arena_t* arena, const int32_t bytes);

void arena_free_all(arena_t* arena);

string* arena_build_string_puts(arena_t* arena, const string* str);
string* arena_build_string_putc(arena_t* arena, const char c);
string* arena_build_string_vsprintf(arena_t* arena, char* format, ...);


// TODO: check which of these are actually used
// for stack-like behavior
void* arena_pop(arena_t* arena, const int32_t bytes);

void* arena_push(arena_t* arena, const void* src, const int32_t bytes);

void arena_dynamic_push(arena_t* arena, const void* src, const int32_t bytes);

void* arena_peek(arena_t* arena, const int32_t bytes);

// tight pack cat
void arena_cat(arena_t* arena0, const arena_t* arena1);

void arena_insert(arena_t* arena, const int32_t pos, const void* src, const int32_t bytes);

#endif /* End ARENA_H */
