#ifndef SCRATCH_H
#define SCRATCH_H

#include <stddef.h>

// https://stackoverflow.com/questions/34574933/a-good-and-idiomatic-way-to-use-gcc-and-clang-attribute-cleanup-and-point
// memory leaks are a skill issue
// so are double frees

void* scratch_alloc(const size_t bytes);

void* scratch_calloc(const size_t bytes);

struct arena_t* scratch_checkout(void);

void scratch_checkin(struct arena_t* scratch);

#ifndef CLEANUP_SCRATCH
static inline void _cleanup_scratch(struct arena_t** p) { scratch_checkin(*p); }
#define CLEANUP_SCRATCH __attribute__((__cleanup__(_cleanup_scratch)))
#endif

#endif /* End SCRATCH_H */
