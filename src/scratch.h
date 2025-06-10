#ifndef SCRATCH_H
#define SCRATCH_H

#include <stddef.h>

// https://stackoverflow.com/questions/34574933/a-good-and-idiomatic-way-to-use-gcc-and-clang-attribute-cleanup-and-point

/* memory leaks are a skill issue */

void* scratch_alloc(const size_t bytes);

void* scratch_calloc(const size_t bytes);

struct arena_t* checkout_scratch(void);

void checkin_scratch(struct arena_t* scratch_arena);

#endif /* End SCRATCH_H */
